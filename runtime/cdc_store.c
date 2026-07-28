#define _POSIX_C_SOURCE 200809L

#include "cdc_store.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Record layout, format v2 (little-endian fixed fields; 3122af5 re-review
 * final repair — framing metadata is AUTHENTICATED before it is trusted):
 *   [4]  magic  "CDC2"
 *   [1]  type   'D' data / 'S' seal
 *   [8]  seq    (data: event ordinal; seal: sealed transaction ordinal)
 *   [4]  payload length (seal: 0)
 *   [32] framing tag: digest over the preceding 17 framing bytes
 *        (magic | type | seq | length). The scanner verifies this tag
 *        BEFORE allocating or reading payload_len, so a mutated length
 *        field can never masquerade as a torn tail.
 *   [32] payload digest (seal: digest of the transaction's event digests)
 *   [n]  payload
 * A transaction = its DATA records followed by one SEAL. Replay/recovery
 * accept only complete, tag-valid, digest-valid, sequence-continuous
 * records and only up to the last SEAL. Records above
 * CDC_STORE_MAX_RECORD fail closed before any allocation. */

static const uint8_t MAGIC[4] = {'C', 'D', 'C', '2'};
enum {
    FRAMING_SIZE = 4 + 1 + 8 + 4,
    OFF_TAG = FRAMING_SIZE,
    OFF_DIGEST = FRAMING_SIZE + CDC_DIGEST_SIZE,
    HEADER_SIZE = FRAMING_SIZE + 2 * CDC_DIGEST_SIZE,
    CDC_STORE_MAX_RECORD = 64 << 20
};

typedef struct {
    uint8_t *payload;
    size_t size;
} staged_event;

struct cdc_store {
    char dir[512];
    char log_path[600];
    char snapshot_path[620];
    uint64_t sealed;      /* sealed transactions visible (base + log) */
    uint64_t events;      /* events in sealed transactions (base + log) */
    long valid_bytes;     /* log byte length covering the sealed prefix */
    /* Compaction base: a snapshot lets the log be truncated while the
     * REPLAY identity is preserved, because the replay digest is a chain
     * that can resume from a recorded state. */
    int has_base;
    uint64_t base_sealed;
    uint64_t base_events;
    uint8_t base_state[CDC_DIGEST_SIZE];
    /* Compare-and-set: an armed fence pins the sealed count a writer
     * believes it is extending; commit re-reads the log and refuses if
     * another writer moved it (stale writer). */
    int fence_armed;
    uint64_t fence_seal;
    staged_event *staged;
    size_t staged_count;
    size_t staged_cap;
    int fail_after;       /* injection: abort after N boundary ops */
    int ops_done;
    int kill_after;       /* injection: SIGKILL after N boundary ops */
};

/* Snapshot record: magic | version | sealed | events | state | tag.
 * The tag authenticates every preceding byte, so a mutated snapshot can
 * never resurrect a wrong base — it fails closed exactly like the log. */
enum {
    SNAP_BODY = 4 + 1 + 8 + 8 + CDC_DIGEST_SIZE,
    SNAP_SIZE = SNAP_BODY + CDC_DIGEST_SIZE
};
static const uint8_t SNAP_MAGIC[4] = {'C', 'D', 'C', 'S'};

static cdc_store_status snapshot_load(const char *path, int *found,
                                      uint64_t *sealed, uint64_t *events,
                                      uint8_t state[CDC_DIGEST_SIZE]);
static int sync_path(const char *path);

const char *cdc_store_status_name(cdc_store_status status) {
    switch (status) {
    case CDC_STORE_OK:
        return "ok";
    case CDC_STORE_EARG:
        return "argument";
    case CDC_STORE_EIO:
        return "io";
    case CDC_STORE_EMEM:
        return "memory";
    case CDC_STORE_ECORRUPT:
        return "corrupt-tail";
    case CDC_STORE_ECRASH:
        return "injected-crash";
    case CDC_STORE_ESTATE:
        return "state";
    case CDC_STORE_EUNSUPPORTED:
        return "unsupported";
    case CDC_STORE_EUNSEALED:
        return "unsealed-tail";
    default:
        return "unknown";
    }
}

static void put_u64(uint8_t *out, uint64_t value) {
    int i;
    for (i = 0; i < 8; i++) {
        out[i] = (uint8_t)(value >> (i * 8));
    }
}

static uint64_t get_u64(const uint8_t *in) {
    uint64_t value = 0;
    int i;
    for (i = 0; i < 8; i++) {
        value |= (uint64_t)in[i] << (i * 8);
    }
    return value;
}

static void put_u32(uint8_t *out, uint32_t value) {
    int i;
    for (i = 0; i < 4; i++) {
        out[i] = (uint8_t)(value >> (i * 8));
    }
}

static uint32_t get_u32(const uint8_t *in) {
    uint32_t value = 0;
    int i;
    for (i = 0; i < 4; i++) {
        value |= (uint32_t)in[i] << (i * 8);
    }
    return value;
}

/* One link of the resumable replay chain. */
static void fold_chain(uint8_t chain[CDC_DIGEST_SIZE],
                       const uint8_t record_digest[CDC_DIGEST_SIZE]) {
    cdc_digest_ctx ctx;
    uint8_t next[CDC_DIGEST_SIZE];
    cdc_digest_init(&ctx);
    cdc_digest_update(&ctx, chain, CDC_DIGEST_SIZE);
    cdc_digest_update(&ctx, record_digest, CDC_DIGEST_SIZE);
    cdc_digest_final(&ctx, next);
    memcpy(chain, next, CDC_DIGEST_SIZE);
}

/* Typed scan result shared by open/verify/replay/attest (review B1/B2):
 * SCAN_CLEAN — sealed prefix is the whole file;
 * SCAN_TAIL — a physically incomplete final record or fully valid but
 *             unsealed transaction tail follows the sealed prefix
 *             (recoverable by truncation, latch-or-hold);
 * SCAN_CORRUPT — an integrity violation inside a structurally complete
 *             record (magic, type, length, payload digest, sequence,
 *             or seal digest): fail closed, never mutate. */
typedef enum { SCAN_CLEAN = 0, SCAN_TAIL = 1, SCAN_CORRUPT = 2 } scan_state;

typedef struct {
    uint64_t sealed;
    uint64_t events; /* events within the sealed prefix */
    long valid_bytes;
    scan_state state;
    /* Chained replay identity: state_0 is the base (all zero, or the state
     * a snapshot recorded), and each sealed record folds in as
     * state_i = digest(state_{i-1} || record_digest_i). The chain is
     * RESUMABLE, so compacting the log away and continuing from a snapshot
     * yields the same replay digest — semantic identity survives a
     * physical rewrite, while the attest digest (raw bytes) legitimately
     * changes. */
    uint8_t replay_state[CDC_DIGEST_SIZE];
} scan_result;

/* Read-fault injection for the scan path (f1f68c0 re-review): simulates a
 * mid-read I/O error after N successful scan reads so the EOF-vs-fault
 * distinction is permanently testable. 0 disarms. */
static int scan_read_fail_at;
static int scan_read_ops;
static int scan_read_injected;

void cdc_store_set_read_fail_after(int operations) {
    scan_read_fail_at = operations;
    scan_read_ops = 0;
    scan_read_injected = 0;
}

static size_t scan_fread(void *buffer, size_t size, FILE *fp) {
    if (scan_read_fail_at > 0) {
        scan_read_ops++;
        if (scan_read_ops >= scan_read_fail_at) {
            scan_read_injected = 1;
            return 0;
        }
    }
    return fread(buffer, 1, size, fp);
}

/* A short or zero read is EOF only when the stream carries no error
 * indicator; a real read fault must surface as CDC_STORE_EIO and may
 * NEVER be classified as a torn tail (which would authorize destructive
 * truncation). */
static int scan_read_faulted(FILE *fp) {
    return ferror(fp) || scan_read_injected;
}

/* base_events/base_sealed continue sequence numbering across a compaction;
 * base_state seeds the replay chain. */
static cdc_store_status scan_log_from(const char *path, uint64_t base_events,
                                      uint64_t base_sealed,
                                      const uint8_t base_state[CDC_DIGEST_SIZE],
                                      scan_result *result) {
    FILE *fp = fopen(path, "rb");
    long offset = 0;
    uint64_t expect_event = base_events + 1;
    uint64_t expect_seal = base_sealed + 1;
    cdc_digest_ctx txn_ctx;
    int txn_open = 0;
    uint8_t chain[CDC_DIGEST_SIZE];
    uint8_t sealed_chain[CDC_DIGEST_SIZE];

    memset(result, 0, sizeof(*result));
    result->state = SCAN_CLEAN;
    result->sealed = base_sealed;
    result->events = base_events;
    memcpy(chain, base_state, CDC_DIGEST_SIZE);
    memcpy(sealed_chain, base_state, CDC_DIGEST_SIZE);
    memcpy(result->replay_state, base_state, CDC_DIGEST_SIZE);
    if (!fp) {
        return errno == ENOENT ? CDC_STORE_OK : CDC_STORE_EIO;
    }
    /* The log must be a regular file; directories, FIFOs, and devices are
     * I/O errors before a single byte is interpreted. */
    {
        struct stat st;
        if (fstat(fileno(fp), &st) != 0 || !S_ISREG(st.st_mode)) {
            fclose(fp);
            return CDC_STORE_EIO;
        }
    }
    for (;;) {
        uint8_t header[HEADER_SIZE];
        size_t got = scan_fread(header, sizeof(header), fp);
        uint32_t payload_len;
        uint64_t seq;
        uint8_t type;
        if (got < sizeof(header) && scan_read_faulted(fp)) {
            fclose(fp);
            return CDC_STORE_EIO;
        }
        if (got == 0) {
            break; /* clean record boundary at EOF */
        }
        if (got < sizeof(header)) {
            result->state = SCAN_TAIL; /* physically incomplete header */
            break;
        }
        if (memcmp(header, MAGIC, sizeof(MAGIC)) != 0) {
            result->state = SCAN_CORRUPT;
            break;
        }
        /* Authenticate the framing (magic, type, seq, length) BEFORE
         * trusting payload_len (3122af5 re-review): a complete header
         * whose tag fails is corruption, never a torn tail. */
        {
            uint8_t tag[CDC_DIGEST_SIZE];
            cdc_digest(header, FRAMING_SIZE, tag);
            if (memcmp(tag, header + OFF_TAG, CDC_DIGEST_SIZE) != 0) {
                result->state = SCAN_CORRUPT;
                break;
            }
        }
        type = header[4];
        seq = get_u64(header + 5);
        payload_len = get_u32(header + 13);
        if (payload_len > (uint32_t)CDC_STORE_MAX_RECORD) {
            result->state = SCAN_CORRUPT; /* bound enforced pre-allocation */
            break;
        }
        if (type == 'D') {
            uint8_t *payload = malloc(payload_len ? payload_len : 1);
            uint8_t digest[CDC_DIGEST_SIZE];
            size_t read_len;
            if (!payload) {
                fclose(fp);
                return CDC_STORE_EMEM;
            }
            read_len = scan_fread(payload, payload_len, fp);
            if (read_len != payload_len) {
                free(payload);
                if (scan_read_faulted(fp)) {
                    fclose(fp);
                    return CDC_STORE_EIO;
                }
                result->state = SCAN_TAIL; /* incomplete payload at EOF */
                break;
            }
            cdc_digest(payload, payload_len, digest);
            free(payload);
            if (memcmp(digest, header + OFF_DIGEST, CDC_DIGEST_SIZE) != 0 ||
                seq != expect_event) {
                result->state = SCAN_CORRUPT;
                break;
            }
            expect_event++;
            if (!txn_open) {
                cdc_digest_init(&txn_ctx);
                txn_open = 1;
            }
            cdc_digest_update(&txn_ctx, header + OFF_DIGEST,
                              CDC_DIGEST_SIZE);
            fold_chain(chain, header + OFF_DIGEST);
            offset += HEADER_SIZE + (long)payload_len;
        } else if (type == 'S') {
            uint8_t seal_expected[CDC_DIGEST_SIZE];
            if (payload_len != 0 || seq != expect_seal || !txn_open) {
                result->state = SCAN_CORRUPT;
                break;
            }
            cdc_digest_final(&txn_ctx, seal_expected);
            txn_open = 0;
            if (memcmp(seal_expected, header + OFF_DIGEST,
                       CDC_DIGEST_SIZE) != 0) {
                result->state = SCAN_CORRUPT;
                break;
            }
            expect_seal++;
            offset += HEADER_SIZE;
            result->sealed++;
            result->valid_bytes = offset;
            result->events = expect_event - 1;
            /* the chain advances only at a seal: unsealed work is not
             * part of the replay identity */
            fold_chain(chain, header + OFF_DIGEST);
            memcpy(sealed_chain, chain, CDC_DIGEST_SIZE);
            memcpy(result->replay_state, sealed_chain, CDC_DIGEST_SIZE);
        } else {
            result->state = SCAN_CORRUPT;
            break;
        }
    }
    fclose(fp);
    /* fully valid DATA records after the last seal are an unsealed tail */
    if (result->state == SCAN_CLEAN && offset != result->valid_bytes) {
        result->state = SCAN_TAIL;
    }
    return CDC_STORE_OK;
}

/* fsync a path (file or directory); best effort errors surface as EIO. */
static int sync_path(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    if (fsync(fd) != 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

cdc_store_status cdc_store_open(const char *dir, cdc_store **out,
                                int *recovered_out) {
    cdc_store *store;
    cdc_store_status status;
    scan_result scan;
    int written;

    if (!dir || !out) {
        return CDC_STORE_EARG;
    }
    *out = NULL;
    if (recovered_out) {
        *recovered_out = 0;
    }
    if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
        return CDC_STORE_EIO;
    }
    store = calloc(1, sizeof(*store));
    if (!store) {
        return CDC_STORE_EMEM;
    }
    written = snprintf(store->dir, sizeof(store->dir), "%s", dir);
    if (written < 0 || (size_t)written >= sizeof(store->dir)) {
        free(store);
        return CDC_STORE_EARG;
    }
    written = snprintf(store->log_path, sizeof(store->log_path),
                       "%s/log.cdcstore", dir);
    if (written < 0 || (size_t)written >= sizeof(store->log_path)) {
        free(store);
        return CDC_STORE_EARG;
    }
    written = snprintf(store->snapshot_path, sizeof(store->snapshot_path),
                       "%s/snapshot.cdcstore", dir);
    if (written < 0 || (size_t)written >= sizeof(store->snapshot_path)) {
        free(store);
        return CDC_STORE_EARG;
    }
    /* A snapshot, if present, is the compaction base. It is authenticated
     * before it is trusted: a tampered snapshot fails the open rather than
     * silently seeding a wrong history. */
    {
        int found = 0;
        cdc_store_status snap = snapshot_load(store->snapshot_path, &found,
                                              &store->base_sealed,
                                              &store->base_events,
                                              store->base_state);
        if (snap != CDC_STORE_OK) {
            free(store);
            return snap;
        }
        store->has_base = found;
    }
    status = scan_log_from(store->log_path, store->base_events,
                           store->base_sealed, store->base_state, &scan);
    if (status != CDC_STORE_OK) {
        free(store);
        return status;
    }
    if (scan.state == SCAN_CORRUPT) {
        /* Committed-prefix integrity violation: fail closed, preserve the
         * evidence bytes exactly as found (review B1). */
        free(store);
        return CDC_STORE_ECORRUPT;
    }
    if (scan.state == SCAN_TAIL) {
        /* Latch-or-hold recovery: only a physically incomplete final
         * record or a valid-but-unsealed transaction tail is discarded.
         * The truncation itself is made durable before recovery is
         * reported (review secondary hardening). */
        if (truncate(store->log_path, scan.valid_bytes) != 0 ||
            sync_path(store->log_path) != 0 ||
            sync_path(store->dir) != 0) {
            free(store);
            return CDC_STORE_EIO;
        }
        if (recovered_out) {
            *recovered_out = 1;
        }
    }
    store->sealed = scan.sealed;
    store->events = scan.events;
    store->valid_bytes = scan.valid_bytes;
    *out = store;
    return CDC_STORE_OK;
}

void cdc_store_close(cdc_store *store) {
    size_t i;
    if (!store) {
        return;
    }
    for (i = 0; i < store->staged_count; i++) {
        free(store->staged[i].payload);
    }
    free(store->staged);
    free(store);
}

uint64_t cdc_store_sealed_count(const cdc_store *store) {
    return store ? store->sealed : 0;
}

cdc_store_status cdc_store_stage(cdc_store *store, const void *payload,
                                 size_t size) {
    staged_event event;
    if (!store || (!payload && size > 0)) {
        return CDC_STORE_EARG;
    }
    if (size > (size_t)CDC_STORE_MAX_RECORD) {
        return CDC_STORE_EARG; /* documented per-record bound */
    }
    if (store->staged_count == store->staged_cap) {
        size_t next = store->staged_cap ? store->staged_cap * 2 : 8;
        void *grown = realloc(store->staged, next * sizeof(staged_event));
        if (!grown) {
            return CDC_STORE_EMEM;
        }
        store->staged = grown;
        store->staged_cap = next;
    }
    event.payload = malloc(size ? size : 1);
    if (!event.payload) {
        return CDC_STORE_EMEM;
    }
    memcpy(event.payload, payload, size);
    event.size = size;
    store->staged[store->staged_count++] = event;
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_rollback(cdc_store *store) {
    size_t i;
    if (!store) {
        return CDC_STORE_EARG;
    }
    for (i = 0; i < store->staged_count; i++) {
        free(store->staged[i].payload);
    }
    store->staged_count = 0;
    return CDC_STORE_OK;
}

/* boundary-op accounting for the crash matrix: each record write is one
 * op; fflush, fsync(log), and fsync(dir) are one op each. */
int cdc_store_commit_operations(const cdc_store *store) {
    if (!store) {
        return 0;
    }
    return (int)store->staged_count + 1 /* seal */ + 3 /* flush+2 sync */;
}

static int injected_crash(cdc_store *store) {
    if (store->fail_after <= 0) {
        return 0;
    }
    store->ops_done++;
    return store->ops_done >= store->fail_after;
}

void cdc_store_set_fail_after(cdc_store *store, int operations) {
    if (store) {
        store->fail_after = operations;
        store->ops_done = 0;
    }
}

/* Writes one record; when the injected crash fires, writes only a torn
 * prefix of it (header + half the payload) to simulate a mid-write power
 * cut, then reports the crash. */
static cdc_store_status write_record(cdc_store *store, FILE *fp,
                                     uint8_t type, uint64_t seq,
                                     const uint8_t *payload,
                                     uint32_t payload_len,
                                     const uint8_t digest[CDC_DIGEST_SIZE]) {
    uint8_t header[HEADER_SIZE];
    int crash = injected_crash(store);
    memcpy(header, MAGIC, sizeof(MAGIC));
    header[4] = type;
    put_u64(header + 5, seq);
    put_u32(header + 13, payload_len);
    cdc_digest(header, FRAMING_SIZE, header + OFF_TAG);
    memcpy(header + OFF_DIGEST, digest, CDC_DIGEST_SIZE);
    if (crash) {
        size_t torn = payload_len / 2;
        fwrite(header, 1, sizeof(header), fp);
        if (torn > 0) {
            fwrite(payload, 1, torn, fp);
        }
        fflush(fp);
        return CDC_STORE_ECRASH;
    }
    if (fwrite(header, 1, sizeof(header), fp) != sizeof(header)) {
        return CDC_STORE_EIO;
    }
    if (payload_len > 0 &&
        fwrite(payload, 1, payload_len, fp) != payload_len) {
        return CDC_STORE_EIO;
    }
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_commit(cdc_store *store) {
    FILE *fp;
    size_t i;
    cdc_digest_ctx seal_ctx;
    uint8_t seal_digest[CDC_DIGEST_SIZE];
    cdc_store_status status = CDC_STORE_OK;
    int dir_fd;

    if (!store) {
        return CDC_STORE_EARG;
    }
    if (store->staged_count == 0) {
        return CDC_STORE_ESTATE;
    }
    /* Compare-and-set: if this writer armed a fence, the on-disk sealed
     * count must still be what it fenced at. Another writer having moved
     * it means this writer's view is stale, so the commit is refused
     * before a single byte is written. */
    if (store->fence_armed) {
        scan_result fence_scan;
        if (scan_log_from(store->log_path, store->base_events,
                          store->base_sealed, store->base_state,
                          &fence_scan) != CDC_STORE_OK) {
            return CDC_STORE_EIO;
        }
        if (fence_scan.state == SCAN_CORRUPT) {
            return CDC_STORE_ECORRUPT;
        }
        if (fence_scan.sealed != store->fence_seal) {
            store->fence_armed = 0;
            return CDC_STORE_ESTATE; /* stale writer */
        }
    }
    fp = fopen(store->log_path, "ab");
    if (!fp) {
        return CDC_STORE_EIO;
    }
    cdc_digest_init(&seal_ctx);
    for (i = 0; i < store->staged_count && status == CDC_STORE_OK; i++) {
        uint8_t digest[CDC_DIGEST_SIZE];
        cdc_digest(store->staged[i].payload, store->staged[i].size, digest);
        cdc_digest_update(&seal_ctx, digest, sizeof(digest));
        status = write_record(store, fp, 'D', store->events + i + 1,
                              store->staged[i].payload,
                              (uint32_t)store->staged[i].size, digest);
    }
    if (status == CDC_STORE_OK) {
        cdc_digest_final(&seal_ctx, seal_digest);
        status = write_record(store, fp, 'S', store->sealed + 1, NULL, 0,
                              seal_digest);
    }
    if (status == CDC_STORE_OK) {
        if (injected_crash(store)) {
            status = CDC_STORE_ECRASH; /* after writes, before flush */
        } else if (fflush(fp) != 0) {
            status = CDC_STORE_EIO;
        }
    }
    if (status == CDC_STORE_OK) {
        if (injected_crash(store)) {
            status = CDC_STORE_ECRASH; /* after flush, before fsync */
        } else if (fsync(fileno(fp)) != 0) {
            status = CDC_STORE_EIO;
        }
    }
    fclose(fp);
    if (status == CDC_STORE_OK) {
        if (injected_crash(store)) {
            status = CDC_STORE_ECRASH; /* after log sync, before dir sync */
        } else {
            dir_fd = open(store->dir, O_RDONLY);
            if (dir_fd < 0 || fsync(dir_fd) != 0) {
                if (dir_fd >= 0) {
                    close(dir_fd);
                }
                status = CDC_STORE_EIO;
            } else {
                close(dir_fd);
            }
        }
    }
    if (status == CDC_STORE_OK) {
        store->events += store->staged_count;
        store->sealed += 1;
        if (store->fence_armed) {
            store->fence_seal = store->sealed;
        }
        store->valid_bytes = -1; /* recomputed on next open/verify */
        cdc_store_rollback(store);
        return CDC_STORE_OK;
    }
    /* crash or error: staged events remain staged; the on-disk tail (if
     * any) is unsealed and will be truncated by recovery. */
    return status;
}

cdc_store_status cdc_store_replay(cdc_store *store, char *out,
                                  size_t out_size) {
    scan_result scan;

    if (!store || !out) {
        return CDC_STORE_EARG;
    }
    /* Replay identity is the chained state the scan produced, resumed from
     * the compaction base. It therefore depends on the sealed history, not
     * on how that history is currently laid out on disk — which is exactly
     * why compaction preserves it while the attest (raw-bytes) digest
     * legitimately changes. */
    if (scan_log_from(store->log_path, store->base_events,
                      store->base_sealed, store->base_state,
                      &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT; /* corrupt evidence is never replayed */
    }
    cdc_digest_hex(scan.replay_state, out, out_size);
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_attest(cdc_store *store, char *out,
                                  size_t out_size) {
    FILE *fp;
    cdc_digest_ctx ctx;
    uint8_t digest[CDC_DIGEST_SIZE];
    uint8_t buffer[4096];
    scan_result scan;
    long remaining;

    if (!store || !out) {
        return CDC_STORE_EARG;
    }
    if (scan_log_from(store->log_path, store->base_events,
                      store->base_sealed, store->base_state,
                      &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT; /* corrupt evidence is never attested */
    }
    cdc_digest_init(&ctx);
    fp = fopen(store->log_path, "rb");
    remaining = scan.valid_bytes;
    while (fp && remaining > 0) {
        size_t take = remaining > (long)sizeof(buffer) ? sizeof(buffer)
                                                       : (size_t)remaining;
        if (fread(buffer, 1, take, fp) != take) {
            fclose(fp);
            return CDC_STORE_EIO;
        }
        cdc_digest_update(&ctx, buffer, take);
        remaining -= (long)take;
    }
    if (fp) {
        fclose(fp);
    }
    cdc_digest_final(&ctx, digest);
    cdc_digest_hex(digest, out, out_size);
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_verify(cdc_store *store) {
    scan_result scan;
    if (!store) {
        return CDC_STORE_EARG;
    }
    if (scan_log_from(store->log_path, store->base_events,
                      store->base_sealed, store->base_state,
                      &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT;
    }
    if (scan.state == SCAN_TAIL) {
        return CDC_STORE_EUNSEALED;
    }
    return CDC_STORE_OK;
}

/* ---- snapshot / compact / fence ------------------------------------- */

/* Reads and authenticates the snapshot, if one exists. Returns
 * CDC_STORE_OK with *found=0 when absent; ECORRUPT when present but
 * tampered — a bad snapshot must never resurrect a wrong base. */
static cdc_store_status snapshot_load(const char *path, int *found,
                                      uint64_t *sealed, uint64_t *events,
                                      uint8_t state[CDC_DIGEST_SIZE]) {
    FILE *fp = fopen(path, "rb");
    uint8_t buffer[SNAP_SIZE];
    uint8_t tag[CDC_DIGEST_SIZE];
    size_t got;

    *found = 0;
    if (!fp) {
        return errno == ENOENT ? CDC_STORE_OK : CDC_STORE_EIO;
    }
    got = fread(buffer, 1, sizeof(buffer), fp);
    if (got != sizeof(buffer)) {
        int faulted = ferror(fp);
        fclose(fp);
        return faulted ? CDC_STORE_EIO : CDC_STORE_ECORRUPT;
    }
    /* reject trailing bytes: a snapshot is exactly one record */
    if (fgetc(fp) != EOF) {
        fclose(fp);
        return CDC_STORE_ECORRUPT;
    }
    fclose(fp);
    if (memcmp(buffer, SNAP_MAGIC, sizeof(SNAP_MAGIC)) != 0 ||
        buffer[4] != 1) {
        return CDC_STORE_ECORRUPT;
    }
    cdc_digest(buffer, SNAP_BODY, tag);
    if (memcmp(tag, buffer + SNAP_BODY, CDC_DIGEST_SIZE) != 0) {
        return CDC_STORE_ECORRUPT;
    }
    *sealed = get_u64(buffer + 5);
    *events = get_u64(buffer + 13);
    memcpy(state, buffer + 21, CDC_DIGEST_SIZE);
    *found = 1;
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_snapshot(cdc_store *store) {
    uint8_t buffer[SNAP_SIZE];
    char temp_path[640];
    FILE *fp;
    scan_result scan;
    int written;

    if (!store) {
        return CDC_STORE_EARG;
    }
    if (scan_log_from(store->log_path, store->base_events, store->base_sealed,
                      store->base_state, &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT;
    }

    memcpy(buffer, SNAP_MAGIC, sizeof(SNAP_MAGIC));
    buffer[4] = 1;
    put_u64(buffer + 5, scan.sealed);
    put_u64(buffer + 13, scan.events);
    memcpy(buffer + 21, scan.replay_state, CDC_DIGEST_SIZE);
    cdc_digest(buffer, SNAP_BODY, buffer + SNAP_BODY);

    written = snprintf(temp_path, sizeof(temp_path), "%s.tmp",
                       store->snapshot_path);
    if (written < 0 || (size_t)written >= sizeof(temp_path)) {
        return CDC_STORE_EARG;
    }
    fp = fopen(temp_path, "wb");
    if (!fp) {
        return CDC_STORE_EIO;
    }
    if (fwrite(buffer, 1, sizeof(buffer), fp) != sizeof(buffer) ||
        fflush(fp) != 0 || fsync(fileno(fp)) != 0) {
        fclose(fp);
        return CDC_STORE_EIO;
    }
    fclose(fp);
    /* activation is atomic, then the directory entry is made durable */
    if (rename(temp_path, store->snapshot_path) != 0 ||
        sync_path(store->dir) != 0) {
        return CDC_STORE_EIO;
    }
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_compact(cdc_store *store) {
    int found = 0;
    uint64_t snap_sealed = 0, snap_events = 0;
    uint8_t snap_state[CDC_DIGEST_SIZE];
    cdc_store_status status;
    scan_result scan;

    if (!store) {
        return CDC_STORE_EARG;
    }
    status = snapshot_load(store->snapshot_path, &found, &snap_sealed,
                           &snap_events, snap_state);
    if (status != CDC_STORE_OK) {
        return status;
    }
    if (!found) {
        return CDC_STORE_ESTATE; /* nothing to compact against */
    }
    if (scan_log_from(store->log_path, store->base_events, store->base_sealed,
                      store->base_state, &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT;
    }
    /* Refuse unless the snapshot covers the entire sealed prefix: compaction
     * may never discard a record the base does not already account for. */
    if (snap_sealed != scan.sealed || snap_events != scan.events ||
        memcmp(snap_state, scan.replay_state, CDC_DIGEST_SIZE) != 0) {
        return CDC_STORE_ESTATE;
    }
    if (truncate(store->log_path, 0) != 0 ||
        sync_path(store->log_path) != 0 || sync_path(store->dir) != 0) {
        return CDC_STORE_EIO;
    }
    store->has_base = 1;
    store->base_sealed = snap_sealed;
    store->base_events = snap_events;
    memcpy(store->base_state, snap_state, CDC_DIGEST_SIZE);
    store->sealed = snap_sealed;
    store->events = snap_events;
    store->valid_bytes = 0;
    return CDC_STORE_OK;
}

cdc_store_status cdc_store_fence(cdc_store *store, uint64_t expected_seal) {
    scan_result scan;

    if (!store) {
        return CDC_STORE_EARG;
    }
    if (scan_log_from(store->log_path, store->base_events, store->base_sealed,
                      store->base_state, &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT;
    }
    if (scan.sealed != expected_seal) {
        store->fence_armed = 0;
        return CDC_STORE_ESTATE; /* the writer's view is already stale */
    }
    store->fence_armed = 1;
    store->fence_seal = expected_seal;
    return CDC_STORE_OK;
}

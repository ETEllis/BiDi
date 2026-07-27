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
    uint64_t sealed;      /* sealed transaction count */
    uint64_t events;      /* events in sealed transactions */
    long valid_bytes;     /* log byte length covering sealed state */
    staged_event *staged;
    size_t staged_count;
    size_t staged_cap;
    int fail_after;       /* injection: abort after N boundary ops */
    int ops_done;
};

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
} scan_result;

static cdc_store_status scan_log(const char *path, scan_result *result) {
    FILE *fp = fopen(path, "rb");
    long offset = 0;
    uint64_t expect_event = 1;
    uint64_t expect_seal = 1;
    cdc_digest_ctx txn_ctx;
    int txn_open = 0;

    memset(result, 0, sizeof(*result));
    result->state = SCAN_CLEAN;
    if (!fp) {
        return errno == ENOENT ? CDC_STORE_OK : CDC_STORE_EIO;
    }
    for (;;) {
        uint8_t header[HEADER_SIZE];
        size_t got = fread(header, 1, sizeof(header), fp);
        uint32_t payload_len;
        uint64_t seq;
        uint8_t type;
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
            read_len = fread(payload, 1, payload_len, fp);
            if (read_len != payload_len) {
                free(payload);
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
    status = scan_log(store->log_path, &scan);
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
    FILE *fp;
    cdc_digest_ctx state_ctx;
    uint8_t state_digest[CDC_DIGEST_SIZE];
    scan_result scan;
    long valid;
    long offset = 0;

    if (!store || !out) {
        return CDC_STORE_EARG;
    }
    if (scan_log(store->log_path, &scan) != CDC_STORE_OK) {
        return CDC_STORE_EIO;
    }
    if (scan.state == SCAN_CORRUPT) {
        return CDC_STORE_ECORRUPT; /* corrupt evidence is never replayed */
    }
    valid = scan.valid_bytes;
    fp = fopen(store->log_path, "rb");
    cdc_digest_init(&state_ctx);
    if (fp) {
        while (offset < valid) {
            uint8_t header[HEADER_SIZE];
            uint32_t payload_len;
            if (fread(header, 1, sizeof(header), fp) != sizeof(header)) {
                fclose(fp);
                return CDC_STORE_EIO;
            }
            payload_len = get_u32(header + 13);
            /* fold every record digest (data and seal) in order */
            cdc_digest_update(&state_ctx, header + OFF_DIGEST,
                              CDC_DIGEST_SIZE);
            if (payload_len > 0) {
                fseek(fp, payload_len, SEEK_CUR);
            }
            offset += HEADER_SIZE + (long)payload_len;
        }
        fclose(fp);
    }
    cdc_digest_final(&state_ctx, state_digest);
    cdc_digest_hex(state_digest, out, out_size);
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
    if (scan_log(store->log_path, &scan) != CDC_STORE_OK) {
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
    if (scan_log(store->log_path, &scan) != CDC_STORE_OK) {
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

cdc_store_status cdc_store_snapshot(cdc_store *store) {
    (void)store;
    return CDC_STORE_EUNSUPPORTED;
}

cdc_store_status cdc_store_compact(cdc_store *store) {
    (void)store;
    return CDC_STORE_EUNSUPPORTED;
}

cdc_store_status cdc_store_fence(cdc_store *store, uint64_t expected_seal) {
    (void)store;
    (void)expected_seal;
    return CDC_STORE_EUNSUPPORTED;
}

#define _POSIX_C_SOURCE 200809L

#include "../../runtime/cdc_store.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    const char *const *payloads;
    const uint64_t *transactions;
    size_t expected;
    size_t visited;
    int mismatch;
} visit_probe;

static cdc_store_status expect_event(void *context, uint64_t event_sequence,
                                     uint64_t transaction_sequence,
                                     const void *payload,
                                     size_t payload_size) {
    visit_probe *probe = context;
    size_t index = probe->visited;
    size_t expected_size;

    if (index >= probe->expected) {
        probe->mismatch = 1;
        return CDC_STORE_ESTATE;
    }
    expected_size = strlen(probe->payloads[index]);
    if (event_sequence != index + 1 ||
        transaction_sequence != probe->transactions[index] ||
        payload_size != expected_size ||
        memcmp(payload, probe->payloads[index], payload_size) != 0) {
        probe->mismatch = 1;
        return CDC_STORE_ESTATE;
    }
    probe->visited++;
    return CDC_STORE_OK;
}

static cdc_store_status count_event(void *context, uint64_t event_sequence,
                                    uint64_t transaction_sequence,
                                    const void *payload,
                                    size_t payload_size) {
    size_t *count = context;
    (void)event_sequence;
    (void)transaction_sequence;
    (void)payload;
    (void)payload_size;
    (*count)++;
    return CDC_STORE_OK;
}

static int commit_payloads(cdc_store *store, const char *const *payloads,
                           size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        if (cdc_store_stage(store, payloads[i], strlen(payloads[i])) !=
            CDC_STORE_OK) {
            return 0;
        }
    }
    return cdc_store_commit(store) == CDC_STORE_OK;
}

static int flip_payload_byte(const char *dir, const char *needle) {
    char path[768];
    FILE *fp;
    uint8_t *bytes;
    long length;
    size_t needle_size = strlen(needle);
    size_t i;
    int found = 0;

    if (snprintf(path, sizeof(path), "%s/log.cdcstore", dir) < 0) {
        return 0;
    }
    fp = fopen(path, "r+b");
    if (!fp) {
        return 0;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (length = ftell(fp)) < 0 ||
        fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }
    bytes = malloc(length > 0 ? (size_t)length : 1);
    if (!bytes) {
        fclose(fp);
        return 0;
    }
    if (fread(bytes, 1, (size_t)length, fp) != (size_t)length) {
        free(bytes);
        fclose(fp);
        return 0;
    }
    for (i = 0; i + needle_size <= (size_t)length; i++) {
        if (memcmp(bytes + i, needle, needle_size) == 0) {
            bytes[i] ^= 0x01u;
            if (fseek(fp, (long)i, SEEK_SET) == 0 &&
                fwrite(bytes + i, 1, 1, fp) == 1 && fflush(fp) == 0) {
                found = 1;
            }
            break;
        }
    }
    free(bytes);
    fclose(fp);
    return found;
}

static void cleanup_store(const char *dir) {
    char lock_path[768];
    (void)cdc_store_reset(dir);
    if (snprintf(lock_path, sizeof(lock_path), "%s/lock.cdcstore", dir) >=
        0) {
        (void)unlink(lock_path);
    }
    (void)rmdir(dir);
}

static int create_temp_dir(char *dir, size_t dir_size) {
    unsigned int attempt;
    for (attempt = 0; attempt < 1000; attempt++) {
        int written =
            snprintf(dir, dir_size, "/tmp/cdc-store-replay-api-%ld-%u",
                     (long)getpid(), attempt);
        if (written < 0 || (size_t)written >= dir_size) {
            return 0;
        }
        if (mkdir(dir, 0700) == 0) {
            return 1;
        }
        if (errno != EEXIST) {
            return 0;
        }
    }
    errno = EEXIST;
    return 0;
}

static int fail(const char *dir, cdc_store *store, const char *message) {
    fprintf(stderr, "cdc-store-replay-api FAIL: %s\n", message);
    cdc_store_close(store);
    cleanup_store(dir);
    return 1;
}

int main(void) {
    char dir[128];
    static const char *const txn_one[] = {"shared-alpha", "shared-beta"};
    static const char *const txn_two[] = {"shared-gamma"};
    static const char *const expected[] = {
        "shared-alpha", "shared-beta", "shared-gamma"};
    static const uint64_t transactions[] = {1, 1, 2};
    static const char *const unsealed[] = {
        "must-not-appear", "torn-payload"};
    cdc_store *store = NULL;
    cdc_store *reopened = NULL;
    visit_probe probe;
    cdc_store_status status;
    int recovered = -1;
    size_t corrupt_callbacks = 0;

    if (!create_temp_dir(dir, sizeof(dir))) {
        fprintf(stderr, "cdc-store-replay-api FAIL: temporary directory: %s\n",
                strerror(errno));
        return 1;
    }
    if (cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
        recovered != 0 || !commit_payloads(store, txn_one, 2) ||
        !commit_payloads(store, txn_two, 1)) {
        return fail(dir, store, "create and seal shared records");
    }

    memset(&probe, 0, sizeof(probe));
    probe.payloads = expected;
    probe.transactions = transactions;
    probe.expected = 3;
    status = cdc_store_visit_events(store, expect_event, &probe);
    if (status != CDC_STORE_OK || probe.mismatch || probe.visited != 3) {
        return fail(dir, store, "sealed order and payloads");
    }

    cdc_store_close(store);
    store = NULL;
    recovered = -1;
    if (cdc_store_open(dir, &reopened, &recovered) != CDC_STORE_OK ||
        recovered != 0) {
        return fail(dir, reopened, "clean reopen");
    }
    memset(&probe, 0, sizeof(probe));
    probe.payloads = expected;
    probe.transactions = transactions;
    probe.expected = 3;
    if (cdc_store_visit_events(reopened, expect_event, &probe) !=
            CDC_STORE_OK ||
        probe.mismatch || probe.visited != 3) {
        return fail(dir, reopened, "payload recovery after reopen");
    }

    if (cdc_store_stage(reopened, unsealed[0], strlen(unsealed[0])) !=
            CDC_STORE_OK ||
        cdc_store_stage(reopened, unsealed[1], strlen(unsealed[1])) !=
            CDC_STORE_OK) {
        return fail(dir, reopened, "stage unsealed control");
    }
    /* Record 1 is complete; record 2 is torn. Neither has a SEAL, so neither
     * may cross the visitor boundary. */
    cdc_store_set_fail_after(reopened, 2);
    if (cdc_store_commit(reopened) != CDC_STORE_ECRASH) {
        return fail(dir, reopened, "create unsealed tail");
    }
    cdc_store_set_fail_after(reopened, 0);
    (void)cdc_store_rollback(reopened);
    memset(&probe, 0, sizeof(probe));
    probe.payloads = expected;
    probe.transactions = transactions;
    probe.expected = 3;
    if (cdc_store_visit_events(reopened, expect_event, &probe) !=
            CDC_STORE_OK ||
        probe.mismatch || probe.visited != 3) {
        return fail(dir, reopened, "unsealed tail was exposed");
    }

    cdc_store_close(reopened);
    reopened = NULL;
    recovered = -1;
    if (cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
        recovered != 1) {
        return fail(dir, store, "unsealed-tail recovery on reopen");
    }
    memset(&probe, 0, sizeof(probe));
    probe.payloads = expected;
    probe.transactions = transactions;
    probe.expected = 3;
    if (cdc_store_visit_events(store, expect_event, &probe) != CDC_STORE_OK ||
        probe.mismatch || probe.visited != 3) {
        return fail(dir, store, "sealed payloads after tail recovery");
    }

    if (!flip_payload_byte(dir, expected[0])) {
        return fail(dir, store, "inject sealed payload corruption");
    }
    status = cdc_store_visit_events(store, count_event, &corrupt_callbacks);
    if (status != CDC_STORE_ECORRUPT || corrupt_callbacks != 0) {
        return fail(dir, store, "corrupt history crossed visitor boundary");
    }
    cdc_store_close(store);
    store = NULL;
    recovered = -1;
    status = cdc_store_open(dir, &reopened, &recovered);
    if (status != CDC_STORE_ECORRUPT || reopened != NULL || recovered != 0) {
        return fail(dir, reopened, "corrupt reopen did not fail closed");
    }

    cleanup_store(dir);
    puts("cdc-store-replay-api PASS: sealed=3 order=exact reopen=exact "
         "unsealed=hidden corruption=rejected callbacks=0");
    return 0;
}

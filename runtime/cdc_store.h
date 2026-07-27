#ifndef CDC_STORE_H
#define CDC_STORE_H

#include <stddef.h>
#include <stdint.h>

#include "cdc_digest.h"

/* cdc_store — the generic durable, replayable state substrate (Phase D,
 * gates CT4/MM1 seed). Protocol verbs (amendment Phase D.1):
 *
 *   append replay snapshot rebuild transaction compare-and-set/fence
 *   commit rollback recover compact attest verify
 *
 * This header carries the whole protocol; the reference backend implements
 * the event-log core (append/transaction/commit/rollback/replay/recover/
 * attest/verify). Snapshot, compaction, and the CAS/fence verb are
 * declared and fail closed with CDC_STORE_EUNSUPPORTED until their gate
 * (recorded in BUILD_STATE) — never silently absent.
 *
 * Durability contract of the reference backend:
 * - The log is append-only: DATA records followed by a SEAL record per
 *   transaction. Replay counts only sealed transactions; recovery
 *   truncates any unsealed or torn tail, so after a crash at ANY write
 *   boundary exactly the old (pre-transaction) or new (sealed) state is
 *   visible — never a partial batch (A10/A4 semantics: latch-or-hold).
 * - Every record carries its payload digest (interim sha256 per D2);
 *   torn or corrupted tails are detected by length/digest/marker checks.
 * - Commit path: write(all records) -> fflush -> fsync(log fd) ->
 *   fsync(directory fd). The injectable failure hook aborts at each
 *   boundary to prove recovery (cdc_store_set_fail_after).
 * - Single writer per store directory; no locks are taken (the caller is
 *   the concurrency boundary at this stage).
 */

typedef struct cdc_store cdc_store;

typedef enum {
    CDC_STORE_OK = 0,
    CDC_STORE_EARG = 1,
    CDC_STORE_EIO = 2,
    CDC_STORE_EMEM = 3,
    CDC_STORE_ECORRUPT = 4,     /* recovery found and truncated a torn tail */
    CDC_STORE_ECRASH = 5,       /* injected failure fired (test harness) */
    CDC_STORE_ESTATE = 6,
    CDC_STORE_EUNSUPPORTED = 7, /* declared verb not yet landed */
} cdc_store_status;

const char *cdc_store_status_name(cdc_store_status status);

/* Opens (creating if needed) the store rooted at `dir`, running recovery:
 * any unsealed or torn tail is truncated and reported via *recovered_out
 * (0 = clean, 1 = a tail was truncated). */
cdc_store_status cdc_store_open(const char *dir, cdc_store **out,
                                int *recovered_out);
void cdc_store_close(cdc_store *store);

/* Number of sealed transactions visible. */
uint64_t cdc_store_sealed_count(const cdc_store *store);

/* Transaction: stage any number of event payloads, then commit (all
 * sealed atomically) or rollback (nothing written). Staging is in-memory;
 * nothing touches the log before commit. */
cdc_store_status cdc_store_stage(cdc_store *store, const void *payload,
                                 size_t size);
cdc_store_status cdc_store_commit(cdc_store *store);
cdc_store_status cdc_store_rollback(cdc_store *store);

/* Replay: folds every sealed event's digest into a deterministic state
 * digest ("sha256:<hex>" written to out). Identical event history yields
 * an identical state digest on any platform. */
cdc_store_status cdc_store_replay(cdc_store *store, char *out,
                                  size_t out_size);

/* Attest: digest of the raw sealed log bytes (evidence identity). */
cdc_store_status cdc_store_attest(cdc_store *store, char *out,
                                  size_t out_size);

/* Verify: full structural re-scan of the log (lengths, digests, seals). */
cdc_store_status cdc_store_verify(cdc_store *store);

/* Declared, not yet landed (fail closed): */
cdc_store_status cdc_store_snapshot(cdc_store *store);
cdc_store_status cdc_store_compact(cdc_store *store);
cdc_store_status cdc_store_fence(cdc_store *store, uint64_t expected_seal);

/* Failure injection (crash matrix): abort the commit path after N
 * successful write/flush/sync operations (0 disables). The abort leaves
 * whatever bytes were written — including torn partial records — for
 * recovery to prove the latch-or-hold contract. */
void cdc_store_set_fail_after(cdc_store *store, int operations);

/* Number of write/flush/sync boundary operations a commit of the current
 * staged transaction would perform (for exhaustive injection sweeps). */
int cdc_store_commit_operations(const cdc_store *store);

#endif

#ifndef CDC_RECEIPT_H
#define CDC_RECEIPT_H

#include <stddef.h>
#include <stdint.h>

/* cdc_receipt — typed effect receipts (gate CT3, amendment A7/§7).
 *
 * Why this exists. `cdc test` used to classify every job's outcome by
 * string-matching the runtime's HUMAN report line: strstr(line,
 * "status=held"), then splitting "<form>=<jobid>" out of the prose. That
 * made the report format load-bearing for the gate — rewording a line, or
 * a payload that happened to contain "status=accepted", could silently
 * change a verdict. Effects were being inferred from prose rather than
 * reported.
 *
 * A receipt is the typed record of one executed effect, built by the SAME
 * code that produces the outcome, before the prose is printed. Both the
 * report line and the receipt are rendered from this struct, so they cannot
 * describe different things — and scripts/verify.sh gates that agreement
 * (receipt/prose parity), so a divergence fails the build rather than
 * quietly winning.
 *
 * Carrier. Receipts are written to the path named by the CDC_RECEIPTS
 * environment variable, one canonical record per line, in execution order.
 * Nothing is written when the variable is unset, so the human surface is
 * unchanged for interactive use. The receipt stream is never mixed into
 * stdout: prose and receipts are separate channels by construction.
 *
 * Record grammar (version 1), fixed field order, no optional reordering:
 *
 *   cdc-receipt v=1 kind=<k> job=<id> op=<op> outcome=<+1|0|-1>
 *     reason=<typed> declared-hold=<0|1> [effect fields...]
 *
 * Values are bare tokens from a closed vocabulary: no quoting, no spaces,
 * no escaping, because every field is either an identifier, an integer, or
 * a member of a fixed set. A value that would violate that is a
 * programming error and is rejected at emit time rather than encoded.
 */

#define CDC_RECEIPT_VERSION 1

/* Balanced-ternary outcome. The carrier is the calculus's own: 0 is
 * EQUILIBRIUM (a hold), never "false" and never "failure". A run that
 * could not produce an outcome at all is CDC_OUTCOME_NONE, which is not a
 * ternary value and never appears in a receipt. */
typedef enum {
    CDC_OUTCOME_NONE = 2,
    CDC_OUTCOME_VIOLATED = -1,
    CDC_OUTCOME_HELD = 0,
    CDC_OUTCOME_ACCEPTED = 1
} cdc_outcome;

/* Effect fields that do not apply to a given kind are CDC_RECEIPT_NA and
 * are omitted from the record entirely rather than encoded as zero. */
#define CDC_RECEIPT_NA (-1)

typedef struct {
    int version;
    char kind[16];   /* commit | nest | persist | universal */
    char job[64];    /* the declaring statement's first argument */
    char op[16];     /* persist verb, or "" */
    cdc_outcome outcome;
    char reason[40]; /* typed hold reason; "none" when accepted */
    int declared_hold; /* the source declared expect-status=held on this job */

    /* Durable-effect observations (persist). -1 = not applicable. */
    int durable;       /* sealed log bytes changed */
    int replay_stable; /* semantic replay identity unchanged */
    long sealed;
    long events;
    long generation;

    /* Barrier detail (commit and persist op=append). */
    char trits[128];  /* sized to the runtime's CommitResult */
    char balance[32]; /* admissible | violated | "" */
} cdc_receipt;

/* Zeroes the receipt and sets every non-applicable field to CDC_RECEIPT_NA
 * and outcome to CDC_OUTCOME_NONE. */
void cdc_receipt_init(cdc_receipt *receipt);

/* Opens the receipt stream named by the CDC_RECEIPTS environment variable
 * (truncating), or returns NULL when the variable is unset or empty.
 * Ownership passes to the caller; close with cdc_receipt_close. */
void *cdc_receipt_open_env(void);
void cdc_receipt_close(void *stream);

/* Writes one canonical record. Returns 0 when `stream` is NULL (receipts
 * disabled), 1 on success, -1 when a field violates the token vocabulary
 * — which is a programming error, never something a source file can
 * trigger. */
int cdc_receipt_emit(void *stream, const cdc_receipt *receipt);

/* Parses one canonical record. Returns 1 on success, 0 when the line is
 * not a receipt (blank, comment, or foreign), -1 when it claims to be a
 * receipt but is malformed or carries an unknown version — malformed
 * receipts fail closed rather than being skipped as noise. */
int cdc_receipt_parse(const char *line, cdc_receipt *receipt);

/* Vocabulary helpers shared by producer and consumer. */
const char *cdc_outcome_token(cdc_outcome outcome);
cdc_outcome cdc_outcome_from_token(const char *token);

#endif

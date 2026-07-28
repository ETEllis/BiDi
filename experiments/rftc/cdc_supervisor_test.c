#include "../../runtime/cdc_abi.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "supervisor check failed at %s:%d: %s\n",         \
                    __FILE__, __LINE__, #expr);                                \
            return 0;                                                          \
        }                                                                      \
    } while (0)

static const uint8_t KEY[CDC_TRANSPORT_TAG_SIZE] = {
    0x91, 0x48, 0x37, 0xaa, 0x11, 0x05, 0x77, 0xc2,
    0x4d, 0x99, 0x61, 0xe0, 0x31, 0x52, 0x18, 0xb7,
    0xd1, 0x0c, 0xa3, 0x4f, 0x8e, 0x29, 0x66, 0x73,
    0x01, 0xf4, 0xbb, 0x5d, 0xc8, 0x42, 0x90, 0x2a,
};

typedef struct {
    unsigned commits;
    int fail;
} commit_state;

typedef struct {
    cdc_supervisor_application_verdict next;
    unsigned calls;
    unsigned mutations;
} typed_apply_state;

typedef struct {
    cdc_supervisor *supervisor;
    const cdc_transport_envelope *envelope;
    commit_state *commit;
    cdc_supervisor_verdict verdict;
} thread_admission;

static void set_id(char *out, const char *value) {
    memcpy(out, value, strlen(value) + 1);
}

static void make_nonce(uint8_t nonce[CDC_TRANSPORT_NONCE_SIZE],
                       uint8_t seed) {
    size_t i;
    for (i = 0; i < CDC_TRANSPORT_NONCE_SIZE; i++) {
        nonce[i] = (uint8_t)(seed + i);
    }
}

static int make_proposal(cdc_transport_envelope *envelope,
                         const char *sender, const char *recipient,
                         uint64_t sequence, const uint8_t *parent,
                         uint8_t nonce_seed, uint64_t horizon,
                         const char *payload) {
    cdc_transport_envelope_init(envelope);
    envelope->schema_version = 1;
    envelope->message_type = CDC_TRANSPORT_PROPOSAL;
    set_id(envelope->sender, sender);
    set_id(envelope->recipient, recipient);
    set_id(envelope->frame, "supervised-frame");
    set_id(envelope->key_id, "supervisor-key");
    set_id(envelope->lease_id, "supervisor-lease");
    envelope->authority_action = CDC_AUTH_PROPOSE;
    envelope->horizon = horizon;
    envelope->sequence = sequence;
    envelope->logical_clock = 500 + sequence;
    make_nonce(envelope->nonce, nonce_seed);
    if (parent) {
        memcpy(envelope->causal_parent, parent, CDC_TRANSPORT_TAG_SIZE);
    }
    return cdc_transport_envelope_set_payload(envelope, payload,
                                              strlen(payload)) &&
           cdc_transport_envelope_sign(envelope, KEY);
}

static int commit_once(const cdc_transport_envelope *envelope,
                       const cdc_supervisor_receipt *receipt, void *context) {
    commit_state *state = context;
    if (state->fail) {
        return 0;
    }
    if (memcmp(receipt->proposal_digest, envelope->payload_digest,
               CDC_TRANSPORT_TAG_SIZE) != 0 ||
        memcmp(receipt->envelope_digest, envelope->envelope_digest,
               CDC_TRANSPORT_TAG_SIZE) != 0) {
        return 0;
    }
    state->commits++;
    return 1;
}

static cdc_supervisor_application_verdict
apply_typed(const cdc_transport_envelope *envelope,
            const cdc_supervisor_receipt *receipt, void *context) {
    typed_apply_state *state = context;
    if (memcmp(receipt->proposal_digest, envelope->payload_digest,
               CDC_TRANSPORT_TAG_SIZE) != 0) {
        return CDC_SUPERVISOR_APPLICATION_HOLD;
    }
    state->calls++;
    if (state->next == CDC_SUPERVISOR_APPLICATION_ACCEPT) {
        state->mutations++;
    }
    return state->next;
}

static cdc_supervisor *make_supervisor(size_t stream_limit,
                                       size_t nonce_limit) {
    cdc_supervisor_config config;
    memset(&config, 0, sizeof(config));
    config.schema_version = 1;
    config.local_id = "supervisor";
    config.key_id = "supervisor-key";
    config.key = KEY;
    config.payload_limit = 4096;
    config.stream_limit = stream_limit;
    config.lease_limit = 4;
    config.nonce_limit = nonce_limit;
    return cdc_supervisor_create(&config);
}

static int add_lease(cdc_supervisor *supervisor) {
    cdc_authority_lease lease;
    memset(&lease, 0, sizeof(lease));
    lease.version = 1;
    set_id(lease.lease_id, "supervisor-lease");
    set_id(lease.subject, "node-a");
    set_id(lease.frame, "supervised-frame");
    lease.actions = CDC_AUTH_PROPOSE;
    lease.horizon_start = 10;
    lease.horizon_end = 20;
    lease.not_before = 100;
    lease.expires_at = 200;
    lease.quorum_required = 2;
    lease.quorum_total = 3;
    return cdc_supervisor_add_lease(supervisor, &lease) ==
           CDC_AUTHORITY_ACCEPT;
}

static void *admit_thread(void *opaque) {
    thread_admission *args = opaque;
    cdc_supervisor_receipt receipt;
    args->verdict = cdc_supervisor_admit(
        args->supervisor, args->envelope, 150, 2, commit_once, args->commit,
        &receipt);
    return NULL;
}

static int test_serialized_admission(void) {
    cdc_supervisor *supervisor = make_supervisor(4, 16);
    cdc_transport_envelope first, second, third, wrong_recipient, wrong_subject,
        wrong_horizon, regressed_clock;
    cdc_supervisor_receipt receipt;
    commit_state state = {0, 0};
    thread_admission a, b;
    pthread_t thread_a, thread_b;

    CHECK(supervisor != NULL && add_lease(supervisor));
    CHECK(make_proposal(&wrong_recipient, "node-a", "other-supervisor", 1,
                        NULL, 1, 12, "wrong-recipient"));
    CHECK(cdc_supervisor_admit(supervisor, &wrong_recipient, 150, 2,
                               commit_once, &state, &receipt) ==
          CDC_SUPERVISOR_REJECT_TRANSPORT);
    CHECK(receipt.transport_verdict == CDC_TRANSPORT_REJECT_RECIPIENT);

    CHECK(make_proposal(&wrong_subject, "node-b", "supervisor", 1, NULL, 2,
                        12, "wrong-subject"));
    CHECK(cdc_supervisor_admit(supervisor, &wrong_subject, 150, 2, commit_once,
                               &state, &receipt) ==
          CDC_SUPERVISOR_REJECT_AUTHORITY);
    CHECK(receipt.authority_verdict == CDC_AUTHORITY_REJECT_SUBJECT);

    CHECK(make_proposal(&wrong_horizon, "node-a", "supervisor", 1, NULL, 3,
                        21, "wrong-horizon"));
    CHECK(cdc_supervisor_admit(supervisor, &wrong_horizon, 150, 2, commit_once,
                               &state, &receipt) ==
          CDC_SUPERVISOR_REJECT_AUTHORITY);
    CHECK(receipt.authority_verdict == CDC_AUTHORITY_REJECT_HORIZON);
    CHECK(state.commits == 0);

    CHECK(make_proposal(&first, "node-a", "supervisor", 1, NULL, 4, 12,
                        "proposal-one"));
    first.horizon = 13;
    CHECK(cdc_supervisor_admit(supervisor, &first, 150, 2, commit_once, &state,
                               &receipt) ==
          CDC_SUPERVISOR_REJECT_TRANSPORT);
    CHECK(receipt.transport_verdict == CDC_TRANSPORT_REJECT_AUTH);
    first.horizon = 12;
    CHECK(cdc_supervisor_admit(supervisor, &first, 150, 1, commit_once, &state,
                               &receipt) ==
          CDC_SUPERVISOR_REJECT_AUTHORITY);
    CHECK(receipt.authority_verdict == CDC_AUTHORITY_REJECT_QUORUM);
    cdc_supervisor_set_partitioned(supervisor, 1);
    CHECK(cdc_supervisor_admit(supervisor, &first, 150, 2, commit_once, &state,
                               &receipt) ==
          CDC_SUPERVISOR_HOLD_TRANSPORT);
    CHECK(receipt.transport_verdict == CDC_TRANSPORT_HOLD_PARTITION);
    cdc_supervisor_set_partitioned(supervisor, 0);
    CHECK(state.commits == 0);

    CHECK(cdc_supervisor_admit(supervisor, &first, 150, 2, commit_once, &state,
                               &receipt) == CDC_SUPERVISOR_ACCEPT);
    CHECK(state.commits == 1 && receipt.sequence == 1 &&
          receipt.action == CDC_AUTH_PROPOSE &&
          receipt.verified_approvals == 2 && receipt.horizon == 12 &&
          receipt.authority_expires_at == 200 &&
          receipt.authority_revision > 0);
    CHECK(cdc_supervisor_admit(supervisor, &first, 150, 2, commit_once, &state,
                               &receipt) ==
          CDC_SUPERVISOR_HOLD_TRANSPORT);
    CHECK(receipt.transport_verdict == CDC_TRANSPORT_HOLD_DUPLICATE &&
          state.commits == 1);

    CHECK(make_proposal(&regressed_clock, "node-a", "supervisor", 2,
                        first.envelope_digest, 8, 12, "regressed-clock"));
    regressed_clock.logical_clock = first.logical_clock;
    CHECK(cdc_transport_envelope_sign(&regressed_clock, KEY));
    CHECK(cdc_supervisor_admit(supervisor, &regressed_clock, 150, 2,
                               commit_once, &state, &receipt) ==
          CDC_SUPERVISOR_HOLD_TRANSPORT);
    CHECK(receipt.transport_verdict == CDC_TRANSPORT_HOLD_CAUSAL_CLOCK &&
          state.commits == 1);

    CHECK(make_proposal(&second, "node-a", "supervisor", 2,
                        first.envelope_digest, 5, 12, "proposal-two"));
    state.fail = 1;
    CHECK(cdc_supervisor_admit(supervisor, &second, 150, 2, commit_once, &state,
                               &receipt) == CDC_SUPERVISOR_HOLD_COMMIT);
    CHECK(state.commits == 1);
    state.fail = 0;
    CHECK(cdc_supervisor_admit(supervisor, &second, 150, 2, commit_once, &state,
                               &receipt) == CDC_SUPERVISOR_ACCEPT);
    CHECK(state.commits == 2);

    CHECK(make_proposal(&third, "node-a", "supervisor", 3,
                        second.envelope_digest, 6, 12, "proposal-three"));
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    a.supervisor = supervisor;
    a.envelope = &third;
    a.commit = &state;
    b = a;
    CHECK(pthread_create(&thread_a, NULL, admit_thread, &a) == 0);
    CHECK(pthread_create(&thread_b, NULL, admit_thread, &b) == 0);
    CHECK(pthread_join(thread_a, NULL) == 0);
    CHECK(pthread_join(thread_b, NULL) == 0);
    CHECK(((a.verdict == CDC_SUPERVISOR_ACCEPT &&
            b.verdict == CDC_SUPERVISOR_HOLD_TRANSPORT) ||
           (b.verdict == CDC_SUPERVISOR_ACCEPT &&
            a.verdict == CDC_SUPERVISOR_HOLD_TRANSPORT)));
    CHECK(state.commits == 3);

    cdc_transport_envelope_free(&first);
    cdc_transport_envelope_free(&second);
    cdc_transport_envelope_free(&third);
    cdc_transport_envelope_free(&wrong_recipient);
    cdc_transport_envelope_free(&wrong_subject);
    cdc_transport_envelope_free(&wrong_horizon);
    cdc_transport_envelope_free(&regressed_clock);
    cdc_supervisor_destroy(supervisor);
    return 1;
}

static int test_resource_exhaustion_is_precommit(void) {
    cdc_supervisor *supervisor = make_supervisor(1, 1);
    cdc_transport_envelope first, second;
    cdc_supervisor_receipt receipt;
    commit_state state = {0, 0};
    CHECK(supervisor != NULL && add_lease(supervisor));
    CHECK(make_proposal(&first, "node-a", "supervisor", 1, NULL, 20, 12,
                        "capacity-one"));
    CHECK(cdc_supervisor_admit(supervisor, &first, 150, 2, commit_once, &state,
                               &receipt) == CDC_SUPERVISOR_ACCEPT);
    CHECK(make_proposal(&second, "node-a", "supervisor", 2,
                        first.envelope_digest, 21, 12, "capacity-two"));
    CHECK(cdc_supervisor_admit(supervisor, &second, 150, 2, commit_once, &state,
                               &receipt) ==
          CDC_SUPERVISOR_HOLD_AUTHORITY);
    CHECK(receipt.authority_verdict == CDC_AUTHORITY_HOLD_LIMIT &&
          state.commits == 1);
    cdc_transport_envelope_free(&first);
    cdc_transport_envelope_free(&second);
    cdc_supervisor_destroy(supervisor);
    return 1;
}

static int test_typed_application_verdicts(void) {
    cdc_supervisor *supervisor = make_supervisor(4, 8);
    cdc_transport_envelope rejected, reused_nonce, retryable;
    cdc_supervisor_receipt receipt;
    typed_apply_state state = {
        CDC_SUPERVISOR_APPLICATION_REJECT, 0, 0};

    CHECK(supervisor != NULL && add_lease(supervisor));
    CHECK(make_proposal(&rejected, "node-a", "supervisor", 1, NULL, 60, 12,
                        "malformed-application-payload"));
    CHECK(cdc_supervisor_admit_ex(supervisor, &rejected, 150, 2, apply_typed,
                                  &state, &receipt) ==
          CDC_SUPERVISOR_REJECT_APPLICATION);
    CHECK(receipt.application_verdict ==
              CDC_SUPERVISOR_APPLICATION_REJECT &&
          state.calls == 1 && state.mutations == 0);
    CHECK(cdc_supervisor_admit_ex(supervisor, &rejected, 150, 2, apply_typed,
                                  &state, &receipt) ==
          CDC_SUPERVISOR_HOLD_TRANSPORT);
    CHECK(receipt.transport_verdict == CDC_TRANSPORT_HOLD_DUPLICATE &&
          receipt.application_verdict ==
              CDC_SUPERVISOR_APPLICATION_NOT_RUN &&
          state.calls == 1);

    CHECK(make_proposal(&reused_nonce, "node-a", "supervisor", 2,
                        rejected.envelope_digest, 60, 12,
                        "same-consumed-nonce"));
    CHECK(cdc_supervisor_admit_ex(supervisor, &reused_nonce, 150, 2,
                                  apply_typed, &state, &receipt) ==
          CDC_SUPERVISOR_REJECT_AUTHORITY);
    CHECK(receipt.authority_verdict == CDC_AUTHORITY_REJECT_REPLAY &&
          state.calls == 1 && state.mutations == 0);

    CHECK(make_proposal(&retryable, "node-a", "supervisor", 2,
                        rejected.envelope_digest, 61, 12,
                        "retryable-application-payload"));
    state.next = CDC_SUPERVISOR_APPLICATION_HOLD;
    CHECK(cdc_supervisor_admit_ex(supervisor, &retryable, 150, 2,
                                  apply_typed, &state, &receipt) ==
          CDC_SUPERVISOR_HOLD_COMMIT);
    CHECK(receipt.application_verdict ==
              CDC_SUPERVISOR_APPLICATION_HOLD &&
          state.calls == 2 && state.mutations == 0);
    state.next = CDC_SUPERVISOR_APPLICATION_ACCEPT;
    CHECK(cdc_supervisor_admit_ex(supervisor, &retryable, 150, 2,
                                  apply_typed, &state, &receipt) ==
          CDC_SUPERVISOR_ACCEPT);
    CHECK(receipt.application_verdict ==
              CDC_SUPERVISOR_APPLICATION_ACCEPT &&
          state.calls == 3 && state.mutations == 1);

    cdc_transport_envelope_free(&rejected);
    cdc_transport_envelope_free(&reused_nonce);
    cdc_transport_envelope_free(&retryable);
    cdc_supervisor_destroy(supervisor);
    return 1;
}

int main(void) {
    if (!test_serialized_admission() ||
        !test_resource_exhaustion_is_precommit() ||
        !test_typed_application_verdicts()) {
        return 1;
    }
    puts("RFTC supervisor PASS: authenticated+authorized+causal admission "
         "serialized; accept/hold/reject typed; terminal reject consumes "
         "causal position without application mutation");
    return 0;
}

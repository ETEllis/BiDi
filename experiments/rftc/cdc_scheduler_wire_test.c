#include "../../runtime/cdc_supervised_scheduler.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "scheduler-wire check failed at %s:%d: %s\n",     \
                    __FILE__, __LINE__, #expr);                                \
            return 0;                                                          \
        }                                                                      \
    } while (0)

static const uint8_t KEY[CDC_TRANSPORT_TAG_SIZE] = {
    0x72, 0x63, 0x33, 0x10, 0xa1, 0xb2, 0xc3, 0xd4,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x90, 0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27,
    0x18, 0x09, 0xfa, 0xeb, 0xdc, 0xcd, 0xbe, 0xaf,
};

static const uint64_t MEMBERS[4] = {1, 2, 3, 4};
static const uint64_t SUCCESSORS[4] = {2, 3, 4, 1};

static void fill_digest(uint8_t digest[CDC_DIGEST_SIZE], uint8_t seed) {
    size_t i;
    for (i = 0; i < CDC_DIGEST_SIZE; i++) {
        digest[i] = (uint8_t)(seed + i + 1);
    }
}

static void fill_nonce(uint8_t nonce[CDC_TRANSPORT_NONCE_SIZE],
                       uint8_t seed) {
    size_t i;
    for (i = 0; i < CDC_TRANSPORT_NONCE_SIZE; i++) {
        nonce[i] = (uint8_t)(seed + i);
    }
}

static void put_u64(uint8_t *out, uint64_t value) {
    size_t i;
    for (i = 0; i < 8; i++) {
        out[7 - i] = (uint8_t)(value >> (i * 8));
    }
}

static cdc_scheduler_observation
make_observation(uint64_t member, double phase, uint64_t logical_clock,
                 uint64_t window_start, uint64_t window_end,
                 uint8_t digest_seed) {
    cdc_scheduler_observation observation;
    memset(&observation, 0, sizeof(observation));
    memcpy(observation.cell_id, "leaf-a", sizeof("leaf-a"));
    observation.member_id = member;
    observation.phase = phase;
    observation.observed_at = window_end;
    observation.logical_clock = logical_clock;
    observation.frame_version = 1;
    observation.window_start = window_start;
    observation.window_end = window_end;
    observation.seal_time = window_end;
    fill_digest(observation.source_digest, digest_seed);
    return observation;
}

static int canonical_reencode(const uint8_t *bytes, size_t length) {
    cdc_scheduler_payload payload;
    uint8_t encoded[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t encoded_length = 0;
    cdc_scheduler_wire_status status =
        cdc_scheduler_payload_decode(bytes, length, &payload);
    if (status != CDC_SCHEDULER_WIRE_OK) {
        return 1;
    }
    if (payload.kind == CDC_SCHEDULER_PAYLOAD_OBSERVATION) {
        status = cdc_scheduler_payload_encode_observation(
            &payload.observation, encoded, &encoded_length);
    } else {
        status = cdc_scheduler_payload_encode_witness(
            payload.cell_id, payload.target_logical_clock,
            &payload.witness, encoded, &encoded_length);
    }
    return status == CDC_SCHEDULER_WIRE_OK &&
           encoded_length == length &&
           memcmp(encoded, bytes, length) == 0;
}

static int test_wire_canonicality(void) {
    cdc_scheduler_observation observation =
        make_observation(1, 2.0 * M_PI + 0.25, 10, 90, 100, 3);
    cdc_topology_witness witness;
    cdc_scheduler_payload decoded;
    uint8_t bytes[CDC_SCHEDULER_PAYLOAD_MAX];
    uint8_t mutated[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t length = 0;
    size_t witness_length = 0;
    size_t i;
    unsigned bit;

    CHECK(cdc_scheduler_payload_encode_observation(
              &observation, bytes, &length) == CDC_SCHEDULER_WIRE_OK);
    CHECK(length <= CDC_SCHEDULER_PAYLOAD_MAX);
    CHECK(cdc_scheduler_payload_decode(bytes, length, &decoded) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(decoded.kind == CDC_SCHEDULER_PAYLOAD_OBSERVATION &&
          fabs(decoded.observation.phase - 0.25) < 1e-12);
    CHECK(canonical_reencode(bytes, length));

    for (i = 0; i < length; i++) {
        for (bit = 0; bit < 8; bit++) {
            memcpy(mutated, bytes, length);
            mutated[i] ^= (uint8_t)(1u << bit);
            CHECK(canonical_reencode(mutated, length));
        }
    }
    for (i = 0; i < length; i++) {
        CHECK(cdc_scheduler_payload_decode(bytes, i, &decoded) !=
              CDC_SCHEDULER_WIRE_OK);
    }
    memcpy(mutated, bytes, length);
    mutated[length] = 0;
    CHECK(cdc_scheduler_payload_decode(mutated, length + 1, &decoded) ==
          CDC_SCHEDULER_WIRE_ECANONICAL);

    memcpy(mutated, bytes, length);
    mutated[11] = 1;
    CHECK(cdc_scheduler_payload_decode(mutated, length, &decoded) ==
          CDC_SCHEDULER_WIRE_ECANONICAL);

    memcpy(mutated, bytes, length);
    put_u64(mutated + 12 + strlen("leaf-a") + 8,
            (uint64_t)-INT64_C(3141592653590));
    CHECK(cdc_scheduler_payload_decode(mutated, length, &decoded) ==
          CDC_SCHEDULER_WIRE_ECANONICAL);

    memset(&witness, 0, sizeof(witness));
    witness.kind = CDC_TOPOLOGY_EVENT_PHASE_SLIP;
    witness.from_member = 1;
    witness.to_member = 2;
    witness.logical_clock = 15;
    fill_digest(witness.evidence_digest, 90);
    CHECK(cdc_scheduler_payload_encode_witness(
              "leaf-a", 20, &witness, bytes, &witness_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(cdc_scheduler_payload_decode(bytes, witness_length, &decoded) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(decoded.kind == CDC_SCHEDULER_PAYLOAD_WITNESS &&
          decoded.target_logical_clock == 20 &&
          decoded.witness.kind == CDC_TOPOLOGY_EVENT_PHASE_SLIP);
    CHECK(canonical_reencode(bytes, witness_length));

    witness.kind = CDC_TOPOLOGY_EVENT_FRAME_CHANGE;
    witness.from_member = 4;
    witness.to_member = 5;
    CHECK(cdc_scheduler_payload_encode_witness(
              "leaf-a", 20, &witness, bytes, &witness_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(cdc_scheduler_payload_decode(bytes, witness_length, &decoded) ==
              CDC_SCHEDULER_WIRE_OK &&
          decoded.witness.kind == CDC_TOPOLOGY_EVENT_FRAME_CHANGE &&
          canonical_reencode(bytes, witness_length));
    return 1;
}

static cdc_scheduler *make_scheduler(void) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec spec;
    cdc_scheduler *scheduler;
    uint8_t configuration_digest[CDC_DIGEST_SIZE];
    memset(&config, 0, sizeof(config));
    config.cell_limit = 2;
    config.event_limit = 4;
    config.maximum_depth = 2;
    scheduler = cdc_scheduler_create(&config);
    if (!scheduler) {
        return NULL;
    }
    memset(&spec, 0, sizeof(spec));
    spec.cell_id = "leaf-a";
    spec.kind = CDC_SCHEDULER_LEAF;
    spec.member_ids = MEMBERS;
    spec.successor_ids = SUCCESSORS;
    spec.member_count = 4;
    spec.frame_version = 1;
    spec.reducer_version = 1;
    spec.topology_version = 1;
    spec.stale_after = 100;
    spec.causal_horizon = 100;
    if (cdc_scheduler_add_cell(scheduler, &spec) != CDC_SCHEDULER_OK ||
        cdc_scheduler_seal(scheduler, configuration_digest) !=
            CDC_SCHEDULER_OK) {
        cdc_scheduler_destroy(scheduler);
        return NULL;
    }
    return scheduler;
}

static cdc_supervisor *make_supervisor(void) {
    cdc_supervisor_config config;
    cdc_authority_lease lease;
    cdc_supervisor *supervisor;
    memset(&config, 0, sizeof(config));
    config.schema_version = 1;
    config.local_id = "scheduler-supervisor";
    config.key_id = "scheduler-key";
    config.key = KEY;
    config.payload_limit = CDC_SCHEDULER_PAYLOAD_MAX;
    config.stream_limit = 2;
    config.lease_limit = 2;
    config.nonce_limit = 32;
    supervisor = cdc_supervisor_create(&config);
    if (!supervisor) {
        return NULL;
    }
    memset(&lease, 0, sizeof(lease));
    lease.version = 1;
    memcpy(lease.lease_id, "scheduler-lease", sizeof("scheduler-lease"));
    memcpy(lease.subject, "node-a", sizeof("node-a"));
    memcpy(lease.frame, "leaf-a", sizeof("leaf-a"));
    lease.actions = CDC_AUTH_OBSERVE | CDC_AUTH_COMMIT;
    lease.horizon_start = 1;
    lease.horizon_end = 100;
    lease.not_before = 100;
    lease.expires_at = 300;
    lease.quorum_required = 2;
    lease.quorum_total = 3;
    if (cdc_supervisor_add_lease(supervisor, &lease) !=
        CDC_AUTHORITY_ACCEPT) {
        cdc_supervisor_destroy(supervisor);
        return NULL;
    }
    return supervisor;
}

static int make_envelope(cdc_transport_envelope *envelope,
                         uint64_t sequence, const uint8_t *parent,
                         uint8_t nonce_seed, uint32_t message_type,
                         uint32_t action, const uint8_t *payload,
                         size_t payload_length) {
    cdc_transport_envelope_init(envelope);
    envelope->schema_version = 1;
    envelope->message_type = message_type;
    memcpy(envelope->sender, "node-a", sizeof("node-a"));
    memcpy(envelope->recipient, "scheduler-supervisor",
           sizeof("scheduler-supervisor"));
    memcpy(envelope->frame, "leaf-a", sizeof("leaf-a"));
    memcpy(envelope->key_id, "scheduler-key", sizeof("scheduler-key"));
    memcpy(envelope->lease_id, "scheduler-lease",
           sizeof("scheduler-lease"));
    envelope->authority_action = action;
    envelope->horizon = 100;
    envelope->sequence = sequence;
    envelope->logical_clock = 1000 + sequence;
    fill_nonce(envelope->nonce, nonce_seed);
    if (parent) {
        memcpy(envelope->causal_parent, parent, CDC_TRANSPORT_TAG_SIZE);
    }
    return cdc_transport_envelope_set_payload(
               envelope, payload, payload_length) &&
           cdc_transport_envelope_sign(envelope, KEY);
}

static int test_supervised_scheduler(void) {
    cdc_scheduler *scheduler = make_scheduler();
    cdc_supervisor *supervisor = make_supervisor();
    cdc_scheduler_observation observations[8];
    cdc_transport_envelope envelopes[9];
    cdc_supervised_scheduler_receipt receipt;
    cdc_scheduler_run_report run;
    cdc_cell_state state;
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    uint8_t malformed[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length;
    const uint8_t *parent = NULL;
    size_t i;

    CHECK(scheduler && supervisor);
    memset(envelopes, 0, sizeof(envelopes));
    for (i = 0; i < 8; i++) {
        observations[i] =
            make_observation(MEMBERS[i % 4], i < 4 ? 0.1 : 0.2,
                             i < 4 ? 10 : 20, i < 4 ? 90 : 101,
                             i < 4 ? 100 : 110, (uint8_t)(20 + i));
    }
    for (i = 0; i < 4; i++) {
        CHECK(cdc_scheduler_payload_encode_observation(
                  &observations[i], payload, &payload_length) ==
              CDC_SCHEDULER_WIRE_OK);
        CHECK(make_envelope(&envelopes[i], i + 1, parent,
                            (uint8_t)(10 + i), CDC_TRANSPORT_OBSERVATION,
                            CDC_AUTH_OBSERVE, payload, payload_length));
        CHECK(cdc_supervised_scheduler_admit(
                  supervisor, scheduler, &envelopes[i], 150, 2,
                  &receipt) == CDC_SUPERVISOR_ACCEPT);
        CHECK(receipt.application_invoked &&
              receipt.application_mutated &&
              receipt.wire_status == CDC_SCHEDULER_WIRE_OK &&
              receipt.scheduler_status == CDC_SCHEDULER_OK);
        parent = envelopes[i].envelope_digest;
    }
    CHECK(cdc_scheduler_queued_count(scheduler) == 4);

    CHECK(cdc_scheduler_payload_encode_observation(
              &observations[4], payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(make_envelope(&envelopes[4], 5, parent, 20,
                        CDC_TRANSPORT_OBSERVATION, CDC_AUTH_OBSERVE, payload,
                        payload_length));
    CHECK(cdc_supervised_scheduler_admit(
              supervisor, scheduler, &envelopes[4], 150, 2, &receipt) ==
          CDC_SUPERVISOR_HOLD_COMMIT);
    CHECK(receipt.scheduler_status == CDC_SCHEDULER_HOLD_LIMIT &&
          !receipt.application_mutated &&
          cdc_scheduler_queued_count(scheduler) == 4);
    CHECK(cdc_scheduler_drain(scheduler, 16, &run) == CDC_SCHEDULER_OK);
    CHECK(cdc_supervised_scheduler_admit(
              supervisor, scheduler, &envelopes[4], 150, 2, &receipt) ==
          CDC_SUPERVISOR_ACCEPT);
    CHECK(receipt.application_mutated &&
          cdc_scheduler_queued_count(scheduler) == 1);
    parent = envelopes[4].envelope_digest;

    memcpy(malformed, payload, payload_length);
    malformed[0] ^= 0xff;
    CHECK(make_envelope(&envelopes[5], 6, parent, 21,
                        CDC_TRANSPORT_OBSERVATION, CDC_AUTH_OBSERVE,
                        malformed, payload_length));
    CHECK(cdc_supervised_scheduler_admit(
              supervisor, scheduler, &envelopes[5], 150, 2, &receipt) ==
          CDC_SUPERVISOR_REJECT_APPLICATION);
    CHECK(receipt.application_invoked &&
          !receipt.application_mutated &&
          receipt.wire_status == CDC_SCHEDULER_WIRE_ESCHEMA &&
          cdc_scheduler_queued_count(scheduler) == 1);
    parent = envelopes[5].envelope_digest;

    for (i = 5; i < 8; i++) {
        size_t envelope_index = i + 1;
        CHECK(cdc_scheduler_payload_encode_observation(
                  &observations[i], payload, &payload_length) ==
              CDC_SCHEDULER_WIRE_OK);
        CHECK(make_envelope(&envelopes[envelope_index],
                            envelope_index + 1, parent,
                            (uint8_t)(22 + i),
                            CDC_TRANSPORT_OBSERVATION, CDC_AUTH_OBSERVE,
                            payload, payload_length));
        CHECK(cdc_supervised_scheduler_admit(
                  supervisor, scheduler, &envelopes[envelope_index], 150, 2,
                  &receipt) == CDC_SUPERVISOR_ACCEPT);
        parent = envelopes[envelope_index].envelope_digest;
    }
    CHECK(cdc_scheduler_drain(scheduler, 16, &run) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &state) ==
          CDC_SCHEDULER_OK);
    CHECK(state.logical_clock == 20 && state.generation == 2);

    for (i = 0; i < 9; i++) {
        cdc_transport_envelope_free(&envelopes[i]);
    }
    cdc_supervisor_destroy(supervisor);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

static int test_stale_hold_exact_retry(void) {
    cdc_scheduler *scheduler = make_scheduler();
    cdc_supervisor *supervisor = make_supervisor();
    cdc_scheduler_observation observation;
    cdc_transport_envelope envelope;
    cdc_supervised_scheduler_receipt receipt;
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length = 0;

    CHECK(scheduler && supervisor);
    observation = make_observation(1, 0.25, 10, 90, 100, 75);
    CHECK(cdc_scheduler_payload_encode_observation(
              &observation, payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(make_envelope(&envelope, 1, NULL, 80,
                        CDC_TRANSPORT_OBSERVATION, CDC_AUTH_OBSERVE,
                        payload, payload_length));

    CHECK(cdc_supervised_scheduler_admit(
              supervisor, scheduler, &envelope, 201, 2, &receipt) ==
          CDC_SUPERVISOR_HOLD_COMMIT);
    CHECK(receipt.application_invoked &&
          !receipt.application_mutated &&
          receipt.scheduler_status == CDC_SCHEDULER_HOLD_STALE &&
          cdc_scheduler_queued_count(scheduler) == 0);

    CHECK(cdc_supervised_scheduler_admit(
              supervisor, scheduler, &envelope, 150, 2, &receipt) ==
          CDC_SUPERVISOR_ACCEPT);
    CHECK(receipt.application_invoked &&
          receipt.application_mutated &&
          receipt.scheduler_status == CDC_SCHEDULER_OK &&
          cdc_scheduler_queued_count(scheduler) == 1);

    cdc_transport_envelope_free(&envelope);
    cdc_supervisor_destroy(supervisor);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

int main(void) {
    if (!test_wire_canonicality() || !test_supervised_scheduler() ||
        !test_stale_hold_exact_retry()) {
        return 1;
    }
    puts("RFTC scheduler wire PASS: canonical roundtrip mutation/truncation "
         "refusal authenticated limit/stale hold-retry "
         "terminal-poison consumption");
    return 0;
}

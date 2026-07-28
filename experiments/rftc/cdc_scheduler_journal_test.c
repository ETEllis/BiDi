#define _POSIX_C_SOURCE 200809L

#include "../../runtime/cdc_scheduler_journal.h"

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "scheduler-journal check failed at %s:%d: %s\n",  \
                    __FILE__, __LINE__, #expr);                                \
            return 0;                                                          \
        }                                                                      \
    } while (0)

static const uint8_t KEY[CDC_TRANSPORT_TAG_SIZE] = {
    0x72, 0x63, 0x33, 0x20, 0xa1, 0xb2, 0xc3, 0xd4,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x90, 0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27,
    0x18, 0x09, 0xfa, 0xeb, 0xdc, 0xcd, 0xbe, 0xaf,
};

static const uint8_t WRONG_KEY[CDC_TRANSPORT_TAG_SIZE] = {
    0x62, 0x63, 0x33, 0x20, 0xa1, 0xb2, 0xc3, 0xd4,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x90, 0x81, 0x72, 0x63, 0x54, 0x45, 0x36, 0x27,
    0x18, 0x09, 0xfa, 0xeb, 0xdc, 0xcd, 0xbe, 0xaf,
};

static const uint64_t MEMBERS[4] = {1, 2, 3, 4};
static const uint64_t SUCCESSORS[4] = {2, 3, 4, 1};
static const uint64_t FRAME_TWO_MEMBERS[5] = {1, 2, 3, 4, 5};
static const uint64_t FRAME_TWO_SUCCESSORS[5] = {2, 3, 4, 5, 1};
static const uint64_t ARRIVAL[4] = {4, 1, 3, 2};
static const cdc_scheduler_journal_replay_config REPLAY_CONFIG = {
    1, "scheduler-supervisor", "scheduler-key", 4
};

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

static int stage_journal_record(
    cdc_store *store, cdc_scheduler_journal_outcome outcome,
    const cdc_transport_envelope *envelope) {
    uint8_t *envelope_wire = NULL;
    uint8_t *record = NULL;
    size_t envelope_length = 0;
    size_t record_length;
    cdc_store_status status;
    if (!cdc_transport_envelope_encode(envelope, &envelope_wire,
                                       &envelope_length) ||
        envelope_length > SIZE_MAX - 16) {
        free(envelope_wire);
        return 0;
    }
    record_length = 16 + envelope_length;
    record = malloc(record_length);
    if (!record) {
        free(envelope_wire);
        return 0;
    }
    memcpy(record, "CDJR", 4);
    record[4] = 1;
    record[5] = (uint8_t)outcome;
    record[6] = 0;
    record[7] = 0;
    put_u64(record + 8, (uint64_t)envelope_length);
    memcpy(record + 16, envelope_wire, envelope_length);
    status = cdc_store_stage(store, record, record_length);
    if (status == CDC_STORE_OK) {
        status = cdc_store_commit(store);
    }
    free(record);
    free(envelope_wire);
    return status == CDC_STORE_OK;
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

static cdc_scheduler *make_scheduler_with_digest(
    size_t event_limit,
    uint8_t configuration_digest[CDC_DIGEST_SIZE]) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec spec;
    cdc_scheduler *scheduler;
    memset(&config, 0, sizeof(config));
    config.cell_limit = 2;
    config.event_limit = event_limit;
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

static cdc_scheduler *make_scheduler(size_t event_limit) {
    uint8_t configuration_digest[CDC_DIGEST_SIZE];
    return make_scheduler_with_digest(event_limit, configuration_digest);
}

static cdc_scheduler *make_frame_two_scheduler(
    size_t event_limit, const cdc_scheduler_epoch_state *previous,
    const uint8_t predecessor_digest[CDC_DIGEST_SIZE]) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec spec;
    cdc_scheduler *scheduler;
    uint8_t configuration_digest[CDC_DIGEST_SIZE];
    memset(&config, 0, sizeof(config));
    config.cell_limit = 2;
    config.event_limit = event_limit;
    config.maximum_depth = 2;
    scheduler = cdc_scheduler_create(&config);
    if (!scheduler) {
        return NULL;
    }
    memset(&spec, 0, sizeof(spec));
    spec.cell_id = "leaf-a";
    spec.kind = CDC_SCHEDULER_LEAF;
    spec.member_ids = FRAME_TWO_MEMBERS;
    spec.successor_ids = FRAME_TWO_SUCCESSORS;
    spec.member_count = 5;
    spec.frame_version = 2;
    spec.reducer_version = 1;
    spec.topology_version = 2;
    spec.stale_after = 100;
    spec.causal_horizon = 100;
    if (cdc_scheduler_add_cell(scheduler, &spec) != CDC_SCHEDULER_OK ||
        cdc_scheduler_import_previous_states(
            scheduler, previous, 1, predecessor_digest) !=
            CDC_SCHEDULER_OK ||
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
    config.nonce_limit = 64;
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
    lease.expires_at = 200;
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
                         uint8_t nonce_seed, const uint8_t *payload,
                         size_t payload_length) {
    cdc_transport_envelope_init(envelope);
    envelope->schema_version = 1;
    envelope->message_type = CDC_TRANSPORT_OBSERVATION;
    memcpy(envelope->sender, "node-a", sizeof("node-a"));
    memcpy(envelope->recipient, "scheduler-supervisor",
           sizeof("scheduler-supervisor"));
    memcpy(envelope->frame, "leaf-a", sizeof("leaf-a"));
    memcpy(envelope->key_id, "scheduler-key", sizeof("scheduler-key"));
    memcpy(envelope->lease_id, "scheduler-lease",
           sizeof("scheduler-lease"));
    envelope->authority_action = CDC_AUTH_OBSERVE;
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

static int create_temp_dir(char *dir, size_t dir_size, const char *label) {
    unsigned int attempt;
    for (attempt = 0; attempt < 1000; attempt++) {
        int written =
            snprintf(dir, dir_size, "/tmp/cdc-scheduler-journal-%s-%ld-%u",
                     label, (long)getpid(), attempt);
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
    return 0;
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

static int admit_payload(cdc_supervisor *supervisor, cdc_store *store,
                         uint64_t sequence, const uint8_t *parent,
                         uint8_t nonce_seed, const uint8_t *payload,
                         size_t payload_length,
                         cdc_transport_envelope *envelope,
                         cdc_scheduler_journal_admission_receipt *receipt) {
    if (!make_envelope(envelope, sequence, parent, nonce_seed, payload,
                       payload_length)) {
        return 0;
    }
    return cdc_scheduler_journal_admit(supervisor, store, envelope, 150, 2,
                                       receipt) ==
           CDC_SUPERVISOR_ACCEPT;
}

typedef struct {
    cdc_supervisor *supervisor;
    cdc_store *store;
    const cdc_transport_envelope *envelope;
    cdc_supervisor_verdict verdict;
    cdc_scheduler_journal_admission_receipt receipt;
} concurrent_admission;

static void *run_concurrent_admission(void *opaque) {
    concurrent_admission *admission = opaque;
    admission->verdict = cdc_scheduler_journal_admit(
        admission->supervisor, admission->store, admission->envelope,
        150, 2, &admission->receipt);
    return NULL;
}

static int test_concurrent_duplicate_admission(void) {
    char dir[192];
    cdc_store *store = NULL;
    cdc_supervisor *supervisor = NULL;
    cdc_scheduler_observation observation;
    cdc_transport_envelope envelope;
    concurrent_admission admissions[2];
    pthread_t threads[2];
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length = 0;
    size_t accepted = 0;
    size_t held_duplicate = 0;
    size_t i;
    int recovered = -1;

    CHECK(create_temp_dir(dir, sizeof(dir), "concurrent"));
    CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
          recovered == 0);
    supervisor = make_supervisor();
    CHECK(supervisor);
    observation = make_observation(1, 0.25, 10, 90, 100, 70);
    CHECK(cdc_scheduler_payload_encode_observation(
              &observation, payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(make_envelope(&envelope, 1, NULL, 80, payload, payload_length));
    memset(admissions, 0, sizeof(admissions));
    for (i = 0; i < 2; i++) {
        admissions[i].supervisor = supervisor;
        admissions[i].store = store;
        admissions[i].envelope = &envelope;
        CHECK(pthread_create(&threads[i], NULL, run_concurrent_admission,
                             &admissions[i]) == 0);
    }
    for (i = 0; i < 2; i++) {
        CHECK(pthread_join(threads[i], NULL) == 0);
        if (admissions[i].verdict == CDC_SUPERVISOR_ACCEPT) {
            accepted++;
            CHECK(admissions[i].receipt.durable);
        } else if (
            admissions[i].verdict == CDC_SUPERVISOR_HOLD_TRANSPORT &&
            admissions[i].receipt.supervisor.transport_verdict ==
                CDC_TRANSPORT_HOLD_DUPLICATE) {
            held_duplicate++;
            CHECK(!admissions[i].receipt.durable);
        }
    }
    CHECK(accepted == 1 && held_duplicate == 1 &&
          cdc_store_sealed_count(store) == 1 &&
          cdc_store_event_count(store) == 1);

    cdc_transport_envelope_free(&envelope);
    cdc_supervisor_destroy(supervisor);
    cdc_store_close(store);
    cleanup_store(dir);
    return 1;
}

static int test_every_commit_crash_boundary(void) {
    int boundary;
    for (boundary = 1; boundary <= 5; boundary++) {
        char dir[192];
        char label[32];
        cdc_store *store = NULL;
        cdc_store *reopened = NULL;
        cdc_supervisor *supervisor = NULL;
        cdc_scheduler_observation observation;
        cdc_scheduler_journal_admission_receipt admission;
        cdc_transport_envelope envelope;
        uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
        size_t payload_length = 0;
        uint64_t before_retry;
        int recovered = -1;

        CHECK(snprintf(label, sizeof(label), "crash-%d", boundary) > 0);
        CHECK(create_temp_dir(dir, sizeof(dir), label));
        CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
              recovered == 0);
        supervisor = make_supervisor();
        CHECK(supervisor);
        observation = make_observation(1, 0.25, 10, 90, 100,
                                       (uint8_t)(90 + boundary));
        CHECK(cdc_scheduler_payload_encode_observation(
                  &observation, payload, &payload_length) ==
              CDC_SCHEDULER_WIRE_OK);
        CHECK(make_envelope(&envelope, 1, NULL,
                            (uint8_t)(100 + boundary), payload,
                            payload_length));
        cdc_store_set_fail_after(store, boundary);
        CHECK(cdc_scheduler_journal_admit(
                  supervisor, store, &envelope, 150, 2, &admission) ==
              CDC_SUPERVISOR_HOLD_COMMIT);
        CHECK(!admission.durable &&
              admission.store_status == CDC_STORE_ECRASH);
        cdc_store_set_fail_after(store, 0);
        cdc_store_close(store);
        store = NULL;

        CHECK(cdc_store_open(dir, &reopened, &recovered) == CDC_STORE_OK);
        before_retry = cdc_store_event_count(reopened);
        CHECK(before_retry <= 1 &&
              cdc_scheduler_journal_admit(
                  supervisor, reopened, &envelope, 150, 2, &admission) ==
                  CDC_SUPERVISOR_ACCEPT);
        CHECK(admission.durable &&
              cdc_store_event_count(reopened) == before_retry + 1 &&
              cdc_store_verify(reopened) == CDC_STORE_OK);

        cdc_transport_envelope_free(&envelope);
        cdc_supervisor_destroy(supervisor);
        cdc_store_close(reopened);
        cleanup_store(dir);
    }
    return 1;
}

static int test_durable_replay_and_failures(void) {
    char dir[192];
    cdc_store *store = NULL;
    cdc_store *reopened = NULL;
    cdc_supervisor *supervisor = NULL;
    cdc_scheduler *first = NULL;
    cdc_scheduler *second = NULL;
    cdc_scheduler *wrong_key_scheduler = NULL;
    cdc_scheduler *limited = NULL;
    cdc_scheduler *nonpristine = NULL;
    cdc_scheduler *compacted = NULL;
    cdc_scheduler_observation observations[8];
    cdc_scheduler_journal_admission_receipt admission;
    cdc_scheduler_journal_replay_report first_report, second_report;
    cdc_scheduler_journal_replay_report failure_report;
    cdc_transport_envelope envelope;
    cdc_transport_envelope failed_envelope;
    cdc_cell_state first_state, second_state;
    uint8_t parent[CDC_TRANSPORT_TAG_SIZE];
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    uint8_t malformed[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length = 0;
    size_t i;
    int recovered = -1;
    int have_parent = 0;

    CHECK(create_temp_dir(dir, sizeof(dir), "replay"));
    CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
          recovered == 0);
    supervisor = make_supervisor();
    CHECK(supervisor);
    memset(&envelope, 0, sizeof(envelope));
    memset(&failed_envelope, 0, sizeof(failed_envelope));

    for (i = 0; i < 8; i++) {
        size_t epoch_member = i % 4;
        observations[i] = make_observation(
            ARRIVAL[epoch_member], i < 4 ? 0.1 : 0.2,
            i < 4 ? 10 : 20, i < 4 ? 90 : 101,
            i < 4 ? 100 : 110, (uint8_t)(20 + i));
        CHECK(cdc_scheduler_payload_encode_observation(
                  &observations[i], payload, &payload_length) ==
              CDC_SCHEDULER_WIRE_OK);
        CHECK(admit_payload(supervisor, store, i + 1,
                            have_parent ? parent : NULL,
                            (uint8_t)(10 + i), payload, payload_length,
                            &envelope, &admission));
        CHECK(admission.durable &&
              admission.store_status == CDC_STORE_OK &&
              admission.wire_status == CDC_SCHEDULER_WIRE_OK);
        memcpy(parent, envelope.envelope_digest, sizeof(parent));
        have_parent = 1;
        cdc_transport_envelope_free(&envelope);
    }

    memcpy(malformed, payload, payload_length);
    malformed[0] ^= 0xffu;
    CHECK(make_envelope(&envelope, 9, parent, 40, malformed,
                        payload_length));
    CHECK(cdc_scheduler_journal_admit(
              supervisor, store, &envelope, 150, 2, &admission) ==
          CDC_SUPERVISOR_REJECT_APPLICATION);
    CHECK(admission.application_invoked && admission.durable &&
          admission.outcome == CDC_SCHEDULER_JOURNAL_OUTCOME_REJECT &&
          admission.wire_status == CDC_SCHEDULER_WIRE_ESCHEMA &&
          cdc_store_event_count(store) == 9);
    memcpy(parent, envelope.envelope_digest, sizeof(parent));
    cdc_transport_envelope_free(&envelope);

    CHECK(cdc_scheduler_payload_encode_observation(
              &observations[7], payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(admit_payload(supervisor, store, 10, parent, 41, payload,
                        payload_length, &envelope, &admission));
    CHECK(cdc_store_event_count(store) == 10);
    memcpy(parent, envelope.envelope_digest, sizeof(parent));
    cdc_transport_envelope_free(&envelope);

    CHECK(make_envelope(&failed_envelope, 11, parent, 42, payload,
                        payload_length));
    cdc_store_set_fail_after(store, 1);
    CHECK(cdc_scheduler_journal_admit(
          supervisor, store, &failed_envelope, 150, 2, &admission) ==
          CDC_SUPERVISOR_HOLD_COMMIT);
    CHECK(admission.application_invoked && !admission.durable &&
          admission.store_status == CDC_STORE_ECRASH &&
          cdc_store_event_count(store) == 10);
    cdc_store_set_fail_after(store, 0);
    cdc_store_close(store);
    store = NULL;

    CHECK(cdc_store_open(dir, &reopened, &recovered) == CDC_STORE_OK &&
          recovered == 1 && cdc_store_event_count(reopened) == 10);
    CHECK(cdc_scheduler_journal_admit(
              supervisor, reopened, &failed_envelope, 150, 2, &admission) ==
          CDC_SUPERVISOR_ACCEPT);
    CHECK(admission.durable && cdc_store_event_count(reopened) == 11);
    cdc_transport_envelope_free(&failed_envelope);

    first = make_scheduler(4);
    second = make_scheduler(4);
    wrong_key_scheduler = make_scheduler(4);
    limited = make_scheduler(4);
    nonpristine = make_scheduler(4);
    compacted = make_scheduler(4);
    CHECK(first && second && wrong_key_scheduler && limited &&
          nonpristine && compacted);

    CHECK(cdc_scheduler_journal_replay(
              reopened, first, KEY, &REPLAY_CONFIG, 11, &first_report) ==
          CDC_SCHEDULER_JOURNAL_OK);
    CHECK(first_report.records_verified == 11 &&
          first_report.events_recovered == 10 &&
          first_report.terminal_rejections == 1 &&
          first_report.duplicate_events == 2 &&
          first_report.queued_remaining == 0 &&
          cdc_scheduler_get_state(first, "leaf-a", &first_state) ==
              CDC_SCHEDULER_OK &&
          first_state.logical_clock == 20 && first_state.generation == 2);

    CHECK(cdc_scheduler_journal_replay(
              reopened, second, KEY, &REPLAY_CONFIG, 11,
              &second_report) ==
          CDC_SCHEDULER_JOURNAL_OK);
    CHECK(cdc_scheduler_get_state(second, "leaf-a", &second_state) ==
              CDC_SCHEDULER_OK &&
          memcmp(&first_state, &second_state, sizeof(first_state)) == 0 &&
          memcmp(first_report.configuration_digest,
                 second_report.configuration_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(first_report.execution_digest,
                 second_report.execution_digest,
                 CDC_DIGEST_SIZE) == 0);

    CHECK(cdc_scheduler_journal_replay(
              reopened, wrong_key_scheduler, WRONG_KEY, &REPLAY_CONFIG, 11,
              &failure_report) == CDC_SCHEDULER_JOURNAL_EAUTH);
    CHECK(cdc_scheduler_is_pristine(wrong_key_scheduler) &&
          failure_report.transport_verdict ==
              CDC_TRANSPORT_REJECT_AUTH);

    CHECK(cdc_scheduler_journal_replay(
              reopened, limited, KEY, &REPLAY_CONFIG, 10,
              &failure_report) ==
          CDC_SCHEDULER_JOURNAL_HOLD_LIMIT);
    CHECK(cdc_scheduler_is_pristine(limited));

    CHECK(cdc_scheduler_publish(nonpristine, &observations[0], 100) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_journal_replay(
              reopened, nonpristine, KEY, &REPLAY_CONFIG, 11,
              &failure_report) ==
          CDC_SCHEDULER_JOURNAL_EARG);

    CHECK(cdc_store_snapshot(reopened) == CDC_STORE_OK &&
          cdc_store_compact(reopened) == CDC_STORE_OK);
    CHECK(cdc_scheduler_journal_replay(
              reopened, compacted, KEY, &REPLAY_CONFIG, 11,
              &failure_report) ==
          CDC_SCHEDULER_JOURNAL_EUNSUPPORTED);
    CHECK(cdc_scheduler_is_pristine(compacted));

    cdc_scheduler_destroy(first);
    cdc_scheduler_destroy(second);
    cdc_scheduler_destroy(wrong_key_scheduler);
    cdc_scheduler_destroy(limited);
    cdc_scheduler_destroy(nonpristine);
    cdc_scheduler_destroy(compacted);
    cdc_supervisor_destroy(supervisor);
    cdc_store_close(reopened);
    cleanup_store(dir);
    return 1;
}

static int test_corruption_zero_mutation(void) {
    char dir[192];
    cdc_store *store = NULL;
    cdc_supervisor *supervisor = NULL;
    cdc_scheduler *scheduler = NULL;
    cdc_scheduler_observation observation;
    cdc_scheduler_journal_admission_receipt admission;
    cdc_scheduler_journal_replay_report report;
    cdc_transport_envelope envelope;
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length = 0;
    int recovered = -1;

    CHECK(create_temp_dir(dir, sizeof(dir), "corrupt"));
    CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
          recovered == 0);
    supervisor = make_supervisor();
    scheduler = make_scheduler(4);
    CHECK(supervisor && scheduler);
    observation = make_observation(1, 0.25, 10, 90, 100, 7);
    CHECK(cdc_scheduler_payload_encode_observation(
              &observation, payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(admit_payload(supervisor, store, 1, NULL, 60, payload,
                        payload_length, &envelope, &admission));
    cdc_transport_envelope_free(&envelope);
    CHECK(flip_payload_byte(dir, "scheduler-supervisor"));
    CHECK(cdc_scheduler_journal_replay(
              store, scheduler, KEY, &REPLAY_CONFIG, 1, &report) ==
          CDC_SCHEDULER_JOURNAL_ESTORE);
    CHECK(report.store_status == CDC_STORE_ECORRUPT &&
          cdc_scheduler_is_pristine(scheduler));

    cdc_scheduler_destroy(scheduler);
    cdc_supervisor_destroy(supervisor);
    cdc_store_close(store);
    cleanup_store(dir);
    return 1;
}

static int test_late_malformed_history_zero_mutation(void) {
    char dir[192];
    cdc_store *store = NULL;
    cdc_scheduler *scheduler = NULL;
    cdc_scheduler_observation observation;
    cdc_scheduler_journal_replay_report report;
    cdc_transport_envelope valid;
    cdc_transport_envelope malformed_envelope;
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    uint8_t malformed[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length = 0;
    int recovered = -1;

    CHECK(create_temp_dir(dir, sizeof(dir), "malformed-history"));
    CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
          recovered == 0);
    scheduler = make_scheduler(4);
    CHECK(scheduler);
    observation = make_observation(1, 0.25, 10, 90, 100, 8);
    CHECK(cdc_scheduler_payload_encode_observation(
              &observation, payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(make_envelope(&valid, 1, NULL, 70, payload, payload_length));
    memcpy(malformed, payload, payload_length);
    malformed[0] ^= 0xffu;
    CHECK(make_envelope(&malformed_envelope, 2, valid.envelope_digest, 71,
                        malformed, payload_length));
    CHECK(stage_journal_record(
              store, CDC_SCHEDULER_JOURNAL_OUTCOME_ACCEPT, &valid) &&
          stage_journal_record(
              store, CDC_SCHEDULER_JOURNAL_OUTCOME_ACCEPT,
              &malformed_envelope));
    CHECK(cdc_scheduler_journal_replay(
              store, scheduler, KEY, &REPLAY_CONFIG, 2, &report) ==
          CDC_SCHEDULER_JOURNAL_EWIRE);
    CHECK(report.wire_status == CDC_SCHEDULER_WIRE_ESCHEMA &&
          cdc_scheduler_is_pristine(scheduler));

    cdc_transport_envelope_free(&valid);
    cdc_transport_envelope_free(&malformed_envelope);
    cdc_scheduler_destroy(scheduler);
    cdc_store_close(store);
    cleanup_store(dir);
    return 1;
}

static int test_replay_peer_policy_zero_mutation(void) {
    enum { CASE_COUNT = 4 };
    size_t policy_case;
    for (policy_case = 0; policy_case < CASE_COUNT; policy_case++) {
        char dir[192];
        char label[32];
        cdc_store *store = NULL;
        cdc_scheduler *scheduler = NULL;
        cdc_scheduler_observation observation;
        cdc_scheduler_journal_replay_report report;
        cdc_transport_envelope envelope;
        uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
        size_t payload_length = 0;
        int recovered = -1;

        CHECK(snprintf(label, sizeof(label), "policy-%zu",
                       policy_case) > 0);
        CHECK(create_temp_dir(dir, sizeof(dir), label));
        CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
              recovered == 0);
        scheduler = make_scheduler(4);
        CHECK(scheduler);
        observation = make_observation(1, 0.25, 10, 90, 100,
                                       (uint8_t)(110 + policy_case));
        CHECK(cdc_scheduler_payload_encode_observation(
                  &observation, payload, &payload_length) ==
              CDC_SCHEDULER_WIRE_OK);
        CHECK(make_envelope(&envelope, 1, NULL,
                            (uint8_t)(120 + policy_case), payload,
                            payload_length));
        if (policy_case == 0) {
            memcpy(envelope.recipient, "wrong-recipient",
                   sizeof("wrong-recipient"));
        } else if (policy_case == 1) {
            memcpy(envelope.key_id, "wrong-key", sizeof("wrong-key"));
        } else if (policy_case == 2) {
            envelope.sequence = 2;
        } else {
            memset(envelope.causal_parent, 0xa5,
                   sizeof(envelope.causal_parent));
        }
        CHECK(cdc_transport_envelope_sign(&envelope, KEY));
        CHECK(stage_journal_record(
            store, CDC_SCHEDULER_JOURNAL_OUTCOME_ACCEPT, &envelope));
        CHECK(cdc_scheduler_journal_replay(
                  store, scheduler, KEY, &REPLAY_CONFIG, 1, &report) ==
              CDC_SCHEDULER_JOURNAL_EPOLICY);
        CHECK(report.transport_verdict != CDC_TRANSPORT_ACCEPT &&
              cdc_scheduler_is_pristine(scheduler));

        cdc_transport_envelope_free(&envelope);
        cdc_scheduler_destroy(scheduler);
        cdc_store_close(store);
        cleanup_store(dir);
    }
    return 1;
}

static int test_configuration_epoch_journal_replay(void) {
    static const double WINDING_ONE[4] = {
        0.0, M_PI / 2.0, M_PI, 3.0 * M_PI / 2.0};
    char dir[192];
    cdc_store *store = NULL;
    cdc_supervisor *supervisor = NULL;
    cdc_scheduler *prior_scheduler = NULL;
    cdc_scheduler *first = NULL;
    cdc_scheduler *second = NULL;
    cdc_scheduler_observation observation;
    cdc_scheduler_journal_admission_receipt admission;
    cdc_scheduler_journal_replay_report first_report, second_report;
    cdc_scheduler_run_report prior_report;
    cdc_transport_envelope envelope;
    cdc_cell_state prior_state, first_state, second_state;
    cdc_scheduler_epoch_state prior_epoch;
    cdc_topology_witness witness;
    uint8_t prior_digest[CDC_DIGEST_SIZE];
    uint8_t parent[CDC_TRANSPORT_TAG_SIZE];
    uint8_t payload[CDC_SCHEDULER_PAYLOAD_MAX];
    size_t payload_length = 0;
    size_t i;
    int recovered = -1;

    prior_scheduler = make_scheduler_with_digest(16, prior_digest);
    CHECK(prior_scheduler);
    for (i = 0; i < 4; i++) {
        observation = make_observation(
            MEMBERS[i], WINDING_ONE[i], 10, 90, 100,
            (uint8_t)(140 + i));
        CHECK(cdc_scheduler_publish(prior_scheduler, &observation, 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(prior_scheduler, 16, &prior_report) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_state(prior_scheduler, "leaf-a",
                                  &prior_state) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_epoch_state(
              prior_scheduler, "leaf-a", &prior_epoch) ==
              CDC_SCHEDULER_OK &&
          memcmp(prior_epoch.state.state_digest, prior_state.state_digest,
                 CDC_DIGEST_SIZE) == 0);

    CHECK(create_temp_dir(dir, sizeof(dir), "frame-epoch"));
    CHECK(cdc_store_open(dir, &store, &recovered) == CDC_STORE_OK &&
          recovered == 0);
    supervisor = make_supervisor();
    CHECK(supervisor);
    memset(parent, 0, sizeof(parent));
    for (i = 0; i < 5; i++) {
        observation = make_observation(
            FRAME_TWO_MEMBERS[i], 0.0, 20, 101, 110,
            (uint8_t)(150 + i));
        observation.frame_version = 2;
        CHECK(cdc_scheduler_payload_encode_observation(
                  &observation, payload, &payload_length) ==
              CDC_SCHEDULER_WIRE_OK);
        CHECK(make_envelope(&envelope, i + 1, i ? parent : NULL,
                            (uint8_t)(160 + i), payload, payload_length));
        CHECK(cdc_scheduler_journal_admit(
                  supervisor, store, &envelope, 150, 2, &admission) ==
                  CDC_SUPERVISOR_ACCEPT);
        memcpy(parent, envelope.envelope_digest, sizeof(parent));
        cdc_transport_envelope_free(&envelope);
    }
    memset(&witness, 0, sizeof(witness));
    witness.kind = CDC_TOPOLOGY_EVENT_FRAME_CHANGE;
    witness.from_member = 4;
    witness.to_member = 5;
    witness.logical_clock = 15;
    fill_digest(witness.evidence_digest, 170);
    CHECK(cdc_scheduler_payload_encode_witness(
              "leaf-a", 20, &witness, payload, &payload_length) ==
          CDC_SCHEDULER_WIRE_OK);
    CHECK(make_envelope(&envelope, 6, parent, 171,
                        payload, payload_length));
    envelope.message_type = CDC_TRANSPORT_DECISION;
    envelope.authority_action = CDC_AUTH_COMMIT;
    CHECK(cdc_transport_envelope_sign(&envelope, KEY));
    CHECK(cdc_scheduler_journal_admit(
              supervisor, store, &envelope, 150, 2, &admission) ==
          CDC_SUPERVISOR_ACCEPT);
    cdc_transport_envelope_free(&envelope);

    first = make_frame_two_scheduler(16, &prior_epoch, prior_digest);
    second = make_frame_two_scheduler(16, &prior_epoch, prior_digest);
    CHECK(first && second && cdc_scheduler_is_pristine(first) &&
          cdc_scheduler_is_pristine(second));
    CHECK(cdc_scheduler_journal_replay(
              store, first, KEY, &REPLAY_CONFIG, 6, NULL) ==
              CDC_SCHEDULER_JOURNAL_EARG &&
          cdc_scheduler_is_pristine(first));
    CHECK(cdc_scheduler_journal_replay(
              store, first, KEY, &REPLAY_CONFIG, 6, &first_report) ==
              CDC_SCHEDULER_JOURNAL_OK &&
          cdc_scheduler_journal_replay(
              store, second, KEY, &REPLAY_CONFIG, 6, &second_report) ==
              CDC_SCHEDULER_JOURNAL_OK);
    CHECK(first_report.records_verified == 6 &&
          first_report.events_recovered == 6 &&
          first_report.terminal_rejections == 0 &&
          cdc_scheduler_get_state(first, "leaf-a", &first_state) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_state(second, "leaf-a", &second_state) ==
              CDC_SCHEDULER_OK &&
          first_state.generation == prior_state.generation + 1 &&
          first_state.frame_version == 2 &&
          first_state.transition_kind == CDC_TOPOLOGY_EVENT_FRAME_CHANGE &&
          memcmp(first_state.previous_state_digest,
                 prior_state.state_digest, CDC_DIGEST_SIZE) == 0 &&
          memcmp(first_state.state_digest, second_state.state_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(first_report.execution_digest,
                 second_report.execution_digest,
                 CDC_DIGEST_SIZE) == 0);

    cdc_scheduler_destroy(prior_scheduler);
    cdc_scheduler_destroy(first);
    cdc_scheduler_destroy(second);
    cdc_supervisor_destroy(supervisor);
    cdc_store_close(store);
    cleanup_store(dir);
    return 1;
}

int main(void) {
    if (!test_concurrent_duplicate_admission() ||
        !test_every_commit_crash_boundary() ||
        !test_durable_replay_and_failures() ||
        !test_corruption_zero_mutation() ||
        !test_late_malformed_history_zero_mutation() ||
        !test_replay_peer_policy_zero_mutation() ||
        !test_configuration_epoch_journal_replay()) {
        return 1;
    }
    puts("RFTC scheduler journal PASS: concurrent-duplicate=serialized "
         "auth+peer-policy=reverified crash-matrix=retryable "
         "replay=deterministic duplicates=idempotent bad-history=zero-mutation "
         "terminal-rejects=causal corruption=zero-mutation "
         "frame-epoch=replayed compaction=refused");
    return 0;
}

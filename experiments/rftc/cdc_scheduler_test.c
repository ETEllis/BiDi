#include "../../runtime/cdc_scheduler.h"

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "scheduler check failed at %s:%d: %s\n",          \
                    __FILE__, __LINE__, #expr);                                \
            return 0;                                                          \
        }                                                                      \
    } while (0)

static const uint64_t LEAF_A_MEMBERS[4] = {1, 2, 3, 4};
static const uint64_t LEAF_A_SUCCESSORS[4] = {2, 3, 4, 1};
static const uint64_t LEAF_B_MEMBERS[4] = {11, 12, 13, 14};
static const uint64_t LEAF_B_SUCCESSORS[4] = {12, 13, 14, 11};
static const uint64_t ROOT_MEMBERS[2] = {101, 102};
static const uint64_t ROOT_SUCCESSORS[2] = {102, 101};
static const char *const ROOT_CHILDREN[2] = {"leaf-a", "leaf-b"};
static const char *const ROOT_CHILDREN_SWAPPED[2] = {"leaf-b", "leaf-a"};
static const uint64_t LEAF_A_MEMBERS_PERMUTED[4] = {3, 1, 4, 2};
static const uint64_t LEAF_A_SUCCESSORS_PERMUTED[4] = {4, 2, 1, 3};
static const uint64_t LEAF_B_MEMBERS_PERMUTED[4] = {13, 11, 14, 12};
static const uint64_t LEAF_B_SUCCESSORS_PERMUTED[4] = {14, 12, 11, 13};
static const uint64_t ROOT_MEMBERS_PERMUTED[2] = {102, 101};
static const uint64_t ROOT_SUCCESSORS_PERMUTED[2] = {101, 102};
static const char *const ROOT_CHILDREN_PERMUTED[2] = {"leaf-b", "leaf-a"};
static const uint64_t FRAME_TWO_MEMBERS[5] = {1, 2, 3, 4, 5};
static const uint64_t FRAME_TWO_SUCCESSORS[5] = {2, 3, 4, 5, 1};
static const uint64_t FRAME_TWO_MEMBERS_PERMUTED[5] = {3, 5, 1, 4, 2};
static const uint64_t FRAME_TWO_SUCCESSORS_PERMUTED[5] = {4, 1, 2, 5, 3};

static void fill_digest(uint8_t digest[CDC_DIGEST_SIZE], uint8_t seed) {
    size_t i;
    for (i = 0; i < CDC_DIGEST_SIZE; i++) {
        digest[i] = (uint8_t)(seed + i + 1);
    }
}

static cdc_scheduler_cell_spec
leaf_spec(const char *cell_id, const uint64_t *members,
          const uint64_t *successors, uint64_t causal_horizon) {
    cdc_scheduler_cell_spec spec;
    memset(&spec, 0, sizeof(spec));
    spec.cell_id = cell_id;
    spec.kind = CDC_SCHEDULER_LEAF;
    spec.member_ids = members;
    spec.successor_ids = successors;
    spec.member_count = 4;
    spec.frame_version = 1;
    spec.reducer_version = 1;
    spec.topology_version = 1;
    spec.stale_after = 100;
    spec.causal_horizon = causal_horizon;
    return spec;
}

static cdc_scheduler_cell_spec root_spec(int permuted_ring_inputs) {
    cdc_scheduler_cell_spec spec;
    memset(&spec, 0, sizeof(spec));
    spec.cell_id = "root";
    spec.kind = CDC_SCHEDULER_COMPOSITE;
    spec.member_ids = permuted_ring_inputs ? ROOT_MEMBERS_PERMUTED
                                           : ROOT_MEMBERS;
    spec.successor_ids = permuted_ring_inputs
                             ? ROOT_SUCCESSORS_PERMUTED
                             : ROOT_SUCCESSORS;
    spec.child_cell_ids = permuted_ring_inputs
                              ? ROOT_CHILDREN_PERMUTED
                              : ROOT_CHILDREN;
    spec.member_count = 2;
    spec.frame_version = 1;
    spec.reducer_version = 1;
    spec.topology_version = 1;
    spec.stale_after = 100;
    spec.causal_horizon = 1000;
    return spec;
}

static cdc_scheduler_observation
observation(const char *cell_id, uint64_t member_id, double phase,
            uint64_t logical_clock, uint64_t window_start,
            uint64_t window_end, uint8_t digest_seed) {
    cdc_scheduler_observation item;
    memset(&item, 0, sizeof(item));
    memcpy(item.cell_id, cell_id, strlen(cell_id) + 1);
    item.member_id = member_id;
    item.phase = phase;
    item.observed_at = window_end;
    item.logical_clock = logical_clock;
    item.frame_version = 1;
    item.window_start = window_start;
    item.window_end = window_end;
    item.seal_time = window_end;
    fill_digest(item.source_digest, digest_seed);
    return item;
}

static cdc_scheduler *make_tree_with_event_limit(
    int reverse_leaf_add_order, int permuted_ring_inputs,
    size_t event_limit, uint8_t config_digest[CDC_DIGEST_SIZE]) {
    cdc_scheduler_config config;
    cdc_scheduler *scheduler;
    cdc_scheduler_cell_spec a =
        leaf_spec("leaf-a",
                  permuted_ring_inputs ? LEAF_A_MEMBERS_PERMUTED
                                       : LEAF_A_MEMBERS,
                  permuted_ring_inputs ? LEAF_A_SUCCESSORS_PERMUTED
                                       : LEAF_A_SUCCESSORS,
                  1000);
    cdc_scheduler_cell_spec b =
        leaf_spec("leaf-b",
                  permuted_ring_inputs ? LEAF_B_MEMBERS_PERMUTED
                                       : LEAF_B_MEMBERS,
                  permuted_ring_inputs ? LEAF_B_SUCCESSORS_PERMUTED
                                       : LEAF_B_SUCCESSORS,
                  1000);
    cdc_scheduler_cell_spec root = root_spec(permuted_ring_inputs);
    memset(&config, 0, sizeof(config));
    config.cell_limit = 8;
    config.event_limit = event_limit;
    config.maximum_depth = 4;
    scheduler = cdc_scheduler_create(&config);
    if (!scheduler) {
        return NULL;
    }
    if (reverse_leaf_add_order) {
        if (cdc_scheduler_add_cell(scheduler, &b) != CDC_SCHEDULER_OK ||
            cdc_scheduler_add_cell(scheduler, &a) != CDC_SCHEDULER_OK) {
            cdc_scheduler_destroy(scheduler);
            return NULL;
        }
    } else if (cdc_scheduler_add_cell(scheduler, &a) != CDC_SCHEDULER_OK ||
               cdc_scheduler_add_cell(scheduler, &b) != CDC_SCHEDULER_OK) {
        cdc_scheduler_destroy(scheduler);
        return NULL;
    }
    if (cdc_scheduler_add_cell(scheduler, &root) != CDC_SCHEDULER_OK ||
        cdc_scheduler_seal(scheduler, config_digest) != CDC_SCHEDULER_OK) {
        cdc_scheduler_destroy(scheduler);
        return NULL;
    }
    return scheduler;
}

static cdc_scheduler *make_tree(int reverse_leaf_add_order,
                                int permuted_ring_inputs,
                                uint8_t config_digest[CDC_DIGEST_SIZE]) {
    return make_tree_with_event_limit(
        reverse_leaf_add_order, permuted_ring_inputs, 64, config_digest);
}

static void fill_tree_events(cdc_scheduler_observation events[8],
                             uint64_t logical_clock, uint64_t window_start,
                             uint64_t window_end, uint8_t seed_delta) {
    size_t i;
    for (i = 0; i < 4; i++) {
        events[i] = observation("leaf-a", LEAF_A_MEMBERS[i], 0.25,
                                logical_clock, window_start, window_end,
                                (uint8_t)(10 + i + seed_delta));
        events[i + 4] =
            observation("leaf-b", LEAF_B_MEMBERS[i], -0.5,
                        logical_clock, window_start, window_end,
                        (uint8_t)(40 + i + seed_delta));
    }
}

static int publish_order(cdc_scheduler *scheduler,
                         const cdc_scheduler_observation events[8],
                         const size_t order[8]) {
    size_t i;
    for (i = 0; i < 8; i++) {
        if (cdc_scheduler_publish(scheduler, &events[order[i]],
                                  events[order[i]].seal_time) !=
            CDC_SCHEDULER_OK) {
            return 0;
        }
    }
    return 1;
}

static int test_graph_and_replay_determinism(void) {
    static const size_t ORDER_A[8] = {7, 0, 5, 2, 6, 1, 4, 3};
    static const size_t ORDER_B[8] = {3, 4, 1, 6, 2, 5, 0, 7};
    cdc_scheduler_observation events[8], changed;
    cdc_scheduler_run_report report_a, report_b, report_c;
    cdc_cell_state root_a, root_b, root_c, leaf_a;
    uint8_t config_a[CDC_DIGEST_SIZE], config_b[CDC_DIGEST_SIZE];
    uint8_t config_c[CDC_DIGEST_SIZE];
    cdc_scheduler *a = make_tree(0, 0, config_a);
    cdc_scheduler *b = make_tree(1, 0, config_b);
    cdc_scheduler *c = make_tree(0, 1, config_c);
    cdc_scheduler_cell_spec extra =
        leaf_spec("late", LEAF_A_MEMBERS, LEAF_A_SUCCESSORS, 1000);

    CHECK(a && b && c);
    CHECK(memcmp(config_a, config_b, CDC_DIGEST_SIZE) == 0 &&
          memcmp(config_a, config_c, CDC_DIGEST_SIZE) == 0);
    CHECK(cdc_scheduler_add_cell(a, &extra) == CDC_SCHEDULER_ECONFLICT);

    fill_tree_events(events, 10, 90, 100, 0);
    CHECK(publish_order(a, events, ORDER_A));
    CHECK(publish_order(b, events, ORDER_B));
    CHECK(publish_order(c, events, ORDER_A));
    CHECK(cdc_scheduler_publish(a, &events[0], 100) ==
          CDC_SCHEDULER_HOLD_DUPLICATE);
    changed = events[0];
    changed.phase = 1.0;
    CHECK(cdc_scheduler_publish(a, &changed, 100) ==
          CDC_SCHEDULER_ECONFLICT);

    CHECK(cdc_scheduler_drain(a, 64, &report_a) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(b, 64, &report_b) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(c, 64, &report_c) == CDC_SCHEDULER_OK);
    CHECK(report_a.events_processed == 8 && report_a.leaf_updates == 2 &&
          report_a.recursive_updates == 1 &&
          report_a.queued_remaining == 0);
    CHECK(memcmp(report_a.configuration_digest, config_a,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(memcmp(report_a.execution_digest, report_b.execution_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(report_a.execution_digest, report_c.execution_digest,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(cdc_scheduler_get_state(a, "root", &root_a) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(b, "root", &root_b) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(c, "root", &root_c) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(a, "leaf-a", &leaf_a) ==
          CDC_SCHEDULER_OK);
    CHECK(memcmp(root_a.state_digest, root_b.state_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(root_a.state_digest, root_c.state_digest,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(root_a.logical_clock == 10 && root_a.generation == 1);
    CHECK(cdc_scheduler_publish(a, &events[0], 100) ==
          CDC_SCHEDULER_HOLD_DUPLICATE);
    CHECK(cdc_scheduler_queued_count(a) == 0);
    CHECK(memcmp(root_a.hidden_class_digest, leaf_a.hidden_class_digest,
                 CDC_DIGEST_SIZE) != 0);

    cdc_scheduler_destroy(a);
    cdc_scheduler_destroy(b);
    cdc_scheduler_destroy(c);
    return 1;
}

static int test_recursive_barrier_and_hidden_provenance(void) {
    cdc_scheduler_observation events[8], changed_events[8];
    cdc_scheduler_run_report report;
    cdc_cell_state root_before, root_after;
    uint8_t config_digest[CDC_DIGEST_SIZE];
    cdc_scheduler *scheduler = make_tree(0, 0, config_digest);
    size_t i;
    CHECK(scheduler);
    fill_tree_events(events, 10, 90, 100, 0);
    for (i = 0; i < 4; i++) {
        CHECK(cdc_scheduler_publish(scheduler, &events[i], 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 64, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "root", &root_before) ==
          CDC_SCHEDULER_HOLD_INCOMPLETE);
    for (i = 4; i < 8; i++) {
        CHECK(cdc_scheduler_publish(scheduler, &events[i], 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 64, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "root", &root_before) ==
          CDC_SCHEDULER_OK);

    fill_tree_events(changed_events, 20, 101, 110, 90);
    for (i = 0; i < 8; i++) {
        CHECK(cdc_scheduler_publish(scheduler, &changed_events[i], 110) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 64, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "root", &root_after) ==
          CDC_SCHEDULER_OK);
    CHECK(fabs(root_before.amplitude - root_after.amplitude) < 1e-12);
    CHECK(fabs(root_before.mean_phase - root_after.mean_phase) < 1e-12);
    CHECK(memcmp(root_before.hidden_class_digest,
                 root_after.hidden_class_digest, CDC_DIGEST_SIZE) != 0);
    CHECK(memcmp(root_before.state_digest, root_after.state_digest,
                 CDC_DIGEST_SIZE) != 0);
    CHECK(memcmp(root_after.previous_state_digest, root_before.state_digest,
                 CDC_DIGEST_SIZE) == 0);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

static cdc_scheduler *make_single_leaf(size_t event_limit,
                                       uint8_t digest[CDC_DIGEST_SIZE]) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec leaf =
        leaf_spec("leaf-a", LEAF_A_MEMBERS, LEAF_A_SUCCESSORS, 1000);
    cdc_scheduler *scheduler;
    memset(&config, 0, sizeof(config));
    config.cell_limit = 2;
    config.event_limit = event_limit;
    config.maximum_depth = 2;
    scheduler = cdc_scheduler_create(&config);
    if (!scheduler) {
        return NULL;
    }
    if (cdc_scheduler_add_cell(scheduler, &leaf) != CDC_SCHEDULER_OK ||
        cdc_scheduler_seal(scheduler, digest) != CDC_SCHEDULER_OK) {
        cdc_scheduler_destroy(scheduler);
        return NULL;
    }
    return scheduler;
}

static int test_causal_blocking_and_limits(void) {
    cdc_scheduler_observation old_events[4], new_events[4], stale;
    cdc_scheduler_run_report report;
    cdc_cell_state state;
    uint8_t digest[CDC_DIGEST_SIZE];
    cdc_scheduler *scheduler = make_single_leaf(16, digest);
    cdc_scheduler *bounded = make_single_leaf(4, digest);
    size_t i;
    CHECK(scheduler && bounded);
    for (i = 0; i < 4; i++) {
        old_events[i] = observation("leaf-a", LEAF_A_MEMBERS[i], 0.1, 10,
                                    90, 100, (uint8_t)(1 + i));
        new_events[i] = observation("leaf-a", LEAF_A_MEMBERS[i], 0.2, 20,
                                    101, 110, (uint8_t)(20 + i));
    }
    CHECK(cdc_scheduler_publish(scheduler, &old_events[0], 100) ==
          CDC_SCHEDULER_OK);
    for (i = 0; i < 4; i++) {
        CHECK(cdc_scheduler_publish(scheduler, &new_events[i], 110) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 16, &report) ==
          CDC_SCHEDULER_HOLD_INCOMPLETE);
    CHECK(report.events_processed == 1 && report.queued_remaining == 4);
    for (i = 1; i < 4; i++) {
        CHECK(cdc_scheduler_publish(scheduler, &old_events[i], 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 16, &report) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &state) ==
          CDC_SCHEDULER_OK);
    CHECK(state.logical_clock == 20 && state.generation == 2);

    stale = observation("leaf-a", 1, 0.0, 30, 111, 120, 90);
    CHECK(cdc_scheduler_publish(scheduler, &stale, 221) ==
          CDC_SCHEDULER_HOLD_STALE);
    CHECK(cdc_scheduler_queued_count(scheduler) == 0);

    CHECK(cdc_scheduler_publish(bounded, &old_events[0], 100) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_publish(bounded, &new_events[0], 110) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_publish(bounded, &new_events[1], 110) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_publish(bounded, &new_events[2], 110) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_publish(bounded, &new_events[3], 110) ==
          CDC_SCHEDULER_HOLD_LIMIT);
    CHECK(cdc_scheduler_queued_count(bounded) == 4);
    cdc_scheduler_destroy(scheduler);
    cdc_scheduler_destroy(bounded);
    return 1;
}

static int test_phase_slip_evidence(void) {
    static const double WINDING_ONE[4] = {
        0.0, M_PI / 2.0, M_PI, 3.0 * M_PI / 2.0};
    cdc_scheduler_observation first[4], zero[4];
    cdc_topology_witness bad, good;
    cdc_scheduler_run_report report;
    cdc_cell_state before, after;
    uint8_t digest[CDC_DIGEST_SIZE];
    cdc_scheduler *scheduler = make_single_leaf(24, digest);
    size_t i;
    CHECK(scheduler);
    for (i = 0; i < 4; i++) {
        first[i] = observation("leaf-a", LEAF_A_MEMBERS[i],
                               WINDING_ONE[i], 10, 90, 100,
                               (uint8_t)(2 + i));
        zero[i] = observation("leaf-a", LEAF_A_MEMBERS[i], 0.0, 20,
                              101, 110, (uint8_t)(12 + i));
        CHECK(cdc_scheduler_publish(scheduler, &first[i], 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 24, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &before) ==
          CDC_SCHEDULER_OK);
    CHECK(before.winding == 1);
    for (i = 0; i < 4; i++) {
        CHECK(cdc_scheduler_publish(scheduler, &zero[i], 110) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 24, &report) == CDC_SCHEDULER_OK);
    CHECK(report.frames_held > 0);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &after) ==
          CDC_SCHEDULER_OK);
    CHECK(after.logical_clock == 10);

    memset(&bad, 0, sizeof(bad));
    bad.kind = CDC_TOPOLOGY_EVENT_PHASE_SLIP;
    bad.from_member = 1;
    bad.to_member = 3;
    bad.logical_clock = 15;
    fill_digest(bad.evidence_digest, 70);
    CHECK(cdc_scheduler_publish_witness(scheduler, "leaf-a", 20, &bad) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(scheduler, 24, &report) ==
          CDC_SCHEDULER_ETRANSITION);
    CHECK(report.rejected_events == 1 && report.queued_remaining == 0);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &after) ==
          CDC_SCHEDULER_OK);
    CHECK(after.logical_clock == 10);

    good = bad;
    good.to_member = 2;
    fill_digest(good.evidence_digest, 80);
    CHECK(cdc_scheduler_publish_witness(scheduler, "leaf-a", 20, &good) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(scheduler, 24, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &after) ==
          CDC_SCHEDULER_OK);
    CHECK(after.logical_clock == 20 && after.winding == 0 &&
          after.transition_kind == CDC_TOPOLOGY_EVENT_PHASE_SLIP);
    CHECK(memcmp(after.transition_evidence_digest, good.evidence_digest,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(cdc_scheduler_publish_witness(scheduler, "leaf-a", 20, &good) ==
          CDC_SCHEDULER_HOLD_DUPLICATE);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

static cdc_scheduler *make_frame_two_epoch(
    const cdc_scheduler_epoch_state *previous,
    const uint8_t predecessor_digest[CDC_DIGEST_SIZE],
    int permuted_ring_inputs,
    uint8_t configuration_digest[CDC_DIGEST_SIZE]) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec spec;
    cdc_scheduler *scheduler;
    memset(&config, 0, sizeof(config));
    config.cell_limit = 2;
    config.event_limit = 24;
    config.maximum_depth = 2;
    scheduler = cdc_scheduler_create(&config);
    if (!scheduler) {
        return NULL;
    }
    memset(&spec, 0, sizeof(spec));
    spec.cell_id = "leaf-a";
    spec.kind = CDC_SCHEDULER_LEAF;
    spec.member_ids = permuted_ring_inputs
                          ? FRAME_TWO_MEMBERS_PERMUTED
                          : FRAME_TWO_MEMBERS;
    spec.successor_ids = permuted_ring_inputs
                             ? FRAME_TWO_SUCCESSORS_PERMUTED
                             : FRAME_TWO_SUCCESSORS;
    spec.member_count = 5;
    spec.frame_version = 2;
    spec.reducer_version = 1;
    spec.topology_version = 2;
    spec.stale_after = 100;
    spec.causal_horizon = 1000;
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

static int test_configuration_epoch_frame_change(void) {
    static const double WINDING_ONE[4] = {
        0.0, M_PI / 2.0, M_PI, 3.0 * M_PI / 2.0};
    cdc_scheduler *first;
    cdc_scheduler *next_a;
    cdc_scheduler *next_b;
    cdc_scheduler_observation initial[4];
    cdc_scheduler_observation changed[5];
    cdc_scheduler_run_report report_a, report_b;
    cdc_cell_state prior, after_a, after_b;
    cdc_scheduler_epoch_state prior_epoch;
    cdc_topology_witness witness;
    uint8_t first_digest[CDC_DIGEST_SIZE];
    uint8_t next_digest_a[CDC_DIGEST_SIZE];
    uint8_t next_digest_b[CDC_DIGEST_SIZE];
    size_t i;

    first = make_single_leaf(24, first_digest);
    CHECK(first);
    for (i = 0; i < 4; i++) {
        initial[i] =
            observation("leaf-a", LEAF_A_MEMBERS[i], WINDING_ONE[i], 10,
                        90, 100, (uint8_t)(160 + i));
        CHECK(cdc_scheduler_publish(first, &initial[i], 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(first, 24, &report_a) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(first, "leaf-a", &prior) ==
              CDC_SCHEDULER_OK &&
          prior.frame_version == 1 && prior.winding == 1);
    CHECK(cdc_scheduler_get_epoch_state(first, "leaf-a", &prior_epoch) ==
              CDC_SCHEDULER_OK &&
          memcmp(prior_epoch.state.state_digest, prior.state_digest,
                 CDC_DIGEST_SIZE) == 0);

    memset(&witness, 0, sizeof(witness));
    witness.kind = CDC_TOPOLOGY_EVENT_FRAME_CHANGE;
    witness.from_member = 4;
    witness.to_member = 5;
    witness.logical_clock = 15;
    fill_digest(witness.evidence_digest, 170);
    CHECK(cdc_scheduler_publish_witness(first, "leaf-a", 20, &witness) ==
          CDC_SCHEDULER_ETRANSITION);

    next_a = make_frame_two_epoch(
        &prior_epoch, first_digest, 0, next_digest_a);
    next_b = make_frame_two_epoch(
        &prior_epoch, first_digest, 1, next_digest_b);
    CHECK(next_a && next_b);
    CHECK(cdc_scheduler_is_pristine(next_a) &&
          cdc_scheduler_is_pristine(next_b) &&
          memcmp(next_digest_a, next_digest_b, CDC_DIGEST_SIZE) == 0 &&
          memcmp(first_digest, next_digest_a, CDC_DIGEST_SIZE) != 0);

    for (i = 0; i < 5; i++) {
        changed[i] =
            observation("leaf-a", FRAME_TWO_MEMBERS[i], 0.0, 20, 101,
                        110, (uint8_t)(180 + i));
        changed[i].frame_version = 2;
        CHECK(cdc_scheduler_publish(next_a, &changed[i], 110) ==
              CDC_SCHEDULER_OK);
    }
    for (i = 5; i > 0; i--) {
        CHECK(cdc_scheduler_publish(next_b, &changed[i - 1], 110) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(next_a, 24, &report_a) ==
              CDC_SCHEDULER_OK &&
          report_a.frames_held > 0);
    CHECK(cdc_scheduler_get_state(next_a, "leaf-a", &after_a) ==
              CDC_SCHEDULER_OK &&
          memcmp(after_a.state_digest, prior.state_digest,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(cdc_scheduler_publish_witness(
              next_a, "leaf-a", 20, &witness) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_publish_witness(
              next_b, "leaf-a", 20, &witness) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(next_a, 24, &report_a) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(next_b, 24, &report_b) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(next_a, "leaf-a", &after_a) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_state(next_b, "leaf-a", &after_b) ==
              CDC_SCHEDULER_OK);
    CHECK(after_a.generation == prior.generation + 1 &&
          after_a.frame_version == 2 && after_a.topology_version == 2 &&
          after_a.logical_clock == 20 && after_a.member_count == 5 &&
          after_a.transition_kind == CDC_TOPOLOGY_EVENT_FRAME_CHANGE &&
          after_a.transition_from_member == 4 &&
          after_a.transition_to_member == 5 &&
          memcmp(after_a.previous_state_digest, prior.state_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(after_a.state_digest, after_b.state_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(report_a.execution_digest, report_b.execution_digest,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(!cdc_scheduler_is_pristine(next_a));

    cdc_scheduler_destroy(first);
    cdc_scheduler_destroy(next_a);
    cdc_scheduler_destroy(next_b);
    return 1;
}

static cdc_scheduler *create_mixed_epoch_tree_unsealed(
    int swap_root_children) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec changed;
    cdc_scheduler_cell_spec unchanged =
        leaf_spec("leaf-b", LEAF_B_MEMBERS, LEAF_B_SUCCESSORS, 1000);
    cdc_scheduler_cell_spec root = root_spec(0);
    cdc_scheduler *scheduler;

    memset(&config, 0, sizeof(config));
    config.cell_limit = 8;
    config.event_limit = 64;
    config.maximum_depth = 4;
    scheduler = cdc_scheduler_create(&config);
    if (!scheduler) {
        return NULL;
    }
    memset(&changed, 0, sizeof(changed));
    changed.cell_id = "leaf-a";
    changed.kind = CDC_SCHEDULER_LEAF;
    changed.member_ids = FRAME_TWO_MEMBERS;
    changed.successor_ids = FRAME_TWO_SUCCESSORS;
    changed.member_count = 5;
    changed.frame_version = 2;
    changed.reducer_version = 1;
    changed.topology_version = 2;
    changed.stale_after = 100;
    changed.causal_horizon = 1000;
    if (swap_root_children) {
        root.child_cell_ids = ROOT_CHILDREN_SWAPPED;
    }
    if (cdc_scheduler_add_cell(scheduler, &changed) != CDC_SCHEDULER_OK ||
        cdc_scheduler_add_cell(scheduler, &unchanged) != CDC_SCHEDULER_OK ||
        cdc_scheduler_add_cell(scheduler, &root) != CDC_SCHEDULER_OK) {
        cdc_scheduler_destroy(scheduler);
        return NULL;
    }
    return scheduler;
}

static cdc_scheduler *make_mixed_epoch_tree(
    const cdc_scheduler_epoch_state prior_states[3],
    const uint8_t predecessor_digest[CDC_DIGEST_SIZE],
    uint8_t configuration_digest[CDC_DIGEST_SIZE]) {
    cdc_scheduler *scheduler = create_mixed_epoch_tree_unsealed(0);
    if (!scheduler) {
        return NULL;
    }
    if (cdc_scheduler_import_previous_states(
            scheduler, prior_states, 3, predecessor_digest) !=
            CDC_SCHEDULER_OK ||
        cdc_scheduler_seal(scheduler, configuration_digest) !=
            CDC_SCHEDULER_OK) {
        cdc_scheduler_destroy(scheduler);
        return NULL;
    }
    return scheduler;
}

static int test_recursive_mixed_epoch_carry_forward(void) {
    static const size_t ORDER[8] = {7, 0, 5, 2, 6, 1, 4, 3};
    cdc_scheduler *previous;
    cdc_scheduler *next;
    cdc_scheduler *atomic_retry;
    cdc_scheduler *lineage_guard;
    cdc_scheduler *foreign_previous;
    cdc_scheduler *mixed_lineage_guard;
    cdc_scheduler *child_rebinding_guard;
    cdc_scheduler *state_tamper_guard;
    cdc_scheduler_observation old_events[8];
    cdc_scheduler_observation foreign_events[8];
    cdc_scheduler_observation changed_a[5];
    cdc_scheduler_observation unchanged_b[4];
    cdc_scheduler_run_report report;
    cdc_scheduler_epoch_state prior[3];
    cdc_scheduler_epoch_state foreign_prior_b;
    cdc_cell_state carried_b, carried_root;
    cdc_cell_state after_a, after_b, after_root;
    cdc_topology_witness witness;
    cdc_scheduler_epoch_state imports[3];
    cdc_scheduler_epoch_state one_import;
    uint8_t previous_digest[CDC_DIGEST_SIZE];
    uint8_t next_digest[CDC_DIGEST_SIZE];
    uint8_t foreign_digest[CDC_DIGEST_SIZE];
    size_t i;

    previous = make_tree(0, 0, previous_digest);
    CHECK(previous);
    fill_tree_events(old_events, 10, 90, 100, 0);
    CHECK(publish_order(previous, old_events, ORDER));
    CHECK(cdc_scheduler_drain(previous, 64, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_epoch_state(previous, "leaf-a", &prior[0]) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_epoch_state(previous, "leaf-b", &prior[1]) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_epoch_state(previous, "root", &prior[2]) ==
              CDC_SCHEDULER_OK);
    CHECK(memcmp(prior[0].source_configuration_digest, previous_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(prior[1].source_configuration_digest, previous_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(prior[2].source_configuration_digest, previous_digest,
                 CDC_DIGEST_SIZE) == 0);

    foreign_previous =
        make_tree_with_event_limit(0, 0, 63, foreign_digest);
    CHECK(foreign_previous &&
          memcmp(foreign_digest, previous_digest, CDC_DIGEST_SIZE) != 0);
    fill_tree_events(foreign_events, 10, 90, 100, 50);
    CHECK(publish_order(foreign_previous, foreign_events, ORDER));
    CHECK(cdc_scheduler_drain(foreign_previous, 64, &report) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_epoch_state(
              foreign_previous, "leaf-b", &foreign_prior_b) ==
              CDC_SCHEDULER_OK &&
          memcmp(foreign_prior_b.source_configuration_digest,
                 foreign_digest, CDC_DIGEST_SIZE) == 0);

    atomic_retry = create_mixed_epoch_tree_unsealed(0);
    CHECK(atomic_retry);
    memcpy(imports, prior, sizeof(imports));
    imports[1].structure_digest[0] ^= 0x01u;
    CHECK(cdc_scheduler_import_previous_states(
              atomic_retry, imports, 3, previous_digest) ==
              CDC_SCHEDULER_EFRAME &&
          cdc_scheduler_get_state(atomic_retry, "leaf-a", &carried_b) ==
              CDC_SCHEDULER_HOLD_INCOMPLETE);
    memcpy(imports, prior, sizeof(imports));
    CHECK(cdc_scheduler_import_previous_states(
              atomic_retry, imports, 3, previous_digest) ==
          CDC_SCHEDULER_OK);
    cdc_scheduler_destroy(atomic_retry);

    state_tamper_guard = create_mixed_epoch_tree_unsealed(0);
    CHECK(state_tamper_guard);
    memcpy(imports, prior, sizeof(imports));
    imports[1].state.amplitude = 12345.0;
    CHECK(cdc_scheduler_import_previous_states(
              state_tamper_guard, imports, 3, previous_digest) ==
              CDC_SCHEDULER_EARG &&
          cdc_scheduler_get_state(
              state_tamper_guard, "leaf-a", &carried_b) ==
              CDC_SCHEDULER_HOLD_INCOMPLETE);
    cdc_scheduler_destroy(state_tamper_guard);

    child_rebinding_guard = create_mixed_epoch_tree_unsealed(1);
    CHECK(child_rebinding_guard);
    CHECK(cdc_scheduler_import_previous_states(
              child_rebinding_guard, prior, 3, previous_digest) ==
              CDC_SCHEDULER_EFRAME &&
          cdc_scheduler_get_state(
              child_rebinding_guard, "leaf-a", &carried_b) ==
              CDC_SCHEDULER_HOLD_INCOMPLETE);
    cdc_scheduler_destroy(child_rebinding_guard);

    mixed_lineage_guard = create_mixed_epoch_tree_unsealed(0);
    CHECK(mixed_lineage_guard);
    memcpy(imports, prior, sizeof(imports));
    imports[1] = foreign_prior_b;
    CHECK(cdc_scheduler_import_previous_states(
              mixed_lineage_guard, imports, 3, previous_digest) ==
              CDC_SCHEDULER_EFRAME &&
          cdc_scheduler_get_state(
              mixed_lineage_guard, "leaf-a", &carried_b) ==
              CDC_SCHEDULER_HOLD_INCOMPLETE);
    cdc_scheduler_destroy(mixed_lineage_guard);

    lineage_guard = create_mixed_epoch_tree_unsealed(0);
    CHECK(lineage_guard);
    one_import = prior[0];
    CHECK(cdc_scheduler_import_previous_states(
              lineage_guard, &one_import, 1, previous_digest) ==
          CDC_SCHEDULER_OK);
    one_import = prior[1];
    CHECK(cdc_scheduler_import_previous_states(
              lineage_guard, &one_import, 1, foreign_digest) ==
          CDC_SCHEDULER_ECONFLICT);
    cdc_scheduler_destroy(lineage_guard);

    next = make_mixed_epoch_tree(prior, previous_digest, next_digest);
    CHECK(next && cdc_scheduler_is_pristine(next) &&
          memcmp(previous_digest, next_digest, CDC_DIGEST_SIZE) != 0);
    CHECK(cdc_scheduler_get_state(next, "leaf-b", &carried_b) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_state(next, "root", &carried_root) ==
              CDC_SCHEDULER_OK &&
          memcmp(carried_b.state_digest, prior[1].state.state_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(carried_root.state_digest, prior[2].state.state_digest,
                 CDC_DIGEST_SIZE) == 0);

    for (i = 0; i < 5; i++) {
        changed_a[i] =
            observation("leaf-a", FRAME_TWO_MEMBERS[i], 0.0, 20, 101,
                        110, (uint8_t)(200 + i));
        changed_a[i].frame_version = 2;
        CHECK(cdc_scheduler_publish(next, &changed_a[i], 110) ==
              CDC_SCHEDULER_OK);
    }
    for (i = 0; i < 4; i++) {
        unchanged_b[i] =
            observation("leaf-b", LEAF_B_MEMBERS[i], -0.5, 20, 101,
                        110, (uint8_t)(210 + i));
        CHECK(cdc_scheduler_publish(next, &unchanged_b[i], 110) ==
              CDC_SCHEDULER_OK);
    }
    memset(&witness, 0, sizeof(witness));
    witness.kind = CDC_TOPOLOGY_EVENT_FRAME_CHANGE;
    witness.from_member = 4;
    witness.to_member = 5;
    witness.logical_clock = 15;
    fill_digest(witness.evidence_digest, 220);
    CHECK(cdc_scheduler_publish_witness(next, "leaf-a", 20, &witness) ==
          CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_drain(next, 64, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(next, "leaf-a", &after_a) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_state(next, "leaf-b", &after_b) ==
              CDC_SCHEDULER_OK &&
          cdc_scheduler_get_state(next, "root", &after_root) ==
              CDC_SCHEDULER_OK);
    CHECK(after_a.frame_version == 2 &&
          after_a.transition_kind == CDC_TOPOLOGY_EVENT_FRAME_CHANGE &&
          after_a.generation == prior[0].state.generation + 1 &&
          after_b.frame_version == 1 &&
          after_b.generation == prior[1].state.generation + 1 &&
          after_root.frame_version == 1 &&
          after_root.generation == prior[2].state.generation + 1 &&
          after_root.logical_clock == 20 &&
          memcmp(after_b.previous_state_digest,
                 prior[1].state.state_digest,
                 CDC_DIGEST_SIZE) == 0 &&
          memcmp(after_root.previous_state_digest,
                 prior[2].state.state_digest,
                 CDC_DIGEST_SIZE) == 0);

    cdc_scheduler_destroy(previous);
    cdc_scheduler_destroy(foreign_previous);
    cdc_scheduler_destroy(next);
    return 1;
}

typedef struct {
    cdc_scheduler *scheduler;
    cdc_scheduler_observation observation;
    cdc_scheduler_status status;
} publish_thread_args;

static void *publish_thread(void *opaque) {
    publish_thread_args *args = opaque;
    args->status =
        cdc_scheduler_publish(args->scheduler, &args->observation,
                              args->observation.seal_time);
    return NULL;
}

static int test_concurrent_publication(void) {
    publish_thread_args args[4];
    pthread_t threads[4];
    cdc_scheduler_run_report report;
    cdc_cell_state state;
    uint8_t digest[CDC_DIGEST_SIZE];
    cdc_scheduler *scheduler = make_single_leaf(16, digest);
    size_t i;
    CHECK(scheduler);
    for (i = 0; i < 4; i++) {
        args[i].scheduler = scheduler;
        args[i].observation =
            observation("leaf-a", LEAF_A_MEMBERS[i], 0.3, 10, 90, 100,
                        (uint8_t)(100 + i));
        args[i].status = CDC_SCHEDULER_EARG;
        CHECK(pthread_create(&threads[i], NULL, publish_thread, &args[i]) ==
              0);
    }
    for (i = 0; i < 4; i++) {
        CHECK(pthread_join(threads[i], NULL) == 0);
        CHECK(args[i].status == CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 16, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &state) ==
          CDC_SCHEDULER_OK);
    CHECK(state.logical_clock == 10 && state.generation == 1);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    size_t ready;
    int go;
} start_gate;

typedef struct {
    cdc_scheduler *scheduler;
    start_gate *gate;
    cdc_scheduler_observation observation;
    cdc_topology_witness witness;
    int publish_witness;
    int drain;
    cdc_scheduler_status status;
} overlap_thread_args;

static int start_gate_init(start_gate *gate) {
    memset(gate, 0, sizeof(*gate));
    return pthread_mutex_init(&gate->mutex, NULL) == 0 &&
           pthread_cond_init(&gate->condition, NULL) == 0;
}

static void start_gate_wait(start_gate *gate) {
    pthread_mutex_lock(&gate->mutex);
    gate->ready++;
    pthread_cond_broadcast(&gate->condition);
    while (!gate->go) {
        pthread_cond_wait(&gate->condition, &gate->mutex);
    }
    pthread_mutex_unlock(&gate->mutex);
}

static void *overlap_thread(void *opaque) {
    overlap_thread_args *args = opaque;
    start_gate_wait(args->gate);
    if (args->drain) {
        cdc_scheduler_run_report report;
        args->status = cdc_scheduler_drain(args->scheduler, 24, &report);
    } else if (args->publish_witness) {
        args->status = cdc_scheduler_publish_witness(
            args->scheduler, "leaf-a", 20, &args->witness);
    } else {
        args->status =
            cdc_scheduler_publish(args->scheduler, &args->observation,
                                  args->observation.seal_time);
    }
    return NULL;
}

static int test_concurrent_publish_witness_and_drain(void) {
    static const double WINDING_ONE[4] = {
        0.0, M_PI / 2.0, M_PI, 3.0 * M_PI / 2.0};
    overlap_thread_args args[6];
    pthread_t threads[6];
    start_gate gate;
    cdc_scheduler_run_report report;
    cdc_cell_state state;
    uint8_t digest[CDC_DIGEST_SIZE];
    cdc_scheduler *scheduler = make_single_leaf(24, digest);
    size_t i;

    CHECK(scheduler && start_gate_init(&gate));
    for (i = 0; i < 4; i++) {
        cdc_scheduler_observation first =
            observation("leaf-a", LEAF_A_MEMBERS[i], WINDING_ONE[i], 10,
                        90, 100, (uint8_t)(130 + i));
        CHECK(cdc_scheduler_publish(scheduler, &first, 100) ==
              CDC_SCHEDULER_OK);
    }
    CHECK(cdc_scheduler_drain(scheduler, 24, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &state) ==
              CDC_SCHEDULER_OK &&
          state.winding == 1);

    memset(args, 0, sizeof(args));
    for (i = 0; i < 6; i++) {
        args[i].scheduler = scheduler;
        args[i].gate = &gate;
        args[i].status = CDC_SCHEDULER_EARG;
    }
    for (i = 0; i < 4; i++) {
        args[i].observation =
            observation("leaf-a", LEAF_A_MEMBERS[i], 0.0, 20, 101, 110,
                        (uint8_t)(140 + i));
    }
    args[4].publish_witness = 1;
    args[4].witness.kind = CDC_TOPOLOGY_EVENT_PHASE_SLIP;
    args[4].witness.from_member = 1;
    args[4].witness.to_member = 2;
    args[4].witness.logical_clock = 15;
    fill_digest(args[4].witness.evidence_digest, 150);
    args[5].drain = 1;

    for (i = 0; i < 6; i++) {
        CHECK(pthread_create(&threads[i], NULL, overlap_thread, &args[i]) ==
              0);
    }
    pthread_mutex_lock(&gate.mutex);
    while (gate.ready != 6) {
        pthread_cond_wait(&gate.condition, &gate.mutex);
    }
    gate.go = 1;
    pthread_cond_broadcast(&gate.condition);
    pthread_mutex_unlock(&gate.mutex);

    for (i = 0; i < 6; i++) {
        CHECK(pthread_join(threads[i], NULL) == 0);
        if (i < 5) {
            CHECK(args[i].status == CDC_SCHEDULER_OK);
        } else {
            CHECK(args[i].status == CDC_SCHEDULER_OK ||
                  args[i].status == CDC_SCHEDULER_HOLD_INCOMPLETE);
        }
    }
    CHECK(cdc_scheduler_drain(scheduler, 24, &report) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_get_state(scheduler, "leaf-a", &state) ==
              CDC_SCHEDULER_OK &&
          state.logical_clock == 20 && state.winding == 0 &&
          state.transition_kind == CDC_TOPOLOGY_EVENT_PHASE_SLIP);

    pthread_cond_destroy(&gate.condition);
    pthread_mutex_destroy(&gate.mutex);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

static int test_graph_refusals(void) {
    cdc_scheduler_config config;
    cdc_scheduler_cell_spec a =
        leaf_spec("leaf-a", LEAF_A_MEMBERS, LEAF_A_SUCCESSORS, 1000);
    cdc_scheduler_cell_spec b =
        leaf_spec("leaf-b", LEAF_B_MEMBERS, LEAF_B_SUCCESSORS, 1000);
    cdc_scheduler_cell_spec root = root_spec(0);
    cdc_scheduler_cell_spec second_parent = root_spec(0);
    cdc_scheduler_cell_spec too_deep;
    const char *const deep_children[2] = {"root", "leaf-a"};
    uint64_t deep_members[2] = {201, 202};
    uint64_t deep_successors[2] = {202, 201};
    uint8_t digest[CDC_DIGEST_SIZE];
    cdc_scheduler *scheduler;
    memset(&config, 0, sizeof(config));
    config.cell_limit = 5;
    config.event_limit = 8;
    config.maximum_depth = 1;
    scheduler = cdc_scheduler_create(&config);
    CHECK(scheduler);
    CHECK(cdc_scheduler_add_cell(scheduler, &a) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_add_cell(scheduler, &b) == CDC_SCHEDULER_OK);
    CHECK(cdc_scheduler_add_cell(scheduler, &root) == CDC_SCHEDULER_OK);
    second_parent.cell_id = "second-parent";
    CHECK(cdc_scheduler_add_cell(scheduler, &second_parent) ==
          CDC_SCHEDULER_ECYCLE);

    memset(&too_deep, 0, sizeof(too_deep));
    too_deep.cell_id = "too-deep";
    too_deep.kind = CDC_SCHEDULER_COMPOSITE;
    too_deep.member_ids = deep_members;
    too_deep.successor_ids = deep_successors;
    too_deep.child_cell_ids = deep_children;
    too_deep.member_count = 2;
    too_deep.frame_version = 1;
    too_deep.reducer_version = 1;
    too_deep.topology_version = 1;
    too_deep.stale_after = 100;
    too_deep.causal_horizon = 1000;
    CHECK(cdc_scheduler_add_cell(scheduler, &too_deep) ==
          CDC_SCHEDULER_ECYCLE);
    CHECK(cdc_scheduler_seal(scheduler, digest) == CDC_SCHEDULER_OK);
    cdc_scheduler_destroy(scheduler);
    return 1;
}

int main(void) {
    if (!test_graph_and_replay_determinism() ||
        !test_recursive_barrier_and_hidden_provenance() ||
        !test_causal_blocking_and_limits() ||
        !test_phase_slip_evidence() ||
        !test_configuration_epoch_frame_change() ||
        !test_recursive_mixed_epoch_carry_forward() ||
        !test_concurrent_publication() ||
        !test_concurrent_publish_witness_and_drain() ||
        !test_graph_refusals()) {
        return 1;
    }
    puts("RFTC scheduler PASS: sealed-graph deterministic-replay "
         "recursive-barrier witnessed-transition "
         "atomic-mixed-frame-epoch-migration "
         "publish+witness+drain-concurrency");
    return 0;
}

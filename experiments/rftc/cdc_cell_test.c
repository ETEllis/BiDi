#include "../../runtime/cdc_cell.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr)) {                                                         \
            fprintf(stderr, "cell check failed at %s:%d: %s\n", __FILE__,     \
                    __LINE__, #expr);                                          \
            return 0;                                                          \
        }                                                                      \
    } while (0)

static void fill_digest(uint8_t digest[CDC_DIGEST_SIZE], uint8_t seed) {
    size_t i;
    for (i = 0; i < CDC_DIGEST_SIZE; i++) {
        digest[i] = (uint8_t)(seed + i + 1);
    }
}

static void fill_ring(cdc_frame_observation *observations, size_t count,
                      const double *phases, uint64_t observed_at,
                      uint64_t logical_clock, uint8_t digest_seed,
                      int reverse) {
    size_t i;
    for (i = 0; i < count; i++) {
        observations[i].member_id = (uint64_t)i + 1;
        observations[i].successor_id =
            reverse ? (i == 0 ? (uint64_t)count : (uint64_t)i)
                    : (uint64_t)((i + 1) % count) + 1;
        observations[i].phase = phases[i];
        observations[i].observed_at = observed_at;
        observations[i].logical_clock = logical_clock;
        fill_digest(observations[i].source_digest,
                    (uint8_t)(digest_seed + i * 7));
    }
}

static cdc_frame_config frame_config(uint64_t frame_version,
                                     uint64_t topology_version,
                                     uint64_t window_start,
                                     uint64_t window_end,
                                     size_t maximum_members) {
    cdc_frame_config config;
    memset(&config, 0, sizeof(config));
    config.frame_id = "cell-frame";
    config.frame_version = frame_version;
    config.reducer_version = 1;
    config.topology_version = topology_version;
    config.window_start = window_start;
    config.window_end = window_end;
    config.stale_after = 20;
    config.minimum_members = 2;
    config.maximum_members = maximum_members;
    return config;
}

static int test_frame_canonicalization_and_refusal(void) {
    static const double PHASES[4] = {0.0, M_PI / 2.0, M_PI,
                                      3.0 * M_PI / 2.0};
    static const size_t ORDER[4] = {2, 0, 3, 1};
    cdc_frame_observation ring[4], shuffled[4], malformed[4];
    cdc_frame_snapshot canonical, permuted;
    cdc_frame_config config = frame_config(1, 1, 90, 100, 4);
    size_t i;

    cdc_frame_snapshot_init(&canonical);
    cdc_frame_snapshot_init(&permuted);
    fill_ring(ring, 4, PHASES, 100, 10, 1, 0);
    for (i = 0; i < 4; i++) {
        shuffled[i] = ring[ORDER[i]];
    }
    CHECK(cdc_frame_seal(&config, ring, 4, 100, &canonical) ==
          CDC_FRAME_OK);
    CHECK(cdc_frame_seal(&config, shuffled, 4, 100, &permuted) ==
          CDC_FRAME_OK);
    CHECK(memcmp(canonical.snapshot_digest, permuted.snapshot_digest,
                 CDC_DIGEST_SIZE) == 0);
    CHECK(canonical.observations[0].member_id == 1 &&
          canonical.observations[3].successor_id == 1);

    memcpy(malformed, ring, sizeof(malformed));
    malformed[3].member_id = malformed[2].member_id;
    cdc_frame_snapshot_free(&permuted);
    CHECK(cdc_frame_seal(&config, malformed, 4, 100, &permuted) ==
          CDC_FRAME_EDUPLICATE);

    memcpy(malformed, ring, sizeof(malformed));
    malformed[0].successor_id = 2;
    malformed[1].successor_id = 1;
    malformed[2].successor_id = 4;
    malformed[3].successor_id = 3;
    CHECK(cdc_frame_seal(&config, malformed, 4, 100, &permuted) ==
          CDC_FRAME_ETOPOLOGY);

    memcpy(malformed, ring, sizeof(malformed));
    malformed[3].logical_clock = 9;
    CHECK(cdc_frame_seal(&config, malformed, 4, 100, &permuted) ==
          CDC_FRAME_ECLOCK);

    config.stale_after = 1;
    memcpy(malformed, ring, sizeof(malformed));
    for (i = 0; i < 4; i++) {
        malformed[i].observed_at = 98;
    }
    CHECK(cdc_frame_seal(&config, malformed, 4, 100, &permuted) ==
          CDC_FRAME_HOLD_STALE);

    cdc_frame_snapshot_free(&canonical);
    cdc_frame_snapshot_free(&permuted);
    return 1;
}

static int test_topology_and_cell_transitions(void) {
    static const double WINDING_ONE[4] = {
        0.0, M_PI / 2.0, M_PI, 3.0 * M_PI / 2.0};
    static const double DEFORMED_ONE[4] = {
        0.1, M_PI / 2.0 - 0.1, M_PI + 0.05,
        3.0 * M_PI / 2.0 + 0.05};
    static const double WINDING_ZERO[4] = {0.0, 0.0, 0.0, 0.0};
    static const double FRAME_TWO[5] = {
        0.0, 2.0 * M_PI / 5.0, 4.0 * M_PI / 5.0,
        6.0 * M_PI / 5.0, 8.0 * M_PI / 5.0};
    cdc_frame_observation first_obs[4], deformed_obs[4], zero_obs[4],
        reverse_obs[4], frame_two_obs[5], frame_three_obs[5], alias_obs[4];
    cdc_frame_snapshot first, deformed, zero, reverse, frame_two, frame_three,
        alias;
    cdc_frame_config config;
    cdc_topology_state topology, reverse_topology;
    cdc_topology_witness witness;
    cdc_cell_state state_one, state_two, state_zero, state_frame_two,
        state_frame_three, alias_state;

    cdc_frame_snapshot_init(&first);
    cdc_frame_snapshot_init(&deformed);
    cdc_frame_snapshot_init(&zero);
    cdc_frame_snapshot_init(&reverse);
    cdc_frame_snapshot_init(&frame_two);
    cdc_frame_snapshot_init(&frame_three);
    cdc_frame_snapshot_init(&alias);

    config = frame_config(1, 1, 90, 100, 5);
    fill_ring(first_obs, 4, WINDING_ONE, 100, 10, 2, 0);
    CHECK(cdc_frame_seal(&config, first_obs, 4, 100, &first) ==
          CDC_FRAME_OK);
    CHECK(cdc_topology_classify(&first, &topology) == CDC_TOPOLOGY_OK);
    CHECK(topology.winding == 1);
    CHECK(cdc_cell_reduce("leaf-a", &first, 100, NULL, NULL, &state_one) ==
          CDC_CELL_OK);
    CHECK(state_one.generation == 1 && state_one.winding == 1);

    config.window_start = 101;
    config.window_end = 110;
    fill_ring(deformed_obs, 4, DEFORMED_ONE, 110, 20, 12, 0);
    CHECK(cdc_frame_seal(&config, deformed_obs, 4, 110, &deformed) ==
          CDC_FRAME_OK);
    CHECK(cdc_cell_reduce("leaf-a", &deformed, 100, &state_one, NULL,
                          &state_two) == CDC_CELL_OK);
    CHECK(state_two.generation == 2 && state_two.winding == 1 &&
          memcmp(state_two.previous_state_digest, state_one.state_digest,
                 CDC_DIGEST_SIZE) == 0);

    fill_ring(reverse_obs, 4, DEFORMED_ONE, 110, 20, 30, 1);
    CHECK(cdc_frame_seal(&config, reverse_obs, 4, 110, &reverse) ==
          CDC_FRAME_OK);
    CHECK(cdc_topology_classify(&reverse, &reverse_topology) ==
          CDC_TOPOLOGY_OK);
    CHECK(memcmp(topology.oriented_boundary_digest,
                 reverse_topology.oriented_boundary_digest,
                 CDC_DIGEST_SIZE) != 0);
    CHECK(cdc_cell_reduce("leaf-a", &reverse, 100, &state_one, NULL,
                          &alias_state) == CDC_CELL_ETRANSITION);

    config.window_start = 111;
    config.window_end = 120;
    fill_ring(zero_obs, 4, WINDING_ZERO, 120, 30, 40, 0);
    CHECK(cdc_frame_seal(&config, zero_obs, 4, 120, &zero) ==
          CDC_FRAME_OK);
    CHECK(cdc_cell_reduce("leaf-a", &zero, 100, &state_two, NULL,
                          &state_zero) == CDC_CELL_ETRANSITION);
    memset(&witness, 0, sizeof(witness));
    witness.kind = CDC_TOPOLOGY_EVENT_PHASE_SLIP;
    witness.from_member = 1;
    witness.to_member = 2;
    witness.logical_clock = 25;
    fill_digest(witness.evidence_digest, 90);
    CHECK(cdc_cell_reduce("leaf-a", &zero, 100, &state_two, &witness,
                          &state_zero) == CDC_CELL_OK);
    CHECK(state_zero.winding == 0 && state_zero.generation == 3);

    config = frame_config(2, 2, 121, 130, 5);
    config.reducer_version = 2;
    fill_ring(frame_two_obs, 5, FRAME_TWO, 130, 40, 50, 0);
    CHECK(cdc_frame_seal(&config, frame_two_obs, 5, 130, &frame_two) ==
          CDC_FRAME_OK);
    CHECK(cdc_cell_reduce("leaf-a", &frame_two, 100, &state_zero, NULL,
                          &state_frame_two) == CDC_CELL_EFRAME);
    memset(&witness, 0, sizeof(witness));
    witness.kind = CDC_TOPOLOGY_EVENT_FRAME_CHANGE;
    witness.logical_clock = 35;
    fill_digest(witness.evidence_digest, 100);
    CHECK(cdc_cell_reduce("leaf-a", &frame_two, 100, &state_zero, &witness,
                          &state_frame_two) == CDC_CELL_OK);
    CHECK(state_frame_two.frame_version == 2 &&
          state_frame_two.reducer_version == 2 &&
          state_frame_two.member_count == 5 &&
          state_frame_two.generation == 4);

    config = frame_config(3, 2, 131, 140, 5);
    fill_ring(frame_three_obs, 5, FRAME_TWO, 140, 50, 60, 0);
    CHECK(cdc_frame_seal(&config, frame_three_obs, 5, 140, &frame_three) ==
          CDC_FRAME_OK);
    witness.logical_clock = 45;
    CHECK(cdc_cell_reduce("leaf-a", &frame_three, 100, &state_frame_two,
                          &witness, &state_frame_three) ==
          CDC_CELL_EFRAME);

    config = frame_config(1, 1, 90, 100, 4);
    fill_ring(alias_obs, 4, WINDING_ONE, 100, 10, 200, 0);
    CHECK(cdc_frame_seal(&config, alias_obs, 4, 100, &alias) ==
          CDC_FRAME_OK);
    CHECK(cdc_cell_reduce("leaf-a", &alias, 100, NULL, NULL, &alias_state) ==
          CDC_CELL_OK);
    CHECK(fabs(alias_state.amplitude - state_one.amplitude) < 1e-12 &&
          fabs(alias_state.mean_phase - state_one.mean_phase) < 1e-12 &&
          alias_state.winding == state_one.winding &&
          memcmp(alias_state.hidden_class_digest,
                 state_one.hidden_class_digest, CDC_DIGEST_SIZE) != 0 &&
          memcmp(alias_state.state_digest, state_one.state_digest,
                 CDC_DIGEST_SIZE) != 0);

    cdc_frame_snapshot_free(&first);
    cdc_frame_snapshot_free(&deformed);
    cdc_frame_snapshot_free(&zero);
    cdc_frame_snapshot_free(&reverse);
    cdc_frame_snapshot_free(&frame_two);
    cdc_frame_snapshot_free(&frame_three);
    cdc_frame_snapshot_free(&alias);
    return 1;
}

int main(void) {
    if (!test_frame_canonicalization_and_refusal() ||
        !test_topology_and_cell_transitions()) {
        return 1;
    }
    puts("RFTC cell PASS: frame=canonical topology=oriented "
         "sector-change=witnessed hidden-provenance=preserved");
    return 0;
}

/*
 * Reference-Frame Topological Coherence rapid crucible.
 *
 * This executable is deliberately classified as CLASSICAL. It tests the
 * missing mechanism between CDC's existing phase flow and its guarded
 * discrete commit: whether many phase-bearing nodes can be reduced to a
 * stable, actionable logical boundary while multiple internal microstates
 * remain hidden behind the same boundary observation.
 *
 * It does not claim quantum superposition, entanglement, or quantum
 * advantage. The final causal-cut screen makes that boundary executable.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../../runtime/cdc_barrier.h"
#include "../../runtime/cdc_digest.h"
#include "../../runtime/cdc_receipt.h"
#include "../../runtime/cdc_rftc.h"
#include "../../runtime/cdc_shared_record.h"
#include "../../runtime/cdc_store.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

enum { NODE_COUNT = 32 };

typedef struct {
    uint64_t state;
} Rng;

typedef struct {
    double r;
    double psi;
} OrderParameter;

typedef struct {
    int seeds;
    double uncoupled_r;
    double coupled_r;
    double separation;
    double critical_coupling;
    double precritical_r;
    double critical_r;
    double max_regression;
    int pass;
} SyncResult;

typedef struct {
    int seeds;
    double moderate_preservation;
    double shuffled_preservation;
    double mean_slip_threshold;
    int pass;
} TopologyResult;

typedef struct {
    int seeds;
    double macro_match_rate;
    double digest_difference_rate;
    double frame_version_difference_rate;
    int pass;
} HiddenResult;

typedef struct {
    int seeds;
    double bidi_recovery_steps;
    double local_recovery_steps;
    double improvement;
    int pass;
} BidiResult;

typedef struct {
    double classical_s;
    double communicating_s;
    int classical_valid;
    int communicating_valid;
    int pass;
} CausalResult;

typedef struct {
    int seeds;
    double subthreshold_total_input;
    double superthreshold_total_input;
    double subthreshold_commit_rate;
    double low_flux_commit_rate;
    double high_flux_commit_rate;
    double committed_count_ratio;
    double mean_excess_delta;
    int buggy_accumulator_would_commit;
    int receipt_parity;
    int pass;
} PacketResult;

typedef struct {
    int seeds;
    double single_fragment_recovery;
    double quorum_recovery;
    double quorum_gain;
    double scrambled_recovery;
    double central_only_recovery;
    int central_only_refused;
    int sealed_fragment_stores;
    int pass;
} RecordResult;

typedef struct {
    const char *profile;
    int seeds;
    int steps;
    uint64_t seed;
    SyncResult sync;
    TopologyResult topology;
    HiddenResult hidden;
    BidiResult bidi;
    CausalResult causal;
    PacketResult packet;
    RecordResult record;
    int pass;
} Report;

static uint64_t rng_next(Rng *rng) {
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return x * UINT64_C(2685821657736338717);
}

static double rng_unit(Rng *rng) {
    return (double)(rng_next(rng) >> 11) *
           (1.0 / 9007199254740992.0);
}

static double rng_signed(Rng *rng) {
    return 2.0 * rng_unit(rng) - 1.0;
}

static Rng rng_for(uint64_t base, uint64_t lane, uint64_t seed_index) {
    Rng rng;
    rng.state = base ^ (UINT64_C(0x9e3779b97f4a7c15) * (lane + 1)) ^
                (UINT64_C(0xbf58476d1ce4e5b9) * (seed_index + 1));
    if (rng.state == 0) {
        rng.state = UINT64_C(0x106689d45497fdb5);
    }
    (void)rng_next(&rng);
    return rng;
}

static double wrap_pi(double value) {
    while (value <= -M_PI) {
        value += 2.0 * M_PI;
    }
    while (value > M_PI) {
        value -= 2.0 * M_PI;
    }
    return value;
}

static OrderParameter order_parameter(const double *theta, int count) {
    OrderParameter out;
    double x = 0.0;
    double y = 0.0;
    int i;
    for (i = 0; i < count; i++) {
        x += cos(theta[i]);
        y += sin(theta[i]);
    }
    x /= (double)count;
    y /= (double)count;
    out.r = hypot(x, y);
    out.psi = atan2(y, x);
    return out;
}

static int ring_winding(const double *theta, int count) {
    double total = 0.0;
    int i;
    for (i = 0; i < count; i++) {
        int next = (i + 1) % count;
        total += wrap_pi(theta[next] - theta[i]);
    }
    return (int)llround(total / (2.0 * M_PI));
}

static void reduce_phases(const double *theta, int count,
                          uint64_t frame_version,
                          cdc_rftc_state *state) {
    uint64_t member_ids[NODE_COUNT];
    cdc_rftc_frame frame;
    int i;

    if (count < 0 || count > NODE_COUNT) {
        fputs("rftc: invalid phase count for digest\n", stderr);
        exit(2);
    }
    for (i = 0; i < count; i++) {
        member_ids[i] = (uint64_t)i;
    }
    memset(&frame, 0, sizeof(frame));
    frame.phases = theta;
    frame.member_ids = member_ids;
    frame.member_count = (size_t)count;
    frame.frame_version = frame_version;
    frame.topology_version = 1;
    frame.logical_clock = 1;
    if (cdc_rftc_reduce(&frame, state) != CDC_RFTC_OK) {
        fputs("rftc: logical-cell reduction failed\n", stderr);
        exit(2);
    }
}

static int verify_rftc_input_boundary(void) {
    const uint64_t member_ids[2] = {0, 1};
    double invalid_phases[2] = {0.0, NAN};
    cdc_rftc_frame frame;
    cdc_rftc_state state;

    memset(&frame, 0, sizeof(frame));
    frame.phases = invalid_phases;
    frame.member_ids = member_ids;
    frame.member_count = 2;
    return cdc_rftc_reduce(&frame, &state) == CDC_RFTC_EARG;
}

static double simulate_global_sync(const double *initial,
                                   const double *omega,
                                   int steps,
                                   double coupling,
                                   uint64_t noise_seed) {
    double theta[NODE_COUNT];
    double next[NODE_COUNT];
    double tail_sum = 0.0;
    int tail_count = 0;
    const double dt = 0.025;
    const double noise = 0.015;
    Rng rng = rng_for(noise_seed, 71, (uint64_t)(coupling * 1000.0));
    int step;
    int i;

    memcpy(theta, initial, sizeof(theta));
    for (step = 0; step < steps; step++) {
        OrderParameter z = order_parameter(theta, NODE_COUNT);
        for (i = 0; i < NODE_COUNT; i++) {
            double field = coupling * z.r * sin(z.psi - theta[i]);
            next[i] = theta[i] +
                      dt * (omega[i] + field + noise * rng_signed(&rng));
        }
        memcpy(theta, next, sizeof(theta));
        if (step >= (steps * 3) / 4) {
            tail_sum += order_parameter(theta, NODE_COUNT).r;
            tail_count++;
        }
    }
    return tail_sum / (double)tail_count;
}

static SyncResult run_sync(const Report *config) {
    static const double couplings[] = {
        0.0, 0.3, 0.6, 0.9, 1.2, 1.5, 1.8, 2.1, 2.4
    };
    enum { COUPLING_COUNT = (int)(sizeof(couplings) / sizeof(couplings[0])) };
    SyncResult result;
    double sums[COUPLING_COUNT];
    double means[COUPLING_COUNT];
    int coupling_index;
    int seed_index;

    memset(&result, 0, sizeof(result));
    memset(sums, 0, sizeof(sums));
    result.seeds = config->seeds;
    for (seed_index = 0; seed_index < config->seeds; seed_index++) {
        double theta[NODE_COUNT];
        double omega[NODE_COUNT];
        Rng rng = rng_for(config->seed, 1, (uint64_t)seed_index);
        int i;
        for (i = 0; i < NODE_COUNT; i++) {
            theta[i] = M_PI * rng_signed(&rng);
            omega[i] = 0.45 * rng_signed(&rng);
        }
        for (coupling_index = 0;
             coupling_index < COUPLING_COUNT;
             coupling_index++) {
            sums[coupling_index] += simulate_global_sync(
                theta, omega, config->steps, couplings[coupling_index],
                config->seed + (uint64_t)seed_index);
        }
    }
    result.critical_coupling = -1.0;
    for (coupling_index = 0;
         coupling_index < COUPLING_COUNT;
         coupling_index++) {
        means[coupling_index] =
            sums[coupling_index] / (double)config->seeds;
        if (coupling_index > 0) {
            double regression =
                means[coupling_index - 1] - means[coupling_index];
            if (regression > result.max_regression) {
                result.max_regression = regression;
            }
        }
        if (result.critical_coupling < 0.0 &&
            means[coupling_index] >= 0.80) {
            result.critical_coupling = couplings[coupling_index];
            result.critical_r = means[coupling_index];
            result.precritical_r =
                coupling_index == 0 ? means[0] : means[coupling_index - 1];
        }
    }
    result.uncoupled_r = means[0];
    result.coupled_r = means[COUPLING_COUNT - 1];
    result.separation = result.coupled_r - result.uncoupled_r;
    result.pass = result.uncoupled_r < 0.35 && result.coupled_r > 0.80 &&
                  result.separation > 0.50 &&
                  result.critical_coupling > couplings[0] &&
                  result.critical_coupling < couplings[COUPLING_COUNT - 1] &&
                  result.precritical_r < 0.80 &&
                  result.critical_r >= 0.80 &&
                  result.max_regression < 0.03;
    return result;
}

static void shuffled_copy(const double *source, double *target, Rng *rng) {
    int i;
    memcpy(target, source, sizeof(double) * NODE_COUNT);
    for (i = NODE_COUNT - 1; i > 0; i--) {
        int j = (int)(rng_unit(rng) * (double)(i + 1));
        double tmp = target[i];
        target[i] = target[j];
        target[j] = tmp;
    }
}

static TopologyResult run_topology(const Report *config) {
    TopologyResult result;
    int moderate_ok = 0;
    int shuffled_ok = 0;
    double slip_sum = 0.0;
    int seed_index;

    memset(&result, 0, sizeof(result));
    result.seeds = config->seeds;
    for (seed_index = 0; seed_index < config->seeds; seed_index++) {
        double theta[NODE_COUNT];
        double perturbed[NODE_COUNT];
        double shuffled[NODE_COUNT];
        double slip[NODE_COUNT];
        double threshold = 2.0 * M_PI;
        Rng rng = rng_for(config->seed, 2, (uint64_t)seed_index);
        int i;
        int step;

        for (i = 0; i < NODE_COUNT; i++) {
            theta[i] = (2.0 * M_PI * (double)i) / (double)NODE_COUNT +
                       0.04 * rng_signed(&rng);
            perturbed[i] = theta[i] + 0.18 * rng_signed(&rng);
        }
        if (ring_winding(perturbed, NODE_COUNT) == 1) {
            moderate_ok++;
        }

        shuffled_copy(theta, shuffled, &rng);
        if (ring_winding(shuffled, NODE_COUNT) == 1) {
            shuffled_ok++;
        }

        for (step = 1; step <= 160; step++) {
            double amplitude = (2.0 * M_PI * (double)step) / 160.0;
            memcpy(slip, theta, sizeof(slip));
            slip[0] += amplitude;
            if (ring_winding(slip, NODE_COUNT) != 1) {
                threshold = amplitude;
                break;
            }
        }
        slip_sum += threshold;
    }
    result.moderate_preservation =
        (double)moderate_ok / (double)config->seeds;
    result.shuffled_preservation =
        (double)shuffled_ok / (double)config->seeds;
    result.mean_slip_threshold = slip_sum / (double)config->seeds;
    result.pass = result.moderate_preservation >= 0.99 &&
                  result.shuffled_preservation < 0.60 &&
                  result.mean_slip_threshold > 2.5 &&
                  result.mean_slip_threshold < 3.8;
    return result;
}

static HiddenResult run_hidden(const Report *config) {
    HiddenResult result;
    int macro_matches = 0;
    int digest_differs = 0;
    int version_differs = 0;
    int seed_index;

    memset(&result, 0, sizeof(result));
    result.seeds = config->seeds;
    for (seed_index = 0; seed_index < config->seeds; seed_index++) {
        double a[NODE_COUNT];
        double b[NODE_COUNT];
        cdc_rftc_state state_a;
        cdc_rftc_state state_b;
        cdc_rftc_state state_a_v2;
        Rng rng = rng_for(config->seed, 3, (uint64_t)seed_index);
        double offset = 0.4 * rng_signed(&rng);
        int shift = 1 + (int)(rng_unit(&rng) * (NODE_COUNT - 1));
        OrderParameter za;
        OrderParameter zb;
        int wa;
        int wb;
        int i;

        for (i = 0; i < NODE_COUNT; i++) {
            a[i] = offset +
                   0.23 * sin((2.0 * M_PI * (double)i) /
                              (double)NODE_COUNT) +
                   0.04 * sin((6.0 * M_PI * (double)i) /
                              (double)NODE_COUNT);
        }
        for (i = 0; i < NODE_COUNT; i++) {
            b[i] = a[(i + shift) % NODE_COUNT];
        }
        za = order_parameter(a, NODE_COUNT);
        zb = order_parameter(b, NODE_COUNT);
        wa = ring_winding(a, NODE_COUNT);
        wb = ring_winding(b, NODE_COUNT);
        reduce_phases(a, NODE_COUNT, 1, &state_a);
        reduce_phases(b, NODE_COUNT, 1, &state_b);
        reduce_phases(a, NODE_COUNT, 2, &state_a_v2);

        if (fabs(za.r - zb.r) < 1e-12 &&
            fabs(wrap_pi(za.psi - zb.psi)) < 1e-12 && wa == wb &&
            fabs(state_a.amplitude - state_b.amplitude) < 1e-12 &&
            fabs(wrap_pi(state_a.mean_phase - state_b.mean_phase)) < 1e-12 &&
            state_a.winding == state_b.winding) {
            macro_matches++;
        }
        if (strcmp(state_a.microstate_digest,
                   state_b.microstate_digest) != 0) {
            digest_differs++;
        }
        if (strcmp(state_a.microstate_digest,
                   state_a_v2.microstate_digest) == 0 &&
            strcmp(state_a.state_digest, state_a_v2.state_digest) != 0) {
            version_differs++;
        }
    }
    result.macro_match_rate =
        (double)macro_matches / (double)config->seeds;
    result.digest_difference_rate =
        (double)digest_differs / (double)config->seeds;
    result.frame_version_difference_rate =
        (double)version_differs / (double)config->seeds;
    result.pass = result.macro_match_rate == 1.0 &&
                  result.digest_difference_rate == 1.0 &&
                  result.frame_version_difference_rate == 1.0;
    return result;
}

static int recovery_steps(const double *initial,
                          const double *omega,
                          int max_steps,
                          double global_gain,
                          uint64_t seed) {
    double theta[NODE_COUNT];
    double next[NODE_COUNT];
    const double dt = 0.03;
    const double local_gain = 0.38;
    int stable = 0;
    int step;
    int i;
    Rng rng = rng_for(seed, 88, (uint64_t)(global_gain * 1000.0));

    memcpy(theta, initial, sizeof(theta));
    for (step = 0; step < max_steps; step++) {
        OrderParameter z = order_parameter(theta, NODE_COUNT);
        for (i = 0; i < NODE_COUNT; i++) {
            int left = (i + NODE_COUNT - 1) % NODE_COUNT;
            int right = (i + 1) % NODE_COUNT;
            double local =
                0.5 * local_gain *
                (sin(theta[left] - theta[i]) +
                 sin(theta[right] - theta[i]));
            double downward =
                global_gain * z.r * sin(z.psi - theta[i]);
            next[i] =
                theta[i] +
                dt * (omega[i] + local + downward +
                      0.006 * rng_signed(&rng));
        }
        memcpy(theta, next, sizeof(theta));
        if (order_parameter(theta, NODE_COUNT).r >= 0.90) {
            stable++;
            if (stable >= 10) {
                return step - 9;
            }
        } else {
            stable = 0;
        }
    }
    return max_steps;
}

static BidiResult run_bidi(const Report *config) {
    BidiResult result;
    double bidi_sum = 0.0;
    double local_sum = 0.0;
    const int max_steps = config->steps;
    int seed_index;

    memset(&result, 0, sizeof(result));
    result.seeds = config->seeds;
    for (seed_index = 0; seed_index < config->seeds; seed_index++) {
        double theta[NODE_COUNT];
        double omega[NODE_COUNT];
        Rng rng = rng_for(config->seed, 4, (uint64_t)seed_index);
        int i;
        for (i = 0; i < NODE_COUNT; i++) {
            theta[i] = 0.12 * rng_signed(&rng);
            omega[i] = 0.08 * rng_signed(&rng);
        }
        /* The perturbation is state-local and leaves no privileged global
         * repair hint. The macro order parameter must be reconstructed from
         * the same phases it will constrain on the next step. */
        for (i = 0; i < NODE_COUNT / 2; i++) {
            int index = (i * 7) % NODE_COUNT;
            theta[index] += 2.4 * rng_signed(&rng);
        }
        bidi_sum += (double)recovery_steps(
            theta, omega, max_steps, 1.35,
            config->seed + (uint64_t)seed_index);
        local_sum += (double)recovery_steps(
            theta, omega, max_steps, 0.0,
            config->seed + (uint64_t)seed_index);
    }
    result.bidi_recovery_steps = bidi_sum / (double)config->seeds;
    result.local_recovery_steps = local_sum / (double)config->seeds;
    result.improvement =
        1.0 - result.bidi_recovery_steps / result.local_recovery_steps;
    result.pass = result.bidi_recovery_steps < 0.60 * (double)max_steps &&
                  result.improvement > 0.30;
    return result;
}

static double chsh_value(int communicating, uint64_t seed) {
    long correlation[4] = {0, 0, 0, 0};
    long count[4] = {0, 0, 0, 0};
    const int trials = 20000;
    Rng rng = rng_for(seed, 5, 0);
    int i;
    for (i = 0; i < trials; i++) {
        int x = (int)(rng_next(&rng) & 1u);
        int y = (int)(rng_next(&rng) & 1u);
        int lambda = (int)(rng_next(&rng) & 1u);
        int a_bit = lambda;
        int b_bit = communicating ? (lambda ^ (x & y)) : lambda;
        int a = a_bit ? -1 : 1;
        int b = b_bit ? -1 : 1;
        int setting = x * 2 + y;
        correlation[setting] += (long)(a * b);
        count[setting]++;
    }
    return (double)correlation[0] / (double)count[0] +
           (double)correlation[1] / (double)count[1] +
           (double)correlation[2] / (double)count[2] -
           (double)correlation[3] / (double)count[3];
}

static CausalResult run_causal(const Report *config) {
    CausalResult result;
    memset(&result, 0, sizeof(result));
    result.classical_s = chsh_value(0, config->seed);
    result.communicating_s = chsh_value(1, config->seed);
    result.classical_valid = 1;
    /* The second arm reads the remote setting x while constructing b. Its
     * apparent S=4 is therefore rejected before any physics claim. */
    result.communicating_valid = 0;
    result.pass = fabs(result.classical_s) <= 2.000001 &&
                  result.communicating_s > 3.99 &&
                  !result.communicating_valid;
    return result;
}

typedef struct {
    long accepted;
    long held;
    uint64_t sealed;
    uint64_t events;
    double mean_excess;
    int receipt_parity;
} PacketArm;

typedef struct {
    const char *arm;
    long count;
    long long excess_microunits;
    int malformed;
} PacketReplay;

static cdc_store_status recover_packet(void *context,
                                       uint64_t event_sequence,
                                       uint64_t transaction_sequence,
                                       const void *payload,
                                       size_t payload_size) {
    PacketReplay *replay = context;
    char text[192];
    char arm[40];
    int event;
    long long excess;
    char trailing;

    (void)event_sequence;
    (void)transaction_sequence;
    if (payload_size >= sizeof(text)) {
        replay->malformed = 1;
        return CDC_STORE_ESTATE;
    }
    memcpy(text, payload, payload_size);
    text[payload_size] = '\0';
    if (sscanf(text,
               "rftc-packet/v1 arm=%39s event=%d excessMicrounits=%lld%c",
               arm, &event, &excess, &trailing) != 3 ||
        strcmp(arm, replay->arm) != 0 || event < 0 || excess < 0) {
        replay->malformed = 1;
        return CDC_STORE_ESTATE;
    }
    replay->count++;
    replay->excess_microunits += excess;
    return CDC_STORE_OK;
}

static int run_packet_arm(const char *dir, const char *trits, int attempts,
                          const char *arm, double payload_value,
                          double boundary_threshold, PacketArm *out) {
    cdc_store *store = NULL;
    cdc_store_status status;
    cdc_barrier_result barrier;
    FILE *receipts = NULL;
    char payload[128];
    int recovered = 0;
    int event;
    long parsed_accepted = 0;
    long parsed_held = 0;
    long long excess_microunits =
        (long long)llround(
            fmax(0.0, payload_value - boundary_threshold) * 1000000.0);

    memset(out, 0, sizeof(*out));
    if ((mkdir(dir, 0777) != 0 && errno != EEXIST) ||
        cdc_store_reset(dir) != CDC_STORE_OK ||
        cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
        recovered) {
        return 0;
    }
    receipts = tmpfile();
    if (!receipts) {
        cdc_store_close(store);
        return 0;
    }
    for (event = 0; event < attempts; event++) {
        cdc_receipt receipt;
        int size;
        if (!cdc_barrier_evaluate(trits, &barrier)) {
            fclose(receipts);
            cdc_store_close(store);
            return 0;
        }
        cdc_receipt_init(&receipt);
        snprintf(receipt.kind, sizeof(receipt.kind), "persist");
        snprintf(receipt.job, sizeof(receipt.job), "%s-%d", arm, event);
        snprintf(receipt.op, sizeof(receipt.op), "append");
        snprintf(receipt.trits, sizeof(receipt.trits), "%s", trits);
        snprintf(receipt.balance, sizeof(receipt.balance), "%s",
                 barrier.admissible ? "admissible" : "violated");
        if (barrier.admissible) {
            size = snprintf(payload, sizeof(payload),
                            "rftc-packet/v1 arm=%s event=%d "
                            "excessMicrounits=%lld",
                            arm, event, excess_microunits);
            if (size < 0 || (size_t)size >= sizeof(payload) ||
                cdc_store_stage(store, payload, (size_t)size) != CDC_STORE_OK ||
                cdc_store_commit(store) != CDC_STORE_OK) {
                fclose(receipts);
                cdc_store_close(store);
                return 0;
            }
            receipt.outcome = CDC_OUTCOME_ACCEPTED;
            snprintf(receipt.reason, sizeof(receipt.reason), "none");
            receipt.durable = 1;
            receipt.replay_stable = 0;
            out->accepted++;
        } else {
            receipt.outcome = CDC_OUTCOME_HELD;
            snprintf(receipt.reason, sizeof(receipt.reason),
                     "balance-violation");
            receipt.durable = 0;
            receipt.replay_stable = 1;
            out->held++;
        }
        receipt.declared_hold = !barrier.admissible;
        receipt.sealed = (long)cdc_store_sealed_count(store);
        receipt.events = (long)cdc_store_event_count(store);
        receipt.generation = (long)cdc_store_generation(store);
        if (cdc_receipt_emit(receipts, &receipt) != 1) {
            fclose(receipts);
            cdc_store_close(store);
            return 0;
        }
    }
    status = cdc_store_verify(store);
    out->sealed = cdc_store_sealed_count(store);
    out->events = cdc_store_event_count(store);
    cdc_store_close(store);
    store = NULL;
    if (status != CDC_STORE_OK ||
        cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
        recovered || cdc_store_verify(store) != CDC_STORE_OK ||
        cdc_store_event_count(store) != out->events) {
        fclose(receipts);
        cdc_store_close(store);
        return 0;
    }
    {
        PacketReplay replay;
        memset(&replay, 0, sizeof(replay));
        replay.arm = arm;
        if (cdc_store_visit_events(store, recover_packet, &replay) !=
                CDC_STORE_OK ||
            replay.malformed || replay.count != out->accepted) {
            fclose(receipts);
            cdc_store_close(store);
            return 0;
        }
        out->mean_excess =
            replay.count == 0
                ? 0.0
                : (double)replay.excess_microunits /
                      ((double)replay.count * 1000000.0);
    }
    cdc_store_close(store);

    rewind(receipts);
    for (;;) {
        char line[1024];
        cdc_receipt receipt;
        int parsed;
        if (!fgets(line, sizeof(line), receipts)) {
            break;
        }
        parsed = cdc_receipt_parse(line, &receipt);
        if (parsed != 1 || strcmp(receipt.kind, "persist") != 0 ||
            strcmp(receipt.op, "append") != 0) {
            fclose(receipts);
            return 0;
        }
        if (receipt.outcome == CDC_OUTCOME_ACCEPTED && receipt.durable == 1) {
            parsed_accepted++;
        } else if (receipt.outcome == CDC_OUTCOME_HELD &&
                   receipt.durable == 0) {
            parsed_held++;
        } else {
            fclose(receipts);
            return 0;
        }
    }
    fclose(receipts);
    out->receipt_parity =
        parsed_accepted == out->accepted && parsed_held == out->held &&
        out->events == (uint64_t)out->accepted &&
        out->sealed == (uint64_t)out->accepted;
    return out->receipt_parity;
}

static PacketResult run_packet_threshold(const Report *config) {
    enum {
        SUBTHRESHOLD_PACKETS = 512,
        LOW_FLUX_PACKETS = 32,
        HIGH_FLUX_PACKETS = 128
    };
    const double boundary_threshold = 1.0;
    const double subthreshold_payload = 0.75;
    const double superthreshold_payload = 1.25;
    PacketResult result;
    PacketArm subthreshold;
    PacketArm low_flux;
    PacketArm high_flux;
    int integrated;

    memset(&result, 0, sizeof(result));
    result.seeds = config->seeds;
    result.subthreshold_total_input =
        (double)SUBTHRESHOLD_PACKETS * subthreshold_payload;
    result.superthreshold_total_input =
        (double)LOW_FLUX_PACKETS * superthreshold_payload;

    integrated =
        run_packet_arm("build/rftc/packet-subthreshold", "-+0",
                       SUBTHRESHOLD_PACKETS, "subthreshold",
                       subthreshold_payload, boundary_threshold,
                       &subthreshold) &&
        run_packet_arm("build/rftc/packet-low-flux", "0+-",
                       LOW_FLUX_PACKETS, "low-flux",
                       superthreshold_payload, boundary_threshold,
                       &low_flux) &&
        run_packet_arm("build/rftc/packet-high-flux", "0+-",
                       HIGH_FLUX_PACKETS, "high-flux",
                       superthreshold_payload, boundary_threshold,
                       &high_flux);

    result.subthreshold_commit_rate =
        (double)subthreshold.accepted / (double)SUBTHRESHOLD_PACKETS;
    result.low_flux_commit_rate =
        (double)low_flux.accepted / (double)LOW_FLUX_PACKETS;
    result.high_flux_commit_rate =
        (double)high_flux.accepted / (double)HIGH_FLUX_PACKETS;
    result.committed_count_ratio =
        low_flux.accepted == 0
            ? 0.0
            : (double)high_flux.accepted / (double)low_flux.accepted;
    result.mean_excess_delta =
        fabs(low_flux.mean_excess - high_flux.mean_excess);
    result.buggy_accumulator_would_commit =
        result.subthreshold_total_input >= boundary_threshold;
    result.receipt_parity =
        integrated && subthreshold.receipt_parity &&
        low_flux.receipt_parity && high_flux.receipt_parity;
    result.pass =
        integrated &&
        result.subthreshold_total_input > result.superthreshold_total_input &&
        result.subthreshold_commit_rate == 0.0 &&
        result.low_flux_commit_rate == 1.0 &&
        result.high_flux_commit_rate == 1.0 &&
        result.committed_count_ratio == 4.0 &&
        result.mean_excess_delta < 1e-12 &&
        result.buggy_accumulator_would_commit &&
        result.receipt_parity;
    return result;
}

static int open_fragment_stores(const char *lane,
                                cdc_store *stores[7]) {
    int fragment;
    for (fragment = 0; fragment < 7; fragment++) {
        char dir[160];
        int recovered = 0;
        if (snprintf(dir, sizeof(dir), "build/rftc/record-%s-%d",
                     lane, fragment) < 0 ||
            (mkdir(dir, 0777) != 0 && errno != EEXIST) ||
            cdc_store_reset(dir) != CDC_STORE_OK ||
            cdc_store_open(dir, &stores[fragment], &recovered) !=
                CDC_STORE_OK ||
            recovered) {
            return 0;
        }
    }
    return 1;
}

static void close_fragment_stores(cdc_store *stores[7]) {
    int fragment;
    for (fragment = 0; fragment < 7; fragment++) {
        cdc_store_close(stores[fragment]);
        stores[fragment] = NULL;
    }
}

static int reopen_fragment_stores(const char *lane,
                                  cdc_store *stores[7]) {
    int fragment;
    for (fragment = 0; fragment < 7; fragment++) {
        char dir[160];
        int recovered = 0;
        if (snprintf(dir, sizeof(dir), "build/rftc/record-%s-%d",
                     lane, fragment) < 0 ||
            cdc_store_open(dir, &stores[fragment], &recovered) !=
                CDC_STORE_OK ||
            recovered || cdc_store_verify(stores[fragment]) != CDC_STORE_OK) {
            return 0;
        }
    }
    return 1;
}

static RecordResult run_record_redundancy(const Report *config) {
    enum { FRAGMENT_COUNT = 7, QUORUM = 4 };
    static const int normal_values[5] = {0, 1, 1, 1, 1};
    static const int scrambled_values[7] = {0, 1, 0, 1, 0, 1, 0};
    const uint64_t record_id = UINT64_C(0x5246544300000007);
    const int truth = 1;
    RecordResult result;
    cdc_store *normal[FRAGMENT_COUNT] = {0};
    cdc_store *scrambled[FRAGMENT_COUNT] = {0};
    cdc_store *central_only[FRAGMENT_COUNT] = {0};
    cdc_shared_record_result recovered;
    cdc_shared_record_status status;
    int integrated = 1;
    int fragment;

    memset(&result, 0, sizeof(result));
    result.seeds = config->seeds;
    integrated = open_fragment_stores("normal", normal) &&
                 open_fragment_stores("scrambled", scrambled);
    if (!integrated) {
        goto done;
    }
    for (fragment = 0; fragment < 5; fragment++) {
        if (cdc_shared_record_publish(normal[fragment], record_id,
                                      (uint64_t)fragment,
                                      normal_values[fragment]) !=
            CDC_SHARED_RECORD_OK) {
            integrated = 0;
            goto done;
        }
    }
    for (fragment = 0; fragment < FRAGMENT_COUNT; fragment++) {
        if (cdc_shared_record_publish(scrambled[fragment], record_id,
                                      (uint64_t)fragment,
                                      scrambled_values[fragment]) !=
            CDC_SHARED_RECORD_OK) {
            integrated = 0;
            goto done;
        }
    }
    close_fragment_stores(normal);
    close_fragment_stores(scrambled);
    if (!reopen_fragment_stores("normal", normal) ||
        !reopen_fragment_stores("scrambled", scrambled)) {
        integrated = 0;
        goto done;
    }
    status = cdc_shared_record_recover(normal, 1, record_id, 1, &recovered);
    result.single_fragment_recovery =
        status == CDC_SHARED_RECORD_OK && recovered.value == truth ? 1.0 : 0.0;
    status = cdc_shared_record_recover(normal, FRAGMENT_COUNT, record_id,
                                       QUORUM, &recovered);
    result.quorum_recovery =
        status == CDC_SHARED_RECORD_OK && recovered.value == truth ? 1.0 : 0.0;
    result.quorum_gain =
        result.quorum_recovery - result.single_fragment_recovery;
    status = cdc_shared_record_recover(scrambled, FRAGMENT_COUNT, record_id,
                                       QUORUM, &recovered);
    result.scrambled_recovery =
        status == CDC_SHARED_RECORD_OK && recovered.value == truth ? 1.0 : 0.0;
    status = cdc_shared_record_recover(central_only, FRAGMENT_COUNT,
                                       record_id, QUORUM, &recovered);
    result.central_only_refused = status == CDC_SHARED_RECORD_ENODATA;
    result.central_only_recovery =
        status == CDC_SHARED_RECORD_OK && recovered.value == truth ? 1.0 : 0.0;
    result.sealed_fragment_stores =
        (int)(cdc_store_event_count(normal[0]) +
              cdc_store_event_count(normal[1]) +
              cdc_store_event_count(normal[2]) +
              cdc_store_event_count(normal[3]) +
              cdc_store_event_count(normal[4]));

done:
    close_fragment_stores(normal);
    close_fragment_stores(scrambled);
    result.pass = integrated &&
                  result.single_fragment_recovery == 0.0 &&
                  result.quorum_recovery == 1.0 &&
                  result.quorum_gain == 1.0 &&
                  result.scrambled_recovery == 0.0 &&
                  result.central_only_recovery == 0.0 &&
                  result.central_only_refused &&
                  result.sealed_fragment_stores == 5;
    return result;
}

static int write_json(const char *path, const Report *report) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "rftc: cannot write %s: %s\n", path,
                strerror(errno));
        return 0;
    }
    fprintf(
        fp,
        "{\n"
        "  \"schema\": \"rftc-crucible/v2\",\n"
        "  \"classification\": "
        "\"CLASSICAL_REFERENCE_FRAME_TOPOLOGICAL_COHERENCE\",\n"
        "  \"profile\": \"%s\",\n"
        "  \"seed\": \"%llu\",\n"
        "  \"seeds\": %d,\n"
        "  \"nodeCount\": %d,\n"
        "  \"experiments\": [\n"
        "    {\"id\":\"synchronization-onset\",\"status\":\"%s\","
        "\"uncoupledR\":%.9f,\"coupledR\":%.9f,\"separation\":%.9f,"
        "\"criticalCoupling\":%.9f,\"precriticalR\":%.9f,"
        "\"criticalR\":%.9f,\"maxRegression\":%.9f,"
        "\"thresholds\":{\"uncoupledMax\":0.35,\"coupledMin\":0.80,"
        "\"separationMin\":0.50,\"criticalR\":0.80,"
        "\"maxRegression\":0.03}},\n"
        "    {\"id\":\"topological-sector\",\"status\":\"%s\","
        "\"moderatePreservation\":%.9f,\"shuffledPreservation\":%.9f,"
        "\"meanSlipThreshold\":%.9f,"
        "\"thresholds\":{\"moderateMin\":0.99,\"shuffledMax\":0.60,"
        "\"slipMin\":2.5,\"slipMax\":3.8}},\n"
        "    {\"id\":\"hidden-granularity\",\"status\":\"%s\","
        "\"macroMatchRate\":%.9f,\"digestDifferenceRate\":%.9f,"
        "\"frameVersionDifferenceRate\":%.9f,"
        "\"claim\":\"distinct microstates share one macro boundary\"},\n"
        "    {\"id\":\"bidi-recovery\",\"status\":\"%s\","
        "\"bidiRecoverySteps\":%.9f,\"localRecoverySteps\":%.9f,"
        "\"improvement\":%.9f,\"thresholds\":{\"improvementMin\":0.30}},\n"
        "    {\"id\":\"causal-cut\",\"status\":\"%s\","
        "\"classicalS\":%.9f,\"communicatingS\":%.9f,"
        "\"classicalValid\":true,\"communicatingValid\":false,"
        "\"claim\":\"a communicating digital cluster cannot certify "
        "nonclassicality\"},\n"
        "    {\"id\":\"packet-threshold\",\"status\":\"%s\","
        "\"subthresholdTotalInput\":%.9f,"
        "\"superthresholdTotalInput\":%.9f,"
        "\"subthresholdCommitRate\":%.9f,"
        "\"lowFluxCommitRate\":%.9f,\"highFluxCommitRate\":%.9f,"
        "\"committedCountRatio\":%.9f,\"meanExcessDelta\":%.9f,"
        "\"buggyAccumulatorWouldCommit\":%s,\"receiptParity\":%s,"
        "\"claim\":\"aggregate drive cannot replace per-event "
        "admissibility\"},\n"
        "    {\"id\":\"record-redundancy\",\"status\":\"%s\","
        "\"singleFragmentRecovery\":%.9f,\"quorumRecovery\":%.9f,"
        "\"quorumGain\":%.9f,\"scrambledRecovery\":%.9f,"
        "\"centralOnlyRecovery\":%.9f,\"centralOnlyRefused\":%s,"
        "\"sealedFragmentStores\":%d,"
        "\"claim\":\"shared facts require independently recoverable "
        "fragments\"}\n"
        "  ],\n"
        "  \"verdict\": \"%s\",\n"
        "  \"claims\": {\n"
        "    \"allowed\": [\"classical collective order parameter\","
        "\"classical topological sector\","
        "\"many-to-one reference-frame coarse graining\","
        "\"bidirectional macro-micro control advantage\","
        "\"typed per-event threshold contract\","
        "\"redundant classical record recovery\"],\n"
        "    \"notAllowed\": [\"quantum superposition\","
        "\"entanglement\","
        "\"Bell nonlocality\","
        "\"quantum computational advantage\"]\n"
        "  }\n"
        "}\n",
        report->profile, (unsigned long long)report->seed, report->seeds,
        NODE_COUNT, report->sync.pass ? "PASS" : "FAIL",
        report->sync.uncoupled_r, report->sync.coupled_r,
        report->sync.separation, report->sync.critical_coupling,
        report->sync.precritical_r, report->sync.critical_r,
        report->sync.max_regression,
        report->topology.pass ? "PASS" : "FAIL",
        report->topology.moderate_preservation,
        report->topology.shuffled_preservation,
        report->topology.mean_slip_threshold,
        report->hidden.pass ? "PASS" : "FAIL",
        report->hidden.macro_match_rate,
        report->hidden.digest_difference_rate,
        report->hidden.frame_version_difference_rate,
        report->bidi.pass ? "PASS" : "FAIL",
        report->bidi.bidi_recovery_steps,
        report->bidi.local_recovery_steps, report->bidi.improvement,
        report->causal.pass ? "PASS" : "FAIL",
        report->causal.classical_s, report->causal.communicating_s,
        report->packet.pass ? "PASS" : "FAIL",
        report->packet.subthreshold_total_input,
        report->packet.superthreshold_total_input,
        report->packet.subthreshold_commit_rate,
        report->packet.low_flux_commit_rate,
        report->packet.high_flux_commit_rate,
        report->packet.committed_count_ratio,
        report->packet.mean_excess_delta,
        report->packet.buggy_accumulator_would_commit ? "true" : "false",
        report->packet.receipt_parity ? "true" : "false",
        report->record.pass ? "PASS" : "FAIL",
        report->record.single_fragment_recovery,
        report->record.quorum_recovery, report->record.quorum_gain,
        report->record.scrambled_recovery,
        report->record.central_only_recovery,
        report->record.central_only_refused ? "true" : "false",
        report->record.sealed_fragment_stores,
        report->pass ? "PASS_FOUNDATIONAL_CLASSICAL_MECHANISM"
                     : "FAIL_COUNTEREXAMPLE_FOUND");
    if (fclose(fp) != 0) {
        fprintf(stderr, "rftc: close failed for %s\n", path);
        return 0;
    }
    return 1;
}

static int write_csv(const char *path, const Report *report) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "rftc: cannot write %s: %s\n", path,
                strerror(errno));
        return 0;
    }
    fputs("experiment,status,metric_a,metric_b,metric_c\n", fp);
    fprintf(fp, "synchronization-onset,%s,%.9f,%.9f,%.9f\n",
            report->sync.pass ? "PASS" : "FAIL",
            report->sync.uncoupled_r, report->sync.coupled_r,
            report->sync.separation);
    fprintf(fp, "topological-sector,%s,%.9f,%.9f,%.9f\n",
            report->topology.pass ? "PASS" : "FAIL",
            report->topology.moderate_preservation,
            report->topology.shuffled_preservation,
            report->topology.mean_slip_threshold);
    fprintf(fp, "hidden-granularity,%s,%.9f,%.9f,0\n",
            report->hidden.pass ? "PASS" : "FAIL",
            report->hidden.macro_match_rate,
            report->hidden.digest_difference_rate);
    fprintf(fp, "bidi-recovery,%s,%.9f,%.9f,%.9f\n",
            report->bidi.pass ? "PASS" : "FAIL",
            report->bidi.bidi_recovery_steps,
            report->bidi.local_recovery_steps, report->bidi.improvement);
    fprintf(fp, "causal-cut,%s,%.9f,%.9f,0\n",
            report->causal.pass ? "PASS" : "FAIL",
            report->causal.classical_s, report->causal.communicating_s);
    fprintf(fp, "packet-threshold,%s,%.9f,%.9f,%.9f\n",
            report->packet.pass ? "PASS" : "FAIL",
            report->packet.subthreshold_commit_rate,
            report->packet.committed_count_ratio,
            report->packet.mean_excess_delta);
    fprintf(fp, "record-redundancy,%s,%.9f,%.9f,%.9f\n",
            report->record.pass ? "PASS" : "FAIL",
            report->record.single_fragment_recovery,
            report->record.quorum_recovery, report->record.quorum_gain);
    if (fclose(fp) != 0) {
        fprintf(stderr, "rftc: close failed for %s\n", path);
        return 0;
    }
    return 1;
}

static void configure_profile(Report *report, const char *profile) {
    report->profile = profile;
    if (strcmp(profile, "smoke") == 0) {
        report->seeds = 24;
        report->steps = 700;
    } else if (strcmp(profile, "rapid") == 0) {
        report->seeds = 256;
        report->steps = 1200;
    } else if (strcmp(profile, "stress") == 0) {
        report->seeds = 4096;
        report->steps = 2400;
    } else {
        fprintf(stderr, "rftc: unknown profile '%s'\n", profile);
        exit(2);
    }
}

static void usage(void) {
    fputs("usage: rftc_crucible [--profile smoke|rapid|stress] "
          "[--seed N] [--json path] [--csv path]\n",
          stderr);
    exit(2);
}

int main(int argc, char **argv) {
    Report report;
    const char *profile = "smoke";
    const char *json_path = "build/rftc/verdict.json";
    const char *csv_path = "build/rftc/metrics.csv";
    int i;

    memset(&report, 0, sizeof(report));
    report.seed = UINT64_C(0x5246544300000001);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
            profile = argv[++i];
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            char *end = NULL;
            unsigned long long value =
                strtoull(argv[++i], &end, 0);
            if (!end || *end != '\0' || value == 0) {
                usage();
            }
            report.seed = (uint64_t)value;
        } else if (strcmp(argv[i], "--json") == 0 && i + 1 < argc) {
            json_path = argv[++i];
        } else if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csv_path = argv[++i];
        } else {
            usage();
        }
    }

    configure_profile(&report, profile);
    if (!verify_rftc_input_boundary()) {
        fputs("rftc: non-finite phase boundary failed\n", stderr);
        return 2;
    }
    report.sync = run_sync(&report);
    report.topology = run_topology(&report);
    report.hidden = run_hidden(&report);
    report.bidi = run_bidi(&report);
    report.causal = run_causal(&report);
    report.packet = run_packet_threshold(&report);
    report.record = run_record_redundancy(&report);
    report.pass = report.sync.pass && report.topology.pass &&
                  report.hidden.pass && report.bidi.pass &&
                  report.causal.pass && report.packet.pass &&
                  report.record.pass;

    if (!write_json(json_path, &report) || !write_csv(csv_path, &report)) {
        return 2;
    }
    printf(
        "rftc crucible %s profile=%s seeds=%d "
        "sync=%.3f->%.3f topology=%.3f hidden=%.3f "
        "bidi-improvement=%.3f chsh=%.3f/%.3f "
        "packet-ratio=%.3f record-quorum=%.3f\n",
        report.pass ? "PASS" : "FAIL", report.profile, report.seeds,
        report.sync.uncoupled_r, report.sync.coupled_r,
        report.topology.moderate_preservation,
        report.hidden.macro_match_rate, report.bidi.improvement,
        report.causal.classical_s, report.causal.communicating_s,
        report.packet.committed_count_ratio,
        report.record.quorum_recovery);
    return report.pass ? 0 : 1;
}

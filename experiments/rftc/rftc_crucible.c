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

#include "../../runtime/cdc_digest.h"

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
    const char *profile;
    int seeds;
    int steps;
    uint64_t seed;
    SyncResult sync;
    TopologyResult topology;
    HiddenResult hidden;
    BidiResult bidi;
    CausalResult causal;
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

static void digest_phases(const double *theta, int count, char out[72]) {
    uint8_t encoded[NODE_COUNT * 8];
    uint8_t digest[CDC_DIGEST_SIZE];
    int i;
    int byte_index;

    if (count < 0 || count > NODE_COUNT) {
        fputs("rftc: invalid phase count for digest\n", stderr);
        exit(2);
    }
    /*
     * Hash a canonical, quantized little-endian representation rather than
     * host doubles. Hidden-class evidence must replay across endianness and
     * floating-point layouts, not merely on the machine that emitted it.
     */
    for (i = 0; i < count; i++) {
        int64_t quantized =
            (int64_t)llround(wrap_pi(theta[i]) * 1000000000000.0);
        uint64_t bits = (uint64_t)quantized;
        for (byte_index = 0; byte_index < 8; byte_index++) {
            encoded[i * 8 + byte_index] =
                (uint8_t)((bits >> (8 * byte_index)) & UINT64_C(0xff));
        }
    }
    cdc_digest(encoded, (size_t)count * 8, digest);
    cdc_digest_hex(digest, out, 72);
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
    int seed_index;

    memset(&result, 0, sizeof(result));
    result.seeds = config->seeds;
    for (seed_index = 0; seed_index < config->seeds; seed_index++) {
        double a[NODE_COUNT];
        double b[NODE_COUNT];
        char digest_a[72];
        char digest_b[72];
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
        digest_phases(a, NODE_COUNT, digest_a);
        digest_phases(b, NODE_COUNT, digest_b);

        if (fabs(za.r - zb.r) < 1e-12 &&
            fabs(wrap_pi(za.psi - zb.psi)) < 1e-12 && wa == wb) {
            macro_matches++;
        }
        if (strcmp(digest_a, digest_b) != 0) {
            digest_differs++;
        }
    }
    result.macro_match_rate =
        (double)macro_matches / (double)config->seeds;
    result.digest_difference_rate =
        (double)digest_differs / (double)config->seeds;
    result.pass = result.macro_match_rate == 1.0 &&
                  result.digest_difference_rate == 1.0;
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
        "  \"schema\": \"rftc-crucible/v1\",\n"
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
        "\"claim\":\"distinct microstates share one macro boundary\"},\n"
        "    {\"id\":\"bidi-recovery\",\"status\":\"%s\","
        "\"bidiRecoverySteps\":%.9f,\"localRecoverySteps\":%.9f,"
        "\"improvement\":%.9f,\"thresholds\":{\"improvementMin\":0.30}},\n"
        "    {\"id\":\"causal-cut\",\"status\":\"%s\","
        "\"classicalS\":%.9f,\"communicatingS\":%.9f,"
        "\"classicalValid\":true,\"communicatingValid\":false,"
        "\"claim\":\"a communicating digital cluster cannot certify "
        "nonclassicality\"}\n"
        "  ],\n"
        "  \"verdict\": \"%s\",\n"
        "  \"claims\": {\n"
        "    \"allowed\": [\"classical collective order parameter\","
        "\"classical topological sector\","
        "\"many-to-one reference-frame coarse graining\","
        "\"bidirectional macro-micro control advantage\"],\n"
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
        report->bidi.pass ? "PASS" : "FAIL",
        report->bidi.bidi_recovery_steps,
        report->bidi.local_recovery_steps, report->bidi.improvement,
        report->causal.pass ? "PASS" : "FAIL",
        report->causal.classical_s, report->causal.communicating_s,
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
    report.sync = run_sync(&report);
    report.topology = run_topology(&report);
    report.hidden = run_hidden(&report);
    report.bidi = run_bidi(&report);
    report.causal = run_causal(&report);
    report.pass = report.sync.pass && report.topology.pass &&
                  report.hidden.pass && report.bidi.pass &&
                  report.causal.pass;

    if (!write_json(json_path, &report) || !write_csv(csv_path, &report)) {
        return 2;
    }
    printf(
        "rftc crucible %s profile=%s seeds=%d "
        "sync=%.3f->%.3f topology=%.3f hidden=%.3f "
        "bidi-improvement=%.3f chsh=%.3f/%.3f\n",
        report.pass ? "PASS" : "FAIL", report.profile, report.seeds,
        report.sync.uncoupled_r, report.sync.coupled_r,
        report.topology.moderate_preservation,
        report.hidden.macro_match_rate, report.bidi.improvement,
        report.causal.classical_s, report.causal.communicating_s);
    return report.pass ? 0 : 1;
}

#include "cdc_variational.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRICT_TOL 1e-12
#define FD_TOL 1e-7

static void require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "u2-variational-test: %s\n", message);
        exit(1);
    }
}

static int close_enough(double actual, double expected, double tolerance) {
    return fabs(actual - expected) <= tolerance;
}

static void require_matrix(const cdc_matrix *matrix, const double *expected,
                           size_t count, double tolerance,
                           const char *message) {
    size_t i;
    require(matrix && matrix->data && matrix->rows * matrix->cols == count,
            "matrix shape mismatch");
    for (i = 0; i < count; ++i) {
        if (!close_enough(matrix->data[i], expected[i], tolerance)) {
            fprintf(stderr,
                    "u2-variational-test: %s at index %zu: got %.17g expected %.17g\n",
                    message, i, matrix->data[i], expected[i]);
            exit(1);
        }
    }
}

static cdc_u2_layout two_cell_layout(void) {
    const char *cells[] = {"driver", "response"};
    cdc_u2_layout layout;
    require(cdc_u2_layout_build(&layout, cells, 2, NULL, 0) == CDC_U2_OK,
            "two-cell layout build failed");
    return layout;
}

static void test_state_manifest_and_pack_order(void) {
    const char *cells[] = {"first", "second"};
    const char *modules[] = {"body"};
    double theta[] = {0.1, 0.2};
    double belief[] = {0.3};
    double prior[] = {0.4};
    double packed[4] = {0};
    double unpacked_theta[2] = {0};
    double unpacked_belief[1] = {0};
    double unpacked_prior[1] = {0};
    const double expected[] = {0.1, 0.2, 0.3, 0.4};
    cdc_u2_layout layout;

    require(cdc_u2_layout_build(&layout, cells, 2, modules, 1) == CDC_U2_OK,
            "state manifest build failed");
    require(layout.dimension == 4 &&
                strcmp(layout.coordinates[0].name, "first.theta") == 0 &&
                strcmp(layout.coordinates[1].name, "second.theta") == 0 &&
                strcmp(layout.coordinates[2].name, "body.belief") == 0 &&
                strcmp(layout.coordinates[3].name, "body.prior") == 0,
            "state manifest coordinate order is unstable");
    require(cdc_u2_layout_find(&layout, "body.prior") == 3,
            "state manifest lookup mismatch");
    require(cdc_u2_state_pack(
                &layout, theta, 2, belief, prior, 1, packed, 4) == CDC_U2_OK,
            "state pack failed");
    require_matrix(&(cdc_matrix){4, 1, packed, 0}, expected, 4, STRICT_TOL,
                   "packed state order mismatch");
    require(cdc_u2_state_unpack(
                &layout, packed, 4, unpacked_theta, 2,
                unpacked_belief, unpacked_prior, 1) == CDC_U2_OK,
            "state unpack failed");
    require(close_enough(unpacked_theta[0], theta[0], STRICT_TOL) &&
                close_enough(unpacked_theta[1], theta[1], STRICT_TOL) &&
                close_enough(unpacked_belief[0], belief[0], STRICT_TOL) &&
                close_enough(unpacked_prior[0], prior[0], STRICT_TOL),
            "state pack/unpack round trip mismatch");
    cdc_u2_layout_release(&layout);
}

typedef struct {
    double coefficient;
    double angle;
} flow_map_context;

static cdc_u2_status explicit_euler_flow_map(
    const double *input, size_t dimension, double *output, void *opaque) {
    flow_map_context *context = (flow_map_context *)opaque;
    if (!input || !output || !context || dimension != 2) {
        return CDC_U2_INVALID_ARGUMENT;
    }
    output[0] = input[0];
    output[1] = input[1] +
                context->coefficient *
                    sin(input[0] + context->angle - input[1]);
    return CDC_U2_OK;
}

static void test_executable_flow_derivative_and_finite_difference(void) {
    cdc_u2_layout layout = two_cell_layout();
    double state[] = {0.0, 0.0};
    double jacobian_data[4];
    const double expected[] = {1.0, 0.0, 0.5, 0.5};
    cdc_matrix jacobian;
    cdc_u2_flow_coupling coupling = {0, 1, 0.5, 0.0};
    flow_map_context context = {0.5, 0.0};
    double max_error = -1.0;

    require(cdc_matrix_view(&jacobian, 2, 2, jacobian_data) == CDC_LINALG_OK,
            "flow Jacobian view failed");
    require(cdc_u2_flow_jacobian(&layout, state, 2, &coupling, 1,
                                 &jacobian) == CDC_U2_OK,
            "flow Jacobian construction failed");
    require_matrix(&jacobian, expected, 4, STRICT_TOL,
                   "explicit-Euler flow derivative mismatch");
    require(cdc_u2_validate_jacobian(
                explicit_euler_flow_map, &context, state, 2, &jacobian,
                1e-6, FD_TOL, &max_error) == CDC_U2_OK,
            "analytic flow derivative disagrees with central differences");
    require(max_error <= FD_TOL,
            "flow finite-difference residual exceeded acceptance tolerance");
    cdc_u2_layout_release(&layout);
}

typedef struct {
    double gain;
    double fixed_up;
} nest_map_context;

static cdc_u2_status executable_nest_map(
    const double *input, size_t dimension, double *output, void *opaque) {
    nest_map_context *context = (nest_map_context *)opaque;
    if (!input || !output || !context || dimension != 3) {
        return CDC_U2_INVALID_ARGUMENT;
    }
    output[0] = input[0] + context->gain * context->fixed_up;
    output[1] = input[1];
    output[2] = output[0];
    return CDC_U2_OK;
}

static void test_nest_derivative_is_of_executed_fixed_trit_map(void) {
    double jacobian_data[9];
    const double expected[] = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        1.0, 0.0, 0.0
    };
    double point[] = {0.25, -0.5, 0.75};
    cdc_matrix jacobian;
    nest_map_context context = {3.0, 2.0 / 3.0};
    double max_error = -1.0;

    require(cdc_matrix_view(&jacobian, 3, 3, jacobian_data) == CDC_LINALG_OK,
            "nest Jacobian view failed");
    require(cdc_u2_nest_jacobian(3, 0, 1, 2, context.gain, &jacobian) ==
                CDC_U2_OK,
            "nest Jacobian construction failed");
    require_matrix(&jacobian, expected, 9, STRICT_TOL,
                   "nest differentiated dynamics absent from primal execution");
    require(cdc_u2_validate_jacobian(
                executable_nest_map, &context, point, 3, &jacobian,
                1e-6, FD_TOL, &max_error) == CDC_U2_OK,
            "analytic nest derivative disagrees with central differences");
    require(max_error <= FD_TOL,
            "nest finite-difference residual exceeded acceptance tolerance");
}

static void test_commit_modes_fail_closed(void) {
    double jacobian_data[4];
    const double identity[] = {1.0, 0.0, 0.0, 1.0};
    cdc_matrix jacobian;
    require(cdc_matrix_view(&jacobian, 2, 2, jacobian_data) == CDC_LINALG_OK,
            "commit Jacobian view failed");
    require(cdc_u2_commit_jacobian(2, CDC_U2_COMMIT_SCHEDULED, 1,
                                   &jacobian) == CDC_U2_OK,
            "fixed-mode scheduled commit derivative failed");
    require_matrix(&jacobian, identity, 4, STRICT_TOL,
                   "scheduled fixed-mode commit must be identity");
    require(cdc_u2_commit_jacobian(2, CDC_U2_COMMIT_SCHEDULED, 0,
                                   &jacobian) == CDC_U2_MODE_DIVERGENCE,
            "mode-divergent commit did not hold");
    require(cdc_u2_commit_jacobian(2, CDC_U2_COMMIT_GUARD_TRIGGERED, 1,
                                   &jacobian) == CDC_U2_GUARD_NOT_LOCALIZED,
            "unlocalized guarded commit fabricated a saltation matrix");
}

static void test_saltation_is_not_reset_only_and_grazing_holds(void) {
    double reset_data[] = {0.5};
    double saltation_data[] = {-99.0};
    double flow_before[] = {1.0};
    double flow_after[] = {2.0};
    double normal[] = {1.0};
    double grazing_before[] = {0.0};
    double denominator = NAN;
    cdc_matrix reset;
    cdc_matrix saltation;

    require(cdc_matrix_view(&reset, 1, 1, reset_data) == CDC_LINALG_OK,
            "reset Jacobian view failed");
    require(cdc_matrix_view(&saltation, 1, 1, saltation_data) == CDC_LINALG_OK,
            "saltation matrix view failed");
    require(cdc_u2_saltation_matrix(
                &reset, flow_before, flow_after, normal, NULL, 1, 0.0,
                1e-12, &saltation, &denominator) == CDC_U2_OK,
            "closed-form transverse saltation failed");
    require(close_enough(denominator, 1.0, STRICT_TOL),
            "saltation denominator mismatch");
    require(close_enough(saltation.data[0], 2.0, STRICT_TOL),
            "saltation correction was omitted");
    require(!close_enough(saltation.data[0], reset.data[0], STRICT_TOL),
            "reset Jacobian was substituted for saltation");

    saltation.data[0] = -77.0;
    require(cdc_u2_saltation_matrix(
                &reset, grazing_before, flow_after, normal, NULL, 1, 0.0,
                1e-12, &saltation, &denominator) ==
                CDC_U2_NONTRANSVERSE_EVENT,
            "grazing/non-transverse event did not hold");
}

static void test_two_event_product_and_event_order(void) {
    double first_data[] = {1.0, 1.0, 0.0, 1.0};
    double second_data[] = {1.0, 0.0, 1.0, 1.0};
    double identity_data[] = {1.0, 0.0, 0.0, 1.0};
    const double expected[] = {1.0, 1.0, 1.0, 2.0};
    cdc_matrix first;
    cdc_matrix second;
    cdc_matrix identity;
    cdc_u2_monodromy monodromy;

    require(cdc_matrix_view(&first, 2, 2, first_data) == CDC_LINALG_OK,
            "first event view failed");
    require(cdc_matrix_view(&second, 2, 2, second_data) == CDC_LINALG_OK,
            "second event view failed");
    require(cdc_matrix_view(&identity, 2, 2, identity_data) == CDC_LINALG_OK,
            "identity event view failed");
    require(cdc_u2_monodromy_init(&monodromy, 2) == CDC_U2_OK,
            "monodromy initialization failed");
    require(cdc_u2_monodromy_append(
                &monodromy, &first, CDC_U2_EVENT_GUARD_TRANSITION, 0.25, 1) ==
                CDC_U2_OK,
            "first event append failed");
    require(cdc_u2_monodromy_append(
                &monodromy, &second, CDC_U2_EVENT_GUARD_TRANSITION, 0.75, 1) ==
                CDC_U2_OK,
            "second event append failed");
    require_matrix(&monodromy.matrix, expected, 4, STRICT_TOL,
                   "monodromy product order reversed");
    require(monodromy.event_count == 2, "monodromy event count mismatch");
    require(monodromy.events[0].kind == CDC_U2_EVENT_GUARD_TRANSITION &&
                close_enough(monodromy.events[0].time, 0.25, STRICT_TOL),
            "first event receipt lost execution order");
    require(monodromy.events[1].kind == CDC_U2_EVENT_GUARD_TRANSITION &&
                close_enough(monodromy.events[1].time, 0.75, STRICT_TOL),
            "second event receipt lost execution order");
    require(cdc_u2_monodromy_append(
                &monodromy, &identity, CDC_U2_EVENT_SCHEDULED_COMMIT,
                0.75, -1) == CDC_U2_OK,
            "explicitly ordered equal-time event was rejected");
    require(monodromy.event_count == 3,
            "equal-time event was not retained in explicit append order");
    require(cdc_u2_monodromy_append(
                &monodromy, &identity, CDC_U2_EVENT_FLOW, 0.5, -1) ==
                CDC_U2_INVALID_ARGUMENT,
            "backwards event timestamp was accepted");
    require(monodromy.event_count == 3,
            "rejected backwards event mutated the event itinerary");
    require_matrix(&monodromy.matrix, expected, 4, STRICT_TOL,
                   "rejected backwards event mutated monodromy");
    cdc_u2_monodromy_release(&monodromy);
}

typedef struct {
    double phase_shift;
    double belief_drift;
    double phase_scale;
    double belief_scale;
} restoration_context;

static cdc_u2_status restore_declared_drift(
    const double *final_state, size_t dimension, double *restored_state,
    void *opaque) {
    restoration_context *context = (restoration_context *)opaque;
    size_t i;
    if (!final_state || !restored_state || !context || dimension != 3) {
        return CDC_U2_INVALID_ARGUMENT;
    }
    for (i = 0; i < dimension; ++i) {
        restored_state[i] = final_state[i];
    }
    restored_state[0] = context->phase_scale *
                        (restored_state[0] - context->phase_shift);
    restored_state[1] = context->belief_scale *
                        (restored_state[1] - context->belief_drift);
    return CDC_U2_OK;
}

static void test_recurrence_projection_does_not_authorize_monodromy(void) {
    const char *cells[] = {"phase"};
    const char *modules[] = {"body"};
    cdc_u2_layout layout;
    /* layout is phase theta, body belief, body prior */
    double initial[] = {0.0, 0.0, 0.0};
    double final[] = {0.25, 1.0, 0.0};
    double projected_final[] = {0.0, 1.0, 0.0};
    double periodic_final[] = {2.0 * 3.14159265358979323846, 0.0, 0.0};
    unsigned char phase_only[] = {1, 0, 0};
    double restoration_derivative_data[] = {
        -1.0, 0.0, 0.0,
         0.0, 2.0, 0.0,
         0.0, 0.0, 1.0
    };
    double wrong_derivative_data[] = {1.0, 0.0, 0.0, 1.0};
    double path_tangent_data[] = {
        0.5, 0.0, 0.0,
        0.0, 0.25, 0.0,
        0.0, 0.0, 1.0
    };
    const double expected_relative_monodromy[] = {
        -0.5, 0.0, 0.0,
         0.0, 0.5, 0.0,
         0.0, 0.0, 1.0
    };
    restoration_context restoration = {0.25, 1.0, -1.0, 2.0};
    cdc_matrix restoration_derivative;
    cdc_matrix wrong_derivative;
    cdc_matrix path_tangent;
    cdc_u2_monodromy relative_monodromy;
    cdc_u2_recurrence_spec full = {
        .kind = CDC_U2_RECURRENCE_FULL,
        .tolerance = 1e-9,
        .include = NULL,
        .restore_endpoint = NULL,
        .restoration_context = NULL,
        .restoration_equivariant = 0,
        .restoration_derivative = NULL,
        .discrete_state_verified = 1
    };
    cdc_u2_recurrence_spec projected = {
        .kind = CDC_U2_RECURRENCE_FULL,
        .tolerance = 1e-9,
        .include = phase_only,
        .restore_endpoint = NULL,
        .restoration_context = NULL,
        .restoration_equivariant = 0,
        .restoration_derivative = NULL,
        .discrete_state_verified = 1
    };
    cdc_u2_recurrence_spec relative = {
        .kind = CDC_U2_RECURRENCE_RELATIVE,
        .tolerance = 1e-9,
        .include = NULL,
        .restore_endpoint = restore_declared_drift,
        .restoration_context = &restoration,
        .restoration_equivariant = 1,
        .restoration_derivative = &restoration_derivative,
        .discrete_state_verified = 1
    };
    cdc_u2_recurrence_spec unproved_relative = relative;
    cdc_u2_recurrence_result result;

    require(cdc_matrix_view(&restoration_derivative, 3, 3,
                            restoration_derivative_data) == CDC_LINALG_OK,
            "restoration derivative view failed");
    require(cdc_matrix_view(&wrong_derivative, 2, 2,
                            wrong_derivative_data) == CDC_LINALG_OK,
            "wrong restoration derivative view failed");
    require(cdc_matrix_view(&path_tangent, 3, 3, path_tangent_data) ==
                CDC_LINALG_OK,
            "path tangent view failed");
    require(cdc_u2_layout_build(&layout, cells, 1, modules, 1) == CDC_U2_OK,
            "recurrence layout build failed");
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &full, &result) ==
                CDC_U2_NOT_RECURRENT,
            "nonrecurrent full state was accepted");
    require(!result.verified && result.residual > result.tolerance,
            "nonrecurrent result falsely marked verified");
    require(cdc_u2_recurrence_check(
                &layout, initial, projected_final, 3, &projected, &result) ==
                CDC_U2_NOT_RECURRENT,
            "projected return was not held before monodromy");
    require(result.verified && result.projected &&
                result.included_coordinates == 1,
            "projected recurrence did not honor coordinate mask");
    require(!result.authorizes_monodromy,
            "coordinate projection was promoted to full/relative recurrence");

    unproved_relative.restoration_equivariant = 0;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &unproved_relative, &result) ==
                CDC_U2_RELATIVE_RESTORATION_UNVERIFIED,
            "unproved relative restoration authorized a return map");
    require(!result.authorizes_monodromy,
            "unproved restoration authorized monodromy");

    unproved_relative = relative;
    unproved_relative.restoration_derivative = NULL;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &unproved_relative, &result) ==
                CDC_U2_RELATIVE_RESTORATION_UNVERIFIED,
            "relative recurrence without D-rho was accepted");
    unproved_relative.restoration_derivative = &wrong_derivative;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &unproved_relative, &result) ==
                CDC_U2_RELATIVE_RESTORATION_UNVERIFIED,
            "dimension-mismatched D-rho was accepted");

    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &relative, &result) ==
                CDC_U2_OK,
            "relative recurrence check must remain held until D-rho is applied");
    require(result.verified && !result.projected &&
                result.restoration_applied && result.restoration_equivariant &&
                result.restoration_derivative_bound,
            "relative recurrence receipt omitted restoration evidence");
    require(!result.restoration_derivative_applied &&
                !result.authorizes_monodromy,
            "relative recurrence authorized monodromy before applying D-rho");
    require(close_enough(result.residual, 0.0, STRICT_TOL),
            "restored relative recurrence left a residual");

    require(cdc_u2_monodromy_init(&relative_monodromy, 3) == CDC_U2_OK,
            "relative monodromy initialization failed");
    require(cdc_u2_monodromy_append(
                &relative_monodromy, &path_tangent,
                CDC_U2_EVENT_FLOW, 1.0, -1) == CDC_U2_OK,
            "relative path tangent append failed");
    require(cdc_u2_monodromy_bind_recurrence(
                &relative_monodromy, &relative, &result, 1.0) == CDC_U2_OK,
            "relative monodromy did not compose D-rho");
    require(result.restoration_derivative_applied &&
                result.authorizes_monodromy,
            "D-rho composition did not authorize relative monodromy");
    require(relative_monodromy.event_count == 2 &&
                relative_monodromy.events[1].kind ==
                    CDC_U2_EVENT_ENDPOINT_RESTORATION,
            "relative restoration derivative is absent from itinerary");
    require_matrix(&relative_monodromy.matrix,
                   expected_relative_monodromy, 9, STRICT_TOL,
                   "relative monodromy is not D-rho times D-U");
    cdc_u2_monodromy_release(&relative_monodromy);

    require(cdc_u2_recurrence_check(
                &layout, initial, periodic_final, 3, &full, &result) ==
                CDC_U2_OK,
            "periodic theta topology was treated as an ordinary real line");
    require(result.verified, "periodic full state not marked recurrent");
    require(result.authorizes_monodromy,
            "verified full recurrence did not authorize monodromy");

    require(cdc_u2_layout_set_period(&layout, 0, 0.0) == CDC_U2_OK,
            "lifted cover topology override failed");
    require(cdc_u2_recurrence_check(
                &layout, initial, periodic_final, 3, &full, &result) ==
                CDC_U2_NOT_RECURRENT,
            "lifted cover theta was silently reduced modulo 2pi");
    require(!result.authorizes_monodromy && result.residual > result.tolerance,
            "unwrapped cover return authorized monodromy without restoration");

    full.discrete_state_verified = 0;
    require(cdc_u2_recurrence_check(
                &layout, initial, periodic_final, 3, &full, &result) ==
                CDC_U2_MODE_DIVERGENCE,
            "continuous return ignored an unrestored discrete mode");
    require(!result.authorizes_monodromy,
            "discrete mode divergence authorized monodromy");
    cdc_u2_layout_release(&layout);
}

static void test_weighted_absolute_relative_recurrence_tolerance(void) {
    const char *cells[] = {"phase"};
    const char *modules[] = {"body"};
    double initial[] = {0.0, 1000.0, 0.0};
    double final[] = {0.0, 1000.5, 0.0};
    double weights[] = {1.0, 2.0, 1.0};
    double zero_weight[] = {1.0, 0.0, 1.0};
    double negative_weight[] = {1.0, -1.0, 1.0};
    double nonfinite_weight[] = {1.0, NAN, 1.0};
    cdc_u2_layout layout;
    cdc_u2_recurrence_spec spec = {
        .kind = CDC_U2_RECURRENCE_FULL,
        .tolerance = 0.0,
        .absolute_tolerance = 0.1,
        .relative_tolerance = 0.001,
        .weights = weights,
        .include = NULL,
        .restore_endpoint = NULL,
        .restoration_context = NULL,
        .restoration_equivariant = 0,
        .restoration_derivative = NULL,
        .discrete_state_verified = 1
    };
    cdc_u2_recurrence_result result;

    require(cdc_u2_layout_build(&layout, cells, 1, modules, 1) == CDC_U2_OK,
            "weighted recurrence layout build failed");
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &spec, &result) == CDC_U2_OK,
            "relative tolerance was not applied to a large coordinate");
    require(result.authorizes_monodromy &&
                close_enough(result.residual, 1.0, STRICT_TOL) &&
                close_enough(result.maximum_allowed_residual, 2.101,
                             STRICT_TOL) &&
                result.normalized_residual < 1.0,
            "weighted absolute-plus-relative receipt mismatch");

    spec.relative_tolerance = 0.0;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &spec, &result) ==
                CDC_U2_NOT_RECURRENT,
            "absolute-only tolerance accepted an excessive residual");
    require(!result.authorizes_monodromy && result.normalized_residual > 1.0,
            "failed recurrence tolerance authorized monodromy");

    spec.relative_tolerance = 0.001;
    spec.weights = zero_weight;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &spec, &result) ==
                CDC_U2_INVALID_ARGUMENT,
            "zero recurrence weight was accepted");
    spec.weights = negative_weight;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &spec, &result) ==
                CDC_U2_INVALID_ARGUMENT,
            "negative recurrence weight was accepted");
    spec.weights = nonfinite_weight;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &spec, &result) ==
                CDC_U2_INVALID_ARGUMENT,
            "nonfinite recurrence weight was accepted");
    spec.weights = weights;
    spec.relative_tolerance = -1e-3;
    require(cdc_u2_recurrence_check(
                &layout, initial, final, 3, &spec, &result) ==
                CDC_U2_INVALID_ARGUMENT,
            "negative relative tolerance was accepted");
    cdc_u2_layout_release(&layout);
}

typedef struct {
    cdc_matrix matrix;
} polarity_linear_context;

static cdc_u2_status polarity_linear_map(
    const double *input, size_t dimension, double *output, void *opaque) {
    polarity_linear_context *context =
        (polarity_linear_context *)opaque;
    if (!context) {
        return CDC_U2_INVALID_ARGUMENT;
    }
    switch (cdc_matrix_jvp(&context->matrix, input, dimension,
                           output, dimension)) {
    case CDC_LINALG_OK:
        return CDC_U2_OK;
    case CDC_LINALG_NONFINITE:
        return CDC_U2_NONFINITE;
    default:
        return CDC_U2_INVALID_ARGUMENT;
    }
}

static void test_scoped_polarity_covariance_witness(void) {
    double rho_data[] = {-1.0, 0.0, 0.0, 1.0};
    double primal_data[] = {1.0, 2.0, 3.0, 4.0};
    double conjugate_data[] = {1.0, -2.0, -3.0, 4.0};
    double wrong_derivative_data[] = {1.0, 0.0, 0.0, 1.0};
    double noninvolution_data[] = {-2.0, 0.0, 0.0, 1.0};
    double source[] = {0.25, -0.5};
    double fixed_aperture[] = {0.0, 1.0};
    double broken_aperture[] = {1.0, 0.0};
    polarity_linear_context rho;
    polarity_linear_context primal;
    polarity_linear_context conjugate;
    polarity_linear_context noninvolution;
    cdc_matrix wrong_derivative;
    cdc_u2_polarity_spec spec;
    cdc_u2_polarity_result result;

    require(cdc_matrix_view(&rho.matrix, 2, 2, rho_data) == CDC_LINALG_OK &&
                cdc_matrix_view(&primal.matrix, 2, 2, primal_data) ==
                    CDC_LINALG_OK &&
                cdc_matrix_view(&conjugate.matrix, 2, 2, conjugate_data) ==
                    CDC_LINALG_OK &&
                cdc_matrix_view(&noninvolution.matrix, 2, 2,
                                noninvolution_data) == CDC_LINALG_OK &&
                cdc_matrix_view(&wrong_derivative, 2, 2,
                                wrong_derivative_data) == CDC_LINALG_OK,
            "polarity fixture matrix view failed");
    memset(&spec, 0, sizeof(spec));
    spec.dimension = 2;
    spec.tolerance = 1e-12;
    spec.source_state = source;
    spec.aperture_state = fixed_aperture;
    spec.source_polarity = polarity_linear_map;
    spec.source_polarity_context = &rho;
    spec.target_polarity = polarity_linear_map;
    spec.target_polarity_context = &rho;
    spec.primal_map = polarity_linear_map;
    spec.primal_context = &primal;
    spec.conjugate_primal_map = polarity_linear_map;
    spec.conjugate_primal_context = &conjugate;
    spec.source_polarity_derivative = &rho.matrix;
    spec.target_polarity_derivative = &rho.matrix;
    spec.primal_derivative = &primal.matrix;
    spec.conjugate_primal_derivative = &conjugate.matrix;

    require(cdc_u2_polarity_covariance_check(&spec, &result) == CDC_U2_OK,
            "polarity covariance check failed");
    require(result.accepted && result.reason == CDC_U2_POLARITY_NONE &&
                result.source_involution_residual <= STRICT_TOL &&
                result.target_involution_residual <= STRICT_TOL &&
                result.aperture_fixed_residual <= STRICT_TOL &&
                result.closure_covariance_residual <= STRICT_TOL &&
                result.tangent_covariance_residual <= STRICT_TOL,
            "valid scoped polarity covariance witness held");

    spec.aperture_state = broken_aperture;
    require(cdc_u2_polarity_covariance_check(&spec, &result) == CDC_U2_OK &&
                !result.accepted &&
                result.reason == CDC_U2_POLARITY_APERTURE_NOT_FIXED,
            "polarity witness erased the fixed-aperture obligation");

    spec.aperture_state = fixed_aperture;
    spec.conjugate_primal_derivative = &wrong_derivative;
    require(cdc_u2_polarity_covariance_check(&spec, &result) == CDC_U2_OK &&
                !result.accepted &&
                result.reason == CDC_U2_POLARITY_TANGENT_NOT_COVARIANT,
            "polarity witness promoted primal resemblance without tangent conjugacy");

    spec.conjugate_primal_derivative = &conjugate.matrix;
    spec.source_polarity_context = &noninvolution;
    spec.source_polarity_derivative = &noninvolution.matrix;
    require(cdc_u2_polarity_covariance_check(&spec, &result) == CDC_U2_OK &&
                !result.accepted &&
                result.reason == CDC_U2_POLARITY_MAP_NOT_INVOLUTIVE,
            "non-involutive polarity map was accepted by naming alone");
}

static cdc_linalg_status diagonal_schur(
    const cdc_matrix *input, cdc_matrix *q, cdc_matrix *t,
    double *eigen_real, double *eigen_imag, void *context) {
    size_t i;
    size_t j;
    (void)context;
    if (input->rows != input->cols || q->rows != input->rows ||
        t->rows != input->rows) {
        return CDC_LINALG_DIMENSION_MISMATCH;
    }
    for (i = 0; i < input->rows; ++i) {
        for (j = 0; j < input->cols; ++j) {
            if (i != j && fabs(input->data[i * input->cols + j]) > STRICT_TOL) {
                return CDC_LINALG_BACKEND_FAILED;
            }
            q->data[i * q->cols + j] = i == j ? 1.0 : 0.0;
            t->data[i * t->cols + j] = input->data[i * input->cols + j];
        }
        eigen_real[i] = input->data[i * input->cols + i];
        eigen_imag[i] = 0.0;
    }
    return CDC_LINALG_OK;
}

static cdc_linalg_status reversed_diagonal_schur(
    const cdc_matrix *input, cdc_matrix *q, cdc_matrix *t,
    double *eigen_real, double *eigen_imag, void *context) {
    (void)context;
    if (input->rows != 2 || input->cols != 2 || q->rows != 2 || q->cols != 2 ||
        t->rows != 2 || t->cols != 2 ||
        fabs(input->data[1]) > STRICT_TOL ||
        fabs(input->data[2]) > STRICT_TOL) {
        return CDC_LINALG_BACKEND_FAILED;
    }
    q->data[0] = 0.0;
    q->data[1] = 1.0;
    q->data[2] = 1.0;
    q->data[3] = 0.0;
    t->data[0] = input->data[3];
    t->data[1] = 0.0;
    t->data[2] = 0.0;
    t->data[3] = input->data[0];
    eigen_real[0] = input->data[3];
    eigen_real[1] = input->data[0];
    eigen_imag[0] = 0.0;
    eigen_imag[1] = 0.0;
    return CDC_LINALG_OK;
}

static void require_spectrum_classification(
    double physical_multiplier, cdc_u2_stability_class expected,
    const char *message) {
    double matrix_data[] = {1.0, 0.0, 0.0, physical_multiplier};
    double generator_data[] = {1.0, 0.0};
    cdc_matrix matrix;
    cdc_matrix generator;
    cdc_u2_spectrum spectrum;
    cdc_real_schur_backend backend = {"test-diagonal", diagonal_schur, NULL};
    cdc_u2_recurrence_result recurrence;
    size_t gauge_count = 0;
    size_t i;

    memset(&recurrence, 0, sizeof(recurrence));
    recurrence.kind = CDC_U2_RECURRENCE_FULL;
    recurrence.verified = 1;
    recurrence.discrete_state_verified = 1;
    recurrence.authorizes_monodromy = 1;
    require(cdc_matrix_view(&matrix, 2, 2, matrix_data) == CDC_LINALG_OK,
            "spectrum matrix view failed");
    require(cdc_matrix_view(&generator, 2, 1, generator_data) == CDC_LINALG_OK,
            "gauge generator matrix view failed");
    require(cdc_u2_spectrum_compute(
                &matrix, &backend, &recurrence, &generator,
                1e-9, 1e-12, &spectrum) == CDC_U2_OK,
            message);
    require(spectrum.classification == expected, message);
    for (i = 0; i < spectrum.dimension; ++i) {
        if (spectrum.multipliers[i].mode == CDC_U2_MULTIPLIER_GAUGE) {
            gauge_count++;
            require(close_enough(spectrum.multipliers[i].real, 1.0, 1e-9) &&
                        close_enough(spectrum.multipliers[i].imag, 0.0, 1e-9),
                    "declared gauge mode is not neutral +1");
        }
    }
    require(gauge_count == 1, "neutral gauge mode count mismatch");
    cdc_u2_spectrum_release(&spectrum);
}

static void test_smooth_stable_unstable_neutral_and_gauge(void) {
    double matrix_data[] = {1.0, 0.0, 0.0, 0.5};
    double wrong_generator_data[] = {0.0, 1.0};
    cdc_matrix matrix;
    cdc_matrix wrong_generator;
    cdc_u2_spectrum spectrum;
    cdc_u2_spectrum reversed_spectrum;
    cdc_real_schur_backend backend = {"test-diagonal", diagonal_schur, NULL};
    cdc_real_schur_backend reversed_backend = {
        "test-reversed-diagonal", reversed_diagonal_schur, NULL
    };
    cdc_u2_recurrence_result recurrence;

    require_spectrum_classification(
        0.5, CDC_U2_STABILITY_STABLE,
        "stable physical multiplier misclassified");
    require_spectrum_classification(
        1.5, CDC_U2_STABILITY_UNSTABLE,
        "unstable physical multiplier misclassified");
    require_spectrum_classification(
        1.0, CDC_U2_STABILITY_MARGINAL,
        "neutral physical multiplier misclassified");

    memset(&recurrence, 0, sizeof(recurrence));
    recurrence.kind = CDC_U2_RECURRENCE_FULL;
    recurrence.verified = 1;
    recurrence.discrete_state_verified = 1;
    recurrence.authorizes_monodromy = 1;
    require(cdc_matrix_view(&matrix, 2, 2, matrix_data) == CDC_LINALG_OK,
            "neutral validation matrix view failed");
    require(cdc_u2_spectrum_compute(
                &matrix, &backend, &recurrence, NULL, 1e-9, 1e-12,
                &spectrum) == CDC_U2_OK,
            "physical unit multiplier was rejected without a gauge claim");
    require(spectrum.classification == CDC_U2_STABILITY_MARGINAL,
            "unit multiplier was silently removed without a generator");
    require(spectrum.multipliers[0].mode == CDC_U2_MULTIPLIER_PHYSICAL &&
                spectrum.multipliers[1].mode == CDC_U2_MULTIPLIER_PHYSICAL,
            "unclaimed multiplier was silently labelled gauge");

    require(cdc_u2_spectrum_compute(
                &matrix, &reversed_backend, &recurrence, NULL,
                1e-9, 1e-12, &reversed_spectrum) == CDC_U2_OK,
            "reversed valid Schur ordering was rejected");
    require(reversed_spectrum.dimension == spectrum.dimension,
            "backend ordering changed spectrum dimension");
    require(close_enough(reversed_spectrum.multipliers[0].real,
                         spectrum.multipliers[0].real, STRICT_TOL) &&
                close_enough(reversed_spectrum.multipliers[1].real,
                             spectrum.multipliers[1].real, STRICT_TOL) &&
                reversed_spectrum.multipliers[0].mode ==
                    spectrum.multipliers[0].mode &&
                reversed_spectrum.multipliers[1].mode ==
                    spectrum.multipliers[1].mode,
            "spectrum serialization depends on backend eigenvalue order");
    cdc_u2_spectrum_release(&reversed_spectrum);
    cdc_u2_spectrum_release(&spectrum);

    require(cdc_matrix_view(&wrong_generator, 2, 1, wrong_generator_data) ==
                CDC_LINALG_OK,
            "wrong generator matrix view failed");
    require(cdc_u2_spectrum_compute(
                &matrix, &backend, &recurrence, &wrong_generator,
                1e-9, 1e-12, &spectrum) ==
                CDC_U2_NEUTRAL_MODE_UNVERIFIED,
            "non-invariant generator was accepted as a gauge direction");

    recurrence.authorizes_monodromy = 0;
    require(cdc_u2_spectrum_compute(
                &matrix, &backend, &recurrence, NULL, 1e-9, 1e-12,
                &spectrum) == CDC_U2_NOT_RECURRENT,
            "spectrum emitted without authorizing recurrence");
}

int main(void) {
    require(strcmp(cdc_u2_status_reason(CDC_U2_NONTRANSVERSE_EVENT),
                   "event-not-transverse") == 0,
            "typed non-transverse reason is unstable");
    require(strcmp(cdc_u2_status_reason(CDC_U2_NOT_RECURRENT),
                   "recurrence-residual") == 0,
            "typed recurrence reason is unstable");
    test_state_manifest_and_pack_order();
    test_executable_flow_derivative_and_finite_difference();
    test_nest_derivative_is_of_executed_fixed_trit_map();
    test_commit_modes_fail_closed();
    test_saltation_is_not_reset_only_and_grazing_holds();
    test_two_event_product_and_event_order();
    test_recurrence_projection_does_not_authorize_monodromy();
    test_weighted_absolute_relative_recurrence_tolerance();
    test_scoped_polarity_covariance_witness();
    test_smooth_stable_unstable_neutral_and_gauge();
    require(cdc_u2_self_test() == CDC_U2_OK,
            "internal U2 mathematical smoke test failed");
    puts("u2 variational adversarial tests: ok");
    return 0;
}

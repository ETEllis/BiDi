#include "cdc_linalg.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOLERANCE 1e-12

static void require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "u2-linalg-test: %s\n", message);
        exit(1);
    }
}

static int close_enough(double actual, double expected) {
    return fabs(actual - expected) <= TOLERANCE;
}

static void require_matrix(const cdc_matrix *matrix, const double *expected,
                           size_t count, const char *message) {
    size_t i;
    require(matrix && matrix->data && matrix->rows * matrix->cols == count,
            "matrix shape mismatch");
    for (i = 0; i < count; ++i) {
        if (!close_enough(matrix->data[i], expected[i])) {
            fprintf(stderr,
                    "u2-linalg-test: %s at index %zu: got %.17g expected %.17g\n",
                    message, i, matrix->data[i], expected[i]);
            exit(1);
        }
    }
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
            if (i != j && fabs(input->data[i * input->cols + j]) > TOLERANCE) {
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

static cdc_linalg_status dishonest_schur(
    const cdc_matrix *input, cdc_matrix *q, cdc_matrix *t,
    double *eigen_real, double *eigen_imag, void *context) {
    size_t i;
    size_t j;
    (void)context;
    for (i = 0; i < input->rows; ++i) {
        for (j = 0; j < input->cols; ++j) {
            q->data[i * q->cols + j] = i == j ? 1.0 : 0.0;
            t->data[i * t->cols + j] = 0.0;
        }
        eigen_real[i] = 0.0;
        eigen_imag[i] = 0.0;
    }
    return CDC_LINALG_OK;
}

static void test_ordered_product(void) {
    double first_data[] = {1.0, 1.0, 0.0, 1.0};
    double second_data[] = {1.0, 0.0, 1.0, 1.0};
    double product_data[4] = {0};
    double reverse_data[4] = {0};
    const double expected[] = {1.0, 1.0, 1.0, 2.0};
    const double reverse_expected[] = {2.0, 1.0, 1.0, 1.0};
    cdc_matrix first;
    cdc_matrix second;
    cdc_matrix product;
    cdc_matrix reverse;

    require(cdc_matrix_view(&first, 2, 2, first_data) == CDC_LINALG_OK,
            "first matrix view failed");
    require(cdc_matrix_view(&second, 2, 2, second_data) == CDC_LINALG_OK,
            "second matrix view failed");
    require(cdc_matrix_view(&product, 2, 2, product_data) == CDC_LINALG_OK,
            "product matrix view failed");
    require(cdc_matrix_view(&reverse, 2, 2, reverse_data) == CDC_LINALG_OK,
            "reverse matrix view failed");
    require(cdc_matrix_multiply(&product, &second, &first) == CDC_LINALG_OK,
            "ordered product failed");
    require(cdc_matrix_multiply(&reverse, &first, &second) == CDC_LINALG_OK,
            "reverse product failed");
    require_matrix(&product, expected, 4,
                   "later event must left-multiply earlier event");
    require_matrix(&reverse, reverse_expected, 4,
                   "reverse product oracle mismatch");
    require(memcmp(product.data, reverse.data, sizeof(product_data)) != 0,
            "non-commuting event order became unobservable");
}

static void test_alias_safe_product(void) {
    double left_data[] = {1.0, 1.0, 0.0, 1.0};
    double right_data[] = {1.0, 0.0, 1.0, 1.0};
    const double expected[] = {2.0, 1.0, 1.0, 1.0};
    cdc_matrix left;
    cdc_matrix right;

    require(cdc_matrix_view(&left, 2, 2, left_data) == CDC_LINALG_OK,
            "alias left view failed");
    require(cdc_matrix_view(&right, 2, 2, right_data) == CDC_LINALG_OK,
            "alias right view failed");
    require(cdc_matrix_multiply(&left, &left, &right) == CDC_LINALG_OK,
            "in-place product failed");
    require_matrix(&left, expected, 4, "in-place product corrupted an operand");
}

static void test_known_smooth_multipliers(void) {
    const double cases[] = {0.5, 1.5, 1.0};
    const char *labels[] = {"stable", "unstable", "neutral"};
    cdc_real_schur_backend backend = {"test-diagonal", diagonal_schur, NULL};
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        double storage[] = {cases[i]};
        cdc_matrix matrix;
        cdc_real_schur_result result;
        require(cdc_matrix_view(&matrix, 1, 1, storage) == CDC_LINALG_OK,
                "smooth multiplier matrix view failed");
        require(cdc_real_schur_compute(&matrix, &backend, TOLERANCE, &result) ==
                    CDC_LINALG_OK,
                labels[i]);
        require(result.dimension == 1, "smooth spectrum dimension mismatch");
        require(close_enough(result.eigen_real[0], cases[i]),
                "known smooth multiplier mismatch");
        require(close_enough(result.eigen_imag[0], 0.0),
                "known real multiplier gained imaginary component");
        require(result.reconstruction_residual <= TOLERANCE,
                "smooth Schur reconstruction residual exceeded tolerance");
        require(result.orthogonality_residual <= TOLERANCE,
                "smooth Schur orthogonality residual exceeded tolerance");
        cdc_real_schur_result_release(&result);
    }
}

static void test_backend_cannot_certify_itself(void) {
    double storage[] = {0.5, 0.0, 0.0, 1.5};
    cdc_matrix matrix;
    cdc_real_schur_result result;
    cdc_real_schur_backend backend = {"dishonest", dishonest_schur, NULL};
    require(cdc_matrix_view(&matrix, 2, 2, storage) == CDC_LINALG_OK,
            "dishonest backend matrix view failed");
    require(cdc_real_schur_compute(&matrix, &backend, TOLERANCE, &result) ==
                CDC_LINALG_VALIDATION_FAILED,
            "invalid Schur factorization was accepted");
}

static void test_system_backend_when_linked(void) {
    const cdc_real_schur_backend *backend = cdc_system_real_schur_backend();
    double storage[] = {0.0, -1.0, 1.0, 0.0};
    cdc_matrix matrix;
    cdc_real_schur_result result;
    if (!backend) {
        puts("u2 system real Schur backend: unavailable (typed hold permitted)");
        return;
    }
    require(cdc_matrix_view(&matrix, 2, 2, storage) == CDC_LINALG_OK,
            "system Schur matrix view failed");
    require(cdc_real_schur_compute(&matrix, NULL, 1e-10, &result) ==
                CDC_LINALG_OK,
            "system real Schur backend failed a rotation matrix");
    require(close_enough(result.eigen_real[0], 0.0) &&
                close_enough(result.eigen_real[1], 0.0),
            "system Schur complex pair has wrong real part");
    require(close_enough(fabs(result.eigen_imag[0]), 1.0) &&
                close_enough(fabs(result.eigen_imag[1]), 1.0) &&
                close_enough(result.eigen_imag[0], -result.eigen_imag[1]),
            "system Schur complex-conjugate pair mismatch");
    require(result.reconstruction_residual <= 1e-10 &&
                result.orthogonality_residual <= 1e-10 &&
                result.triangular_residual <= 1e-10,
            "system Schur validation residual exceeded tolerance");
    cdc_real_schur_result_release(&result);
    puts("u2 system real Schur backend: ok");
}

int main(void) {
    test_ordered_product();
    test_alias_safe_product();
    test_known_smooth_multipliers();
    test_backend_cannot_certify_itself();
    test_system_backend_when_linked();
    puts("u2 linalg adversarial tests: ok");
    return 0;
}

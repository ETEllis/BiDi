/* `cdc test` — Phase C typed test runner (gate CT3 seed).
 *
 * Discovery is free: every CDC job already carries inline expectations, so
 * a source file IS its own test. For each file the runner selects every
 * execution mode its directives require, executes it through the legacy
 * runtime entry point in a forked child (isolation without host process
 * spawning — the child calls back into this same binary's linked runtime),
 * and classifies results with the Amendment A7 typed policy:
 *
 *   +1 commit  — "status=accepted" records
 *    0 hold    — "status=held" records; a hold is EXPECTED only when the
 *                job that held is declared with expect-status=held; any
 *                unexpected hold fails --gate
 *   nest       — nest-integration records, counted separately
 *   -1 fail    — a child that exits nonzero (violated expectation, crash)
 *
 * Counts are always reported separately; a merged pass total is forbidden
 * (A7). Execution is deterministic: files in argument order, modes in a
 * fixed family order, one child at a time.
 */
#define _POSIX_C_SOURCE 200809L

#include "cmd_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../cdc_abi.h"
#include "../cdc_digest.h"
#include "../cdc_receipt.h"

int cdc_native_main(int argc, char **argv);

typedef struct {
    long commit;
    long hold;
    long nest;
    long fail;
    long unexpected_hold;
    long expected_hold;
    long runs;
    /* Prose-derived shadow counts. The gate classifies from typed effect
     * receipts; these are recomputed independently from the human report
     * lines and must agree exactly. A divergence means the two channels
     * have drifted and is a failure, not a warning. */
    long prose_commit;
    long prose_hold;
    long prose_nest;
    long parity_breaks;
} test_counts;

/* mode selection: which runtime mode families a parsed unit requires */
typedef struct {
    const char *mode;
    const char *const directives[7];
} mode_rule;

static const mode_rule MODE_RULES[] = {
    {"run", {"flow", "commit", "nest", NULL}},
    {"compile", {"compile", NULL}},
    {"interpret", {"interpret", NULL}},
    {"prove", {"proof", NULL}},
    {"surface",
     {"guard", "trace", "measure", "policy", "bridge", "counter", NULL}},
    {"council", {"deliberate", NULL}},
    {"evolve", {"evolve", NULL}},
    {"universal", {"universal", NULL}},
    {"persist", {"persist", NULL}},
};
enum { MODE_RULE_COUNT = sizeof(MODE_RULES) / sizeof(MODE_RULES[0]) };

static int program_contains(const cdc_program *program,
                            const char *const *directives) {
    size_t count = cdc_program_statement_count(program);
    size_t i;
    int d;
    for (i = 0; i < count; i++) {
        const char *directive =
            cdc_program_statement_directive(program, i);
        for (d = 0; directives[d]; d++) {
            if (directive && strcmp(directive, directives[d]) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* Is `job` declared in this unit as the EXECUTABLE statement of the given
 * form with expect-status=held? Review B3: the authorization is bound to
 * the typed executable statement identity (directive == the runtime record
 * form AND first argument == job id); an unrelated statement — a witness,
 * an expect line, a different form — must never authorize a runtime HOLD. */
static int job_expects_hold(const cdc_program *program, const char *form,
                            const char *job) {
    size_t count = cdc_program_statement_count(program);
    size_t i;
    for (i = 0; i < count; i++) {
        const char *directive =
            cdc_program_statement_directive(program, i);
        const char *arg = cdc_program_statement_arg(program, i, 0);
        const char *expect;
        if (!directive || strcmp(directive, form) != 0) {
            continue;
        }
        if (!arg || strcmp(arg, job) != 0) {
            continue;
        }
        expect = cdc_program_statement_attr(program, i, "expect-status");
        if (expect && strcmp(expect, "held") == 0) {
            return 1;
        }
    }
    return 0;
}

static void classify_prose(FILE *fp, test_counts *counts, int verbose) {
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "status=accepted")) {
            counts->prose_commit++;
        } else if (strstr(line, "status=held")) {
            counts->prose_hold++;
        }
        if (strncmp(line, "nest=", 5) == 0) {
            counts->prose_nest++;
        }
        if (verbose) {
            fputs(line, stdout);
        }
    }
}

/* Typed classifier: reads the effect receipts the runtime emitted for this
 * child. Every outcome is a field, not a substring — a payload containing
 * the text "status=accepted" can no longer manufacture a verdict, and
 * rewording a report line can no longer change one.
 *
 * Returns 0 when the receipt stream is unreadable or malformed. A
 * malformed receipt is a hard failure: the gate must not silently fall
 * back to prose, because that is exactly the fragility being removed. */
/* Ordered per-check parity vector (interface section 7). One record per
 * executed effect, in execution order, chained so that a record which
 * changes position changes every later trace digest too. The receipt is
 * the effects payload verbatim, so the vector and the receipt cannot
 * describe different effects. */
static FILE *vector_stream;
static cdc_vector_chain vector_chain;

static int classify_receipts(const char *path, const cdc_program *program,
                             const char *file, const char *mode,
                             test_counts *counts) {
    FILE *fp = fopen(path, "r");
    char line[4096];
    if (!fp) {
        fprintf(stderr, "cdc test: FAIL %s (%s): no effect receipts\n", file,
                mode);
        return 0;
    }
    while (fgets(line, sizeof(line), fp)) {
        cdc_receipt receipt;
        int parsed = cdc_receipt_parse(line, &receipt);
        if (parsed == 0) {
            continue; /* not a receipt line */
        }
        if (parsed < 0) {
            fprintf(stderr,
                    "cdc test: FAIL %s (%s): malformed effect receipt: %s",
                    file, mode, line);
            fclose(fp);
            return 0;
        }
        if (vector_stream) {
            char identifier[320];
            char record[640];
            size_t len = strlen(line);
            while (len > 0 &&
                   (line[len - 1] == '\n' || line[len - 1] == '\r')) {
                line[--len] = '\0';
            }
            snprintf(identifier, sizeof(identifier), "%s:%s:%s", file, mode,
                     receipt.job);
            if (cdc_vector_render(&vector_chain, identifier,
                                  cdc_receipt_decision(&receipt),
                                  receipt.trits[0] ? receipt.trits : NULL,
                                  line, len,
                                  receipt.closure[0] ? receipt.closure : NULL,
                                  record, sizeof(record)) < 0) {
                fprintf(stderr,
                        "cdc test: FAIL %s (%s): vector record too long\n",
                        file, mode);
                fclose(fp);
                return 0;
            }
            fprintf(vector_stream, "%s\n", record);
        }
        if (strcmp(receipt.kind, "nest") == 0) {
            counts->nest++;
            continue;
        }
        if (receipt.outcome == CDC_OUTCOME_ACCEPTED) {
            counts->commit++;
        } else if (receipt.outcome == CDC_OUTCOME_HELD) {
            counts->hold++;
            /* Authorization is bound to the declaring statement's typed
             * identity (review B3). The runtime resolved that when it built
             * the receipt; this re-checks it against the parsed program so
             * a forged receipt cannot authorize its own hold. */
            if (receipt.declared_hold &&
                job_expects_hold(program, receipt.kind, receipt.job)) {
                counts->expected_hold++;
            } else {
                counts->unexpected_hold++;
                fprintf(stderr,
                        "cdc test: unexpected hold in %s (%s): %s %s "
                        "reason=%s\n",
                        file, mode, receipt.kind, receipt.job,
                        receipt.reason);
            }
        }
    }
    fclose(fp);
    return 1;
}

static int run_mode(const char *file, const char *mode,
                    const cdc_program *program, test_counts *counts,
                    int verbose) {
    char out_path[128];
    char receipt_path[128];
    pid_t pid;
    int status;

    /* Review hardening: a unique per-invocation child-output path so
     * concurrent cdc test invocations cannot collide. */
    snprintf(out_path, sizeof(out_path), "build/cdc_test_child_%ld.txt",
             (long)getpid());
    snprintf(receipt_path, sizeof(receipt_path),
             "build/cdc_test_receipts_%ld.txt", (long)getpid());
    remove(receipt_path);
    fflush(NULL);
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "cdc test: FAIL %s (%s): fork failed\n", file,
                mode);
        counts->fail++; /* review B4: unobserved runs are failures */
        return 0;
    }
    if (pid == 0) {
        char *child_argv[4];
        /* The child emits typed effect receipts to their own channel; they
         * are never mixed into the captured stdout. */
        if (setenv("CDC_RECEIPTS", receipt_path, 1) != 0) {
            _exit(126);
        }
        if (!freopen(out_path, "w", stdout) ||
            !freopen(out_path, "a", stderr)) {
            _exit(127);
        }
        child_argv[0] = (char *)"cdc";
        child_argv[1] = (char *)mode;
        child_argv[2] = (char *)file;
        child_argv[3] = NULL;
        /* exit (not _exit) so the child's redirected stdio flushes; no
         * atexit handlers exist in this binary. */
        exit(cdc_native_main(3, child_argv));
    }
    if (waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "cdc test: FAIL %s (%s): waitpid failed\n", file,
                mode);
        counts->fail++;
        return 0;
    }
    counts->runs++;
    {
        long before_commit = counts->commit;
        long before_hold = counts->hold;
        long before_nest = counts->nest;
        long before_prose_commit = counts->prose_commit;
        long before_prose_hold = counts->prose_hold;
        long before_prose_nest = counts->prose_nest;
        FILE *fp = fopen(out_path, "r");
        if (!fp) {
            fprintf(stderr,
                    "cdc test: FAIL %s (%s): child output unreadable\n",
                    file, mode);
            counts->fail++;
            return 0;
        }
        classify_prose(fp, counts, verbose);
        fclose(fp);
        if (!classify_receipts(receipt_path, program, file, mode, counts)) {
            counts->fail++;
            return 0;
        }
        /* Receipt/prose parity. The typed channel decides the verdict; this
         * proves the human channel is telling the same story. If a report
         * line is reworded, or an effect gains a receipt without a line (or
         * the reverse), the build fails here rather than the two quietly
         * diverging. */
        if (counts->commit - before_commit !=
                counts->prose_commit - before_prose_commit ||
            counts->hold - before_hold !=
                counts->prose_hold - before_prose_hold ||
            counts->nest - before_nest !=
                counts->prose_nest - before_prose_nest) {
            fprintf(stderr,
                    "cdc test: FAIL %s (%s): receipt/prose divergence "
                    "(receipts commit=%ld hold=%ld nest=%ld; prose "
                    "commit=%ld hold=%ld nest=%ld)\n",
                    file, mode, counts->commit - before_commit,
                    counts->hold - before_hold, counts->nest - before_nest,
                    counts->prose_commit - before_prose_commit,
                    counts->prose_hold - before_prose_hold,
                    counts->prose_nest - before_prose_nest);
            counts->parity_breaks++;
        }
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        counts->fail++;
        fprintf(stderr, "cdc test: FAIL %s (%s) exit=%d\n", file, mode,
                WIFEXITED(status) ? WEXITSTATUS(status) : -1);
        {
            FILE *fp = fopen(out_path, "r");
            char line[4096];
            if (fp) {
                while (fgets(line, sizeof(line), fp)) {
                    fputs(line, stderr);
                }
                fclose(fp);
            }
        }
        return 0;
    }
    return 1;
}

int cdc_cmd_test(int argc, char **argv) {
    test_counts counts;
    int gate = 0, verbose = 0;
    int i, m;
    int first_file = 0;
    int vector_path_index = -1;

    memset(&counts, 0, sizeof(counts));
    cdc_vector_chain_init(&vector_chain);
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--gate") == 0) {
            gate = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--vectors") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "cdc test --vectors: no path given\n");
                return 2;
            }
            vector_stream = fopen(argv[i + 1], "w");
            if (!vector_stream) {
                fprintf(stderr, "cdc test --vectors: cannot write %s\n",
                        argv[i + 1]);
                return 2;
            }
            vector_path_index = i + 1;
            i++;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "cdc test: unknown option %s\n", argv[i]);
            return 2;
        } else if (!first_file && i != vector_path_index) {
            first_file = i;
        }
    }
    if (!first_file && (argc == 0 || argv[0][0] == '-')) {
        fprintf(stderr,
                "usage: cdc test [--gate] [--verbose] <files...>\n");
        return 2;
    }

    for (i = 0; i < argc; i++) {
        cdc_program *program = NULL;
        cdc_status status;
        if (argv[i][0] == '-' || i == vector_path_index) {
            continue;
        }
        status = cdc_program_parse(argv[i], NULL, 0, &program);
        if (status != CDC_OK) {
            fprintf(stderr, "cdc test: %s: %s\n", argv[i],
                    cdc_status_name(status));
            cdc_program_destroy(program);
            counts.fail++;
            continue;
        }
        for (m = 0; m < MODE_RULE_COUNT; m++) {
            if (program_contains(program, MODE_RULES[m].directives)) {
                run_mode(argv[i], MODE_RULES[m].mode, program, &counts,
                         verbose);
            }
        }
        cdc_program_destroy(program);
    }

    /* A7: separate counts, never a merged total; unexpected holds are
     * failures while expected holds are first-class equilibrium results.
     * Review B4: a run set with zero executed checks carries no evidence
     * and is never green. */
    {
        int failed = counts.fail > 0 || counts.unexpected_hold > 0 ||
                     counts.parity_breaks > 0 || counts.runs == 0;
        /* CT0: the verdict names the corpus it was reached over. Without
         * this a verdict says a run passed but not what it ran on, which
         * is not evidence anyone can re-check. */
        char corpus[96];
        {
            const char *inputs[256];
            size_t input_count = 0;
            uint8_t digest[CDC_DIGEST_SIZE];
            int k;
            for (k = 0; k < argc && input_count < 256; k++) {
                if (argv[k][0] == '-' || k == vector_path_index) {
                    continue;
                }
                inputs[input_count++] = argv[k];
            }
            if (input_count > 0 &&
                cdc_digest_corpus(inputs, input_count, digest)) {
                cdc_digest_hex(digest, corpus, sizeof(corpus));
            } else {
                snprintf(corpus, sizeof(corpus), "unavailable");
                failed = 1; /* an unidentifiable corpus is not a pass */
            }
        }
        printf("cdc test %s runs=%ld commit=%ld hold=%ld (expected=%ld "
               "unexpected=%ld) nest=%ld fail=%ld parity=%ld corpus=%s%s\n",
               failed ? "FAIL" : "ok", counts.runs, counts.commit,
               counts.hold, counts.expected_hold, counts.unexpected_hold,
               counts.nest, counts.fail, counts.parity_breaks, corpus,
               counts.runs == 0 ? " (no executable stage selected)" : "");
        (void)gate; /* strictness is unconditional; flag kept for CLI
                       stability */
        if (vector_stream) {
            fclose(vector_stream);
            vector_stream = NULL;
        }
        return failed ? 1 : 0;
    }
}

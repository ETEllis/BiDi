/* cdc_frontend_check: grammar-1 frontend differential harness (gate CT1).
 *
 * Modes:
 *   dump <files...>        emit grammar-0-equivalent declaration records;
 *                          byte-compared against `cdc_boot.py --dump`
 *   canon <files...>       emit canonical grammar-1 serialization
 *   roundtrip <files...>   parse -> canonical -> reparse -> structural equal
 *   attr-parity <files...> field-for-field comparison of frontend attribute
 *                          extraction vs the legacy cdc_read_attr scanner,
 *                          with typed divergence classes
 *   bounds                 adversarial corpus: typed diagnostics, no crash
 *   oom <file>             allocator-failure injection at every allocation
 *   reject <files...>      every file must produce >=1 error diagnostic
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cdc_abi.h"
#include "cdc_ast.h"
#include "cdc_diagnostic.h"
#include "cdc_digest.h"
#include "cdc_lexer.h"
#include "cdc_parser.h"
#include "cdc_source.h"
#include "cdc_store.h"

static const char *base_name(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int parse_or_report(const char *path, cdc_unit *program,
                           cdc_diag_list *diags) {
    if (!cdc_unit_parse_file(path, program, diags)) {
        fprintf(stderr, "cdc-frontend: %s: read or allocation failure\n",
                path);
        return 0;
    }
    if (diags->errors > 0) {
        cdc_diag_list_print(diags, stderr);
        return 0;
    }
    return 1;
}

/* ---- dump ---------------------------------------------------------- */

static void dump_stmt(const cdc_unit *program, const cdc_stmt *stmt) {
    const char *file = base_name(program->file);
    size_t i, j;
    if (stmt->kind == CDC_STMT_END) {
        return; /* grammar 0 skips structural end lines before dispatch */
    }
    printf("%s:%d|%s|", file, stmt->line, cdc_stmt_directive(stmt));
    if (stmt->kind == CDC_STMT_EXPECT) {
        for (i = 1; i < stmt->token_count; i++) {
            if (i > 1) {
                putchar(' ');
            }
            fputs(stmt->tokens[i].text, stdout);
        }
        printf("|\n");
        return;
    }
    {
        int first = 1;
        for (i = 1; i < stmt->token_count; i++) {
            if (!memchr(stmt->tokens[i].text, '=', stmt->tokens[i].length)) {
                if (!first) {
                    putchar(',');
                }
                fputs(stmt->tokens[i].text, stdout);
                first = 0;
            }
        }
    }
    putchar('|');
    {
        /* dict semantics: first-occurrence order, last value wins */
        int first = 1;
        for (i = 1; i < stmt->token_count; i++) {
            const char *eq =
                memchr(stmt->tokens[i].text, '=', stmt->tokens[i].length);
            size_t key_len;
            int seen_before = 0;
            if (!eq) {
                continue;
            }
            key_len = (size_t)(eq - stmt->tokens[i].text);
            for (j = 1; j < i; j++) {
                const char *prior_eq = memchr(stmt->tokens[j].text, '=',
                                              stmt->tokens[j].length);
                if (prior_eq &&
                    (size_t)(prior_eq - stmt->tokens[j].text) == key_len &&
                    strncmp(stmt->tokens[j].text, stmt->tokens[i].text,
                            key_len) == 0) {
                    seen_before = 1;
                    break;
                }
            }
            if (seen_before) {
                continue;
            }
            {
                char key[256];
                const char *value;
                if (key_len >= sizeof(key)) {
                    fprintf(stderr, "cdc-frontend: attribute key too long\n");
                    exit(1);
                }
                memcpy(key, stmt->tokens[i].text, key_len);
                key[key_len] = '\0';
                value = cdc_stmt_attr(stmt, key);
                if (!first) {
                    putchar(';');
                }
                printf("%s=%s", key, value ? value : "");
                first = 0;
            }
        }
    }
    printf("|\n");
}

static int cmd_dump(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; i++) {
        cdc_unit program;
        cdc_diag_list diags;
        size_t s;
        cdc_diag_list_init(&diags);
        if (!parse_or_report(argv[i], &program, &diags)) {
            cdc_unit_free(&program);
            cdc_diag_list_free(&diags);
            return 1;
        }
        for (s = 0; s < program.count; s++) {
            dump_stmt(&program, &program.stmts[s]);
        }
        cdc_unit_free(&program);
        cdc_diag_list_free(&diags);
    }
    return 0;
}

/* ---- canon / roundtrip --------------------------------------------- */

static int cmd_canon(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; i++) {
        cdc_unit program;
        cdc_diag_list diags;
        cdc_diag_list_init(&diags);
        if (!parse_or_report(argv[i], &program, &diags)) {
            cdc_unit_free(&program);
            cdc_diag_list_free(&diags);
            return 1;
        }
        cdc_unit_canonical(&program, stdout);
        cdc_unit_free(&program);
        cdc_diag_list_free(&diags);
    }
    return 0;
}

static int cmd_roundtrip(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; i++) {
        cdc_unit first, second;
        cdc_diag_list diags;
        char *canon_buf = NULL;
        size_t canon_size = 0;
        FILE *mem;

        cdc_diag_list_init(&diags);
        if (!parse_or_report(argv[i], &first, &diags)) {
            cdc_unit_free(&first);
            cdc_diag_list_free(&diags);
            return 1;
        }
        mem = open_memstream(&canon_buf, &canon_size);
        if (!mem) {
            fprintf(stderr, "cdc-frontend: memstream failure\n");
            return 1;
        }
        cdc_unit_canonical(&first, mem);
        fclose(mem);
        if (!cdc_unit_parse_buffer(canon_buf, canon_size, first.file,
                                      &second, &diags) ||
            diags.errors > 0) {
            fprintf(stderr, "cdc-frontend: %s: canonical form failed to "
                            "reparse\n",
                    argv[i]);
            cdc_diag_list_print(&diags, stderr);
            return 1;
        }
        if (!cdc_unit_equal(&first, &second)) {
            fprintf(stderr, "cdc-frontend: %s: roundtrip mismatch\n",
                    argv[i]);
            return 1;
        }
        free(canon_buf);
        cdc_unit_free(&first);
        cdc_unit_free(&second);
        cdc_diag_list_free(&diags);
    }
    printf("frontend roundtrip ok files=%d\n", argc);
    return 0;
}

/* ---- attr-parity ---------------------------------------------------- */

typedef struct {
    long checked;
    long quoting;
    long collision;
    long duplicate;
    long skipped_long;
    long failed;
} parity_counts;

static void parity_line(const cdc_unit *program, const cdc_stmt *stmt,
                        const char *raw_line, parity_counts *counts) {
    char stripped[8192];
    size_t i, j;

    snprintf(stripped, sizeof(stripped), "%s", raw_line);
    cdc_strip_comment(stripped);

    for (i = 1; i < stmt->token_count; i++) {
        const char *eq =
            memchr(stmt->tokens[i].text, '=', stmt->tokens[i].length);
        char key[256];
        char legacy[4096];
        const char *frontend_first;
        const char *frontend_last;
        size_t key_len;
        int duplicated = 0;

        if (!eq) {
            continue;
        }
        key_len = (size_t)(eq - stmt->tokens[i].text);
        if (key_len == 0 || key_len >= sizeof(key)) {
            counts->skipped_long++;
            continue;
        }
        memcpy(key, stmt->tokens[i].text, key_len);
        key[key_len] = '\0';
        if (strlen(key) > 60) { /* legacy needle buffer is 64 with "=" */
            counts->skipped_long++;
            continue;
        }
        /* only evaluate each key once per line (at its first occurrence) */
        {
            int earlier = 0;
            for (j = 1; j < i; j++) {
                const char *prior_eq = memchr(stmt->tokens[j].text, '=',
                                              stmt->tokens[j].length);
                if (prior_eq &&
                    (size_t)(prior_eq - stmt->tokens[j].text) == key_len &&
                    strncmp(stmt->tokens[j].text, key, key_len) == 0) {
                    earlier = 1;
                    break;
                }
            }
            if (earlier) {
                continue;
            }
        }
        for (j = i + 1; j < stmt->token_count; j++) {
            const char *later_eq = memchr(stmt->tokens[j].text, '=',
                                          stmt->tokens[j].length);
            if (later_eq &&
                (size_t)(later_eq - stmt->tokens[j].text) == key_len &&
                strncmp(stmt->tokens[j].text, key, key_len) == 0) {
                duplicated = 1;
                break;
            }
        }

        frontend_first = cdc_stmt_attr_first(stmt, key);
        frontend_last = cdc_stmt_attr(stmt, key);
        counts->checked++;
        if (duplicated ||
            (frontend_first && frontend_last &&
             strcmp(frontend_first, frontend_last) != 0)) {
            counts->duplicate++;
        }

        if (!cdc_read_attr(stripped, key, legacy, sizeof(legacy))) {
            counts->failed++;
            fprintf(stderr, "attr-parity FAIL %s:%d %s: legacy scanner "
                            "found nothing\n",
                    base_name(program->file), stmt->line, key);
            continue;
        }
        if (frontend_first && strcmp(legacy, frontend_first) == 0) {
            continue; /* exact agreement */
        }
        /* collision: the legacy strstr hit begins before this token */
        {
            char needle[64];
            const char *hit;
            snprintf(needle, sizeof(needle), "%s=", key);
            hit = strstr(stripped, needle);
            /* a hit not immediately preceded by start-of-line or whitespace
             * sits inside another token: the legacy scanner read from the
             * middle of an unrelated attribute (substring collision) */
            if (hit && hit != stripped && hit[-1] != ' ' &&
                hit[-1] != '\t') {
                counts->collision++;
                fprintf(stderr, "attr-parity collision %s:%d %s\n",
                        base_name(program->file), stmt->line, key);
                continue;
            }
        }
        /* quoting divergence: legacy retains quotes / truncates at space */
        if (frontend_first && legacy[0] == '"') {
            const char *body = legacy + 1;
            size_t body_len = strlen(body);
            if (body_len > 0 && body[body_len - 1] == '"') {
                body_len--;
            }
            if (strncmp(frontend_first, body, body_len) == 0 &&
                (frontend_first[body_len] == '\0' ||
                 strchr(frontend_first + body_len, ' ') != NULL ||
                 frontend_first[body_len] == ' ')) {
                counts->quoting++;
                continue;
            }
        }
        counts->failed++;
        fprintf(stderr,
                "attr-parity FAIL %s:%d %s: legacy=%s frontend=%s\n",
                base_name(program->file), stmt->line, key, legacy,
                frontend_first ? frontend_first : "<none>");
    }
}

static int cmd_attr_parity(int argc, char **argv) {
    parity_counts counts;
    int i;
    memset(&counts, 0, sizeof(counts));
    for (i = 0; i < argc; i++) {
        cdc_unit program;
        cdc_diag_list diags;
        FILE *fp;
        char raw[8192];
        int line_no = 0;
        size_t s = 0;

        cdc_diag_list_init(&diags);
        if (!parse_or_report(argv[i], &program, &diags)) {
            cdc_unit_free(&program);
            cdc_diag_list_free(&diags);
            return 1;
        }
        fp = fopen(argv[i], "r");
        if (!fp) {
            fprintf(stderr, "cdc-frontend: cannot reopen %s\n", argv[i]);
            return 1;
        }
        while (fgets(raw, sizeof(raw), fp)) {
            line_no++;
            cdc_trim_newline(raw);
            while (s < program.count && program.stmts[s].line < line_no) {
                s++;
            }
            if (s < program.count && program.stmts[s].line == line_no &&
                program.stmts[s].kind != CDC_STMT_END) {
                parity_line(&program, &program.stmts[s], raw, &counts);
            }
        }
        fclose(fp);
        cdc_unit_free(&program);
        cdc_diag_list_free(&diags);
    }
    printf("frontend attr-parity checked=%ld quoting=%ld collision=%ld "
           "duplicate=%ld skipped=%ld failed=%ld\n",
           counts.checked, counts.quoting, counts.collision,
           counts.duplicate, counts.skipped_long, counts.failed);
    return counts.failed == 0 ? 0 : 1;
}

/* ---- bounds --------------------------------------------------------- */

typedef struct {
    const char *name;
    const char *code; /* expected diagnostic code, NULL = must accept */
    const char *buffer;
    size_t length; /* 0 = strlen(buffer) */
} bounds_case;

static int diags_contain(const cdc_diag_list *diags, const char *code) {
    size_t i;
    for (i = 0; i < diags->count; i++) {
        if (strcmp(diags->items[i].code, code) == 0) {
            return 1;
        }
    }
    return 0;
}

static int cmd_bounds(void) {
    static const char nul_case[] = "flow x a\0b";
    bounds_case cases[16];
    size_t n = 0, i;
    char *long_line = NULL;
    char *many_tokens = NULL;
    int failures = 0;

    cases[n].name = "trailing-escape";
    cases[n].code = "CDC011";
    cases[n].buffer = "flow x a=1\\";
    cases[n].length = 0;
    n++;
    cases[n].name = "unclosed-single";
    cases[n].code = "CDC012";
    cases[n].buffer = "commit y label='open";
    cases[n].length = 0;
    n++;
    cases[n].name = "unclosed-double-escape";
    cases[n].code = "CDC011";
    cases[n].buffer = "commit y label=\"open\\";
    cases[n].length = 0;
    n++;
    cases[n].name = "unknown-directive";
    cases[n].code = "CDC020";
    cases[n].buffer = "bogus x y=1";
    cases[n].length = 0;
    n++;
    cases[n].name = "witness-missing-id";
    cases[n].code = "CDC021";
    cases[n].buffer = "witness claim=\"only attrs\"";
    cases[n].length = 0;
    n++;
    cases[n].name = "duplicate-framework";
    cases[n].code = "CDC022";
    cases[n].buffer = "framework F9 label=a requires=r permits=p\n"
                      "framework F9 label=b requires=r permits=p";
    cases[n].length = 0;
    n++;
    cases[n].name = "embedded-nul";
    cases[n].code = "CDC014";
    cases[n].buffer = nul_case;
    cases[n].length = sizeof(nul_case) - 1;
    n++;
    cases[n].name = "empty-source";
    cases[n].code = NULL;
    cases[n].buffer = "";
    cases[n].length = 0;
    n++;
    cases[n].name = "quote-concat";
    cases[n].code = NULL;
    cases[n].buffer = "flow con'cat'\"enate\" a=1";
    cases[n].length = 0;
    n++;
    cases[n].name = "escaped-space-token";
    cases[n].code = NULL;
    cases[n].buffer = "flow with\\ space b=2";
    cases[n].length = 0;
    n++;

    /* line-too-long */
    {
        size_t big = (size_t)CDC_LEX_MAX_LINE + 8;
        long_line = malloc(big + 1);
        if (long_line) {
            memset(long_line, 'a', big);
            long_line[big] = '\0';
            cases[n].name = "line-too-long";
            cases[n].code = "CDC010";
            cases[n].buffer = long_line;
            cases[n].length = big;
            n++;
        }
    }
    /* too many tokens */
    {
        size_t count = (size_t)CDC_LEX_MAX_TOKENS + 8;
        size_t bytes = count * 2 + 16;
        many_tokens = malloc(bytes);
        if (many_tokens) {
            char *p = many_tokens;
            size_t k;
            memcpy(p, "flow", 4);
            p += 4;
            for (k = 0; k < count; k++) {
                *p++ = ' ';
                *p++ = 'a';
            }
            *p = '\0';
            cases[n].name = "too-many-tokens";
            cases[n].code = "CDC013";
            cases[n].buffer = many_tokens;
            cases[n].length = (size_t)(p - many_tokens);
            n++;
        }
    }

    for (i = 0; i < n; i++) {
        cdc_unit program;
        cdc_diag_list diags;
        size_t length =
            cases[i].length ? cases[i].length : strlen(cases[i].buffer);
        cdc_diag_list_init(&diags);
        cdc_unit_parse_buffer(cases[i].buffer, length, cases[i].name,
                                 &program, &diags);
        if (cases[i].code) {
            if (!diags_contain(&diags, cases[i].code)) {
                fprintf(stderr, "bounds FAIL %s: expected %s\n",
                        cases[i].name, cases[i].code);
                failures++;
            }
        } else if (diags.errors != 0) {
            fprintf(stderr, "bounds FAIL %s: unexpected rejection\n",
                    cases[i].name);
            cdc_diag_list_print(&diags, stderr);
            failures++;
        }
        cdc_unit_free(&program);
        cdc_diag_list_free(&diags);
    }
    free(long_line);
    free(many_tokens);
    if (failures) {
        return 1;
    }
    printf("frontend bounds ok cases=%d\n", (int)n);
    return 0;
}

/* ---- oom ------------------------------------------------------------ */

static long oom_fail_at;
static long oom_counter;

static void *failing_alloc(void *ptr, size_t size) {
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    oom_counter++;
    if (oom_counter == oom_fail_at) {
        return NULL;
    }
    return realloc(ptr, size);
}

static int cmd_oom(const char *path) {
    long attempt;
    for (attempt = 1; attempt < 100000; attempt++) {
        cdc_unit program;
        cdc_diag_list diags;
        int completed;
        oom_fail_at = attempt;
        oom_counter = 0;
        cdc_frontend_set_allocator(failing_alloc);
        cdc_diag_list_init(&diags);
        completed = cdc_unit_parse_file(path, &program, &diags);
        cdc_frontend_set_allocator(NULL);
        {
            int clean_success = completed && !diags.out_of_memory &&
                                oom_counter < oom_fail_at;
            cdc_unit_free(&program);
            cdc_diag_list_free(&diags);
            if (clean_success) {
                printf("frontend oom ok attempts=%ld\n", attempt);
                return 0;
            }
        }
    }
    fprintf(stderr, "frontend oom: no clean completion within bound\n");
    return 1;
}

/* ---- ABI counterexamples (2026-07-23 adversarial review) ------------ */

/* Defect-1 regression: a rejected source under an arbitrarily long path
 * must serialize to complete JSON containing the full path and message —
 * no fixed-slot truncation, no out-of-bounds. */
static int cmd_abi_diag(const char *path) {
    cdc_program *program = NULL;
    cdc_result *result = NULL;
    char *json = NULL;
    size_t length = 0;
    cdc_status status = cdc_program_parse(path, NULL, 0, &program);

    if (status != CDC_ERR_PARSE) {
        fprintf(stderr, "abi-diag FAIL: expected parse rejection, got %s\n",
                cdc_status_name(status));
        return 1;
    }
    if (cdc_program_diagnostics(program, &result) != CDC_OK ||
        cdc_result_serialize(result, &json, &length) != CDC_OK) {
        fprintf(stderr, "abi-diag FAIL: diagnostics/serialize\n");
        return 1;
    }
    if (!strstr(json, path)) {
        fprintf(stderr, "abi-diag FAIL: serialized JSON lacks full path\n");
        return 1;
    }
    if (!strstr(json, "error[CDC")) {
        fprintf(stderr, "abi-diag FAIL: serialized JSON lacks message\n");
        return 1;
    }
    fwrite(json, 1, length, stdout);
    fputc('\n', stdout);
    printf("abi-diag ok bytes=%zu\n", length);
    cdc_bytes_free(json);
    cdc_result_destroy(result);
    cdc_program_destroy(program);
    return 0;
}

/* Defect-2 regression: the ABI status for a path must match expectation
 * (io | parse | ok | memory) — directories, FIFOs, and devices are io,
 * never empty accepted programs. */
static int cmd_abi_io(const char *path, const char *expect) {
    cdc_program *program = NULL;
    cdc_status status = cdc_program_parse(path, NULL, 0, &program);
    const char *name = cdc_status_name(status);
    if (strcmp(name, expect) != 0) {
        fprintf(stderr, "abi-io FAIL %s: expected %s got %s\n", path, expect,
                name);
        cdc_program_destroy(program);
        return 1;
    }
    if (status == CDC_ERR_IO && program != NULL) {
        fprintf(stderr, "abi-io FAIL %s: io status must not yield a handle\n",
                path);
        return 1;
    }
    printf("abi-io ok path=%s status=%s\n", path, name);
    cdc_program_destroy(program);
    return 0;
}

/* Defect-2 regression, mid-read arm: reading a directory descriptor through
 * the stream path must surface ferror() as a typed CDC003, never EOF-as-
 * empty-program. (The production file path rejects directories before
 * reading; this proves the backstop.) */
static int cmd_io_mid_read(const char *dir_path) {
    int fd = open(dir_path, O_RDONLY);
    FILE *fp;
    cdc_unit unit;
    cdc_diag_list diags;
    int completed;

    if (fd < 0) {
        fprintf(stderr, "io-mid-read FAIL: cannot open %s\n", dir_path);
        return 1;
    }
    fp = fdopen(fd, "rb");
    if (!fp) {
        fprintf(stderr, "io-mid-read FAIL: fdopen\n");
        return 1;
    }
    cdc_diag_list_init(&diags);
    completed = cdc_unit_parse_stream(fp, dir_path, &unit, &diags);
    fclose(fp);
    if (completed != 0 || !diags_contain(&diags, "CDC003")) {
        fprintf(stderr,
                "io-mid-read FAIL: completed=%d (want 0 with CDC003)\n",
                completed);
        return 1;
    }
    printf("io-mid-read ok reason=CDC003\n");
    cdc_unit_free(&unit);
    cdc_diag_list_free(&diags);
    return 0;
}

/* Allocation-failure sweep over the full ABI diagnostic pipeline: at every
 * injected failure the pipeline must return a typed status (CDC_ERR_MEMORY
 * or a completed parse status) and release everything (leaks are caught by
 * the sanitizer build running this same mode). */
static int cmd_oom_abi(const char *path) {
    long attempt;
    for (attempt = 1; attempt < 100000; attempt++) {
        cdc_program *program = NULL;
        cdc_result *result = NULL;
        char *json = NULL;
        size_t length = 0;
        cdc_status status;
        int clean = 1;

        oom_fail_at = attempt;
        oom_counter = 0;
        cdc_frontend_set_allocator(failing_alloc);
        status = cdc_program_parse(path, NULL, 0, &program);
        if (status == CDC_OK || status == CDC_ERR_PARSE) {
            if (cdc_program_diagnostics(program, &result) == CDC_OK) {
                if (cdc_result_serialize(result, &json, &length) != CDC_OK) {
                    clean = 0;
                }
            } else {
                clean = 0;
            }
        } else if (status != CDC_ERR_MEMORY && status != CDC_ERR_IO) {
            cdc_frontend_set_allocator(NULL);
            fprintf(stderr, "oom-abi FAIL: unexpected status %s\n",
                    cdc_status_name(status));
            return 1;
        }
        cdc_frontend_set_allocator(NULL);
        cdc_bytes_free(json);
        cdc_result_destroy(result);
        cdc_program_destroy(program);
        if (clean && oom_counter < oom_fail_at &&
            (status == CDC_OK || status == CDC_ERR_PARSE)) {
            printf("oom-abi ok attempts=%ld\n", attempt);
            return 0;
        }
    }
    fprintf(stderr, "oom-abi FAIL: no clean completion within bound\n");
    return 1;
}

/* ---- store: digest vectors + crash matrix (Phase D, CT4/MM1 seed) --- */

static int digest_self_test(void) {
    uint8_t digest[CDC_DIGEST_SIZE];
    char hex[80];
    /* Published BLAKE3 vectors (canonical algorithm per Amendment A3). */
    cdc_digest("", 0, digest);
    cdc_digest_hex(digest, hex, sizeof(hex));
    if (strcmp(hex, "blake3:af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9ad"
                    "c112b7cc9a93cae41f3262") != 0) {
        fprintf(stderr, "digest FAIL: empty vector -> %s\n", hex);
        return 0;
    }
    cdc_digest("abc", 3, digest);
    cdc_digest_hex(digest, hex, sizeof(hex));
    if (strcmp(hex, "blake3:6437b3ac38465133ffb63b75273a8db548c558465d"
                    "79db03fd359c6cd5bd9d85") != 0) {
        fprintf(stderr, "digest FAIL: abc vector -> %s\n", hex);
        return 0;
    }
    return 1;
}

/* Full reference-vector sweep against the committed fixture: single-block,
 * block-boundary, chunk-boundary, and multi-level tree inputs, each hashed
 * both one-shot and through irregular streaming splits so update-path
 * boundary handling is covered too. */
static int cmd_digest_vectors(const char *path) {
    FILE *fp = fopen(path, "r");
    char line[256];
    long pass = 0, fail = 0;

    if (!fp) {
        fprintf(stderr, "digest-vectors: cannot open %s\n", path);
        return 1;
    }
    if (!digest_self_test()) {
        fclose(fp);
        return 1;
    }
    while (fgets(line, sizeof(line), fp)) {
        long n;
        char want[80];
        unsigned char *buf;
        long i, offset, step;
        cdc_digest_ctx ctx;
        uint8_t out[CDC_DIGEST_SIZE];
        char got[80];

        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        if (sscanf(line, "%ld %79s", &n, want) != 2) {
            continue;
        }
        buf = malloc((size_t)(n ? n : 1));
        if (!buf) {
            fclose(fp);
            return 1;
        }
        for (i = 0; i < n; i++) {
            buf[i] = (unsigned char)(i % 251);
        }
        /* one-shot */
        cdc_digest(buf, (size_t)n, out);
        cdc_digest_hex(out, got, sizeof(got));
        if (strcmp(got + 7, want) != 0) {
            fprintf(stderr, "digest-vectors FAIL len=%ld one-shot\n", n);
            fail++;
            free(buf);
            continue;
        }
        /* streaming in growing irregular increments */
        cdc_digest_init(&ctx);
        offset = 0;
        step = 1;
        while (offset < n) {
            long take = (n - offset < step) ? n - offset : step;
            cdc_digest_update(&ctx, buf + offset, (size_t)take);
            offset += take;
            step = step * 7 + 13;
        }
        cdc_digest_final(&ctx, out);
        cdc_digest_hex(out, got, sizeof(got));
        if (strcmp(got + 7, want) != 0) {
            fprintf(stderr, "digest-vectors FAIL len=%ld streaming\n", n);
            fail++;
            free(buf);
            continue;
        }
        free(buf);
        pass++;
    }
    fclose(fp);
    printf("digest-vectors ok vectors=%ld failed=%ld\n", pass, fail);
    return fail == 0 ? 0 : 1;
}

/* Native file digest so evidence records are produced by the same
 * implementation the runtime uses (no external digest tool). */
static int cmd_digest_file(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; i++) {
        uint8_t digest[CDC_DIGEST_SIZE];
        char hex[80];
        if (!cdc_digest_file(argv[i], digest)) {
            fprintf(stderr, "digest-file: cannot read %s\n", argv[i]);
            return 1;
        }
        cdc_digest_hex(digest, hex, sizeof(hex));
        printf("%s  %s\n", hex, argv[i]);
    }
    return 0;
}

static int store_commit_txn(cdc_store *store, int which) {
    char payload[64];
    int i;
    for (i = 0; i < 3; i++) {
        snprintf(payload, sizeof(payload), "txn-%d-event-%d", which, i);
        if (cdc_store_stage(store, payload, strlen(payload)) !=
            CDC_STORE_OK) {
            return 0;
        }
    }
    return cdc_store_commit(store) == CDC_STORE_OK;
}

/* Builds a reference store with `txns` committed transactions under
 * dir/name and returns its replay digest. */
static int store_reference_digest(const char *base, const char *name,
                                  int txns, char *out, size_t out_size) {
    char dir[512];
    cdc_store *store = NULL;
    int t;
    snprintf(dir, sizeof(dir), "%s/%s", base, name);
    if (cdc_store_open(dir, &store, NULL) != CDC_STORE_OK) {
        return 0;
    }
    for (t = 1; t <= txns; t++) {
        if (!store_commit_txn(store, t)) {
            cdc_store_close(store);
            return 0;
        }
    }
    if (cdc_store_replay(store, out, out_size) != CDC_STORE_OK) {
        cdc_store_close(store);
        return 0;
    }
    cdc_store_close(store);
    return 1;
}

static int cmd_store_crash(const char *base) {
    char ref_old[80], ref_new[80];
    int boundaries = 0, old_state = 0, new_state = 0;
    int k;

    if (!digest_self_test()) {
        return 1;
    }
    if (!store_reference_digest(base, "ref1", 1, ref_old,
                                sizeof(ref_old)) ||
        !store_reference_digest(base, "ref2", 2, ref_new,
                                sizeof(ref_new))) {
        fprintf(stderr, "store-crash FAIL: reference stores\n");
        return 1;
    }
    if (strcmp(ref_old, ref_new) == 0) {
        fprintf(stderr, "store-crash FAIL: reference digests collide\n");
        return 1;
    }

    /* boundary count for the second transaction (3 events) */
    {
        char dir[512];
        cdc_store *probe = NULL;
        char payload[8] = "p";
        int i;
        snprintf(dir, sizeof(dir), "%s/probe", base);
        if (cdc_store_open(dir, &probe, NULL) != CDC_STORE_OK) {
            return 1;
        }
        for (i = 0; i < 3; i++) {
            cdc_store_stage(probe, payload, 1);
        }
        boundaries = cdc_store_commit_operations(probe);
        cdc_store_close(probe);
    }

    for (k = 1; k <= boundaries; k++) {
        char dir[512];
        cdc_store *store = NULL;
        cdc_store_status status;
        char replayed[80];
        uint64_t sealed;
        int recovered = 0;

        snprintf(dir, sizeof(dir), "%s/crash_%d", base, k);
        if (cdc_store_open(dir, &store, NULL) != CDC_STORE_OK ||
            !store_commit_txn(store, 1)) {
            fprintf(stderr, "store-crash FAIL: baseline txn (k=%d)\n", k);
            return 1;
        }
        {
            char payload[64];
            int i;
            for (i = 0; i < 3; i++) {
                snprintf(payload, sizeof(payload), "txn-2-event-%d", i);
                cdc_store_stage(store, payload, strlen(payload));
            }
        }
        cdc_store_set_fail_after(store, k);
        status = cdc_store_commit(store);
        cdc_store_close(store);
        if (status == CDC_STORE_OK) {
            fprintf(stderr,
                    "store-crash FAIL: injection %d did not fire\n", k);
            return 1;
        }
        if (status != CDC_STORE_ECRASH) {
            fprintf(stderr, "store-crash FAIL: injection %d -> %s\n", k,
                    cdc_store_status_name(status));
            return 1;
        }
        /* recovery: reopen and require exactly old or new state */
        if (cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK) {
            fprintf(stderr, "store-crash FAIL: reopen (k=%d)\n", k);
            return 1;
        }
        sealed = cdc_store_sealed_count(store);
        if (cdc_store_replay(store, replayed, sizeof(replayed)) !=
                CDC_STORE_OK ||
            cdc_store_verify(store) != CDC_STORE_OK) {
            fprintf(stderr, "store-crash FAIL: replay/verify (k=%d)\n", k);
            cdc_store_close(store);
            return 1;
        }
        cdc_store_close(store);
        if (sealed == 1 && strcmp(replayed, ref_old) == 0) {
            old_state++;
        } else if (sealed == 2 && strcmp(replayed, ref_new) == 0) {
            new_state++;
        } else {
            fprintf(stderr,
                    "store-crash FAIL: partial state at k=%d "
                    "(sealed=%llu)\n",
                    k, (unsigned long long)sealed);
            return 1;
        }
    }
    printf("store-crash ok boundaries=%d old=%d new=%d\n", boundaries,
           old_state, new_state);
    return 0;
}

static int cmd_store_check(const char *base) {
    char dir[512];
    char a[80], b[80], attest[80];
    cdc_store *store = NULL;

    if (!digest_self_test()) {
        return 1;
    }
    if (!store_reference_digest(base, "det1", 2, a, sizeof(a)) ||
        !store_reference_digest(base, "det2", 2, b, sizeof(b))) {
        fprintf(stderr, "store-check FAIL: determinism stores\n");
        return 1;
    }
    if (strcmp(a, b) != 0) {
        fprintf(stderr, "store-check FAIL: replay digests differ\n");
        return 1;
    }
    snprintf(dir, sizeof(dir), "%s/det1", base);
    if (cdc_store_open(dir, &store, NULL) != CDC_STORE_OK) {
        return 1;
    }
    if (cdc_store_sealed_count(store) != 2 ||
        cdc_store_attest(store, attest, sizeof(attest)) != CDC_STORE_OK ||
        cdc_store_verify(store) != CDC_STORE_OK) {
        fprintf(stderr, "store-check FAIL: attest/verify\n");
        cdc_store_close(store);
        return 1;
    }
    /* Rollback leaves nothing staged, so the following commit has no work
     * and is a typed state error rather than an empty transaction. Fencing
     * at the wrong seal is likewise a typed refusal. (snapshot/compact/
     * fence behaviour proper is owned by the store-protocol suite.) */
    cdc_store_stage(store, "ghost", 5);
    cdc_store_rollback(store);
    if (cdc_store_commit(store) != CDC_STORE_ESTATE ||
        cdc_store_fence(store, 99) != CDC_STORE_ESTATE) {
        fprintf(stderr, "store-check FAIL: typed statuses\n");
        cdc_store_close(store);
        return 1;
    }
    cdc_store_close(store);
    printf("store-check ok determinism=1 attest=%s\n", attest);
    return 0;
}

static int read_file_bytes(const char *path, uint8_t **out, size_t *size);
static int write_file_bytes(const char *path, const uint8_t *bytes,
                            size_t size);

static long file_size(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 ? (long)st.st_size : -1;
}

/* ---- snapshot / compact / fence (Phase D protocol completion) -------- */

static int cmd_store_protocol(const char *base) {
    char dir[512], log_path[600], snap_path[620];
    cdc_store *store = NULL, *other = NULL;
    char before[80], after[80], attest_before[80], attest_after[80];
    uint8_t *snap_bytes = NULL;
    size_t snap_size = 0;
    long log_size_before, log_size_after;
    int failures = 0;
    int t;

    snprintf(dir, sizeof(dir), "%s/protocol", base);
    snprintf(log_path, sizeof(log_path), "%s/log.cdcstore", dir);
    snprintf(snap_path, sizeof(snap_path), "%s/snapshot.cdcstore", dir);
    if (cdc_store_open(dir, &store, NULL) != CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: open\n");
        return 1;
    }
    for (t = 1; t <= 3; t++) {
        if (!store_commit_txn(store, t)) {
            fprintf(stderr, "store-protocol FAIL: seed txn %d\n", t);
            return 1;
        }
    }
    if (cdc_store_replay(store, before, sizeof(before)) != CDC_STORE_OK ||
        cdc_store_attest(store, attest_before, sizeof(attest_before)) !=
            CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: baseline digests\n");
        return 1;
    }
    log_size_before = file_size(log_path);

    /* compact without a snapshot must refuse rather than discard history */
    if (cdc_store_compact(store) != CDC_STORE_ESTATE) {
        fprintf(stderr, "store-protocol FAIL: compact without snapshot\n");
        failures++;
    }
    if (cdc_store_snapshot(store) != CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: snapshot\n");
        return 1;
    }
    if (cdc_store_compact(store) != CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: compact\n");
        return 1;
    }
    log_size_after = file_size(log_path);
    if (cdc_store_replay(store, after, sizeof(after)) != CDC_STORE_OK ||
        cdc_store_attest(store, attest_after, sizeof(attest_after)) !=
            CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: post-compaction digests\n");
        return 1;
    }
    /* THE claim: semantic identity survives the physical rewrite. */
    if (strcmp(before, after) != 0) {
        fprintf(stderr, "store-protocol FAIL: replay identity changed\n  %s\n  %s\n",
                before, after);
        failures++;
    }
    if (strcmp(attest_before, attest_after) == 0) {
        fprintf(stderr, "store-protocol FAIL: attest digest should change\n");
        failures++;
    }
    if (log_size_after >= log_size_before || log_size_after != 0) {
        fprintf(stderr, "store-protocol FAIL: log not compacted (%ld -> %ld)\n",
                log_size_before, log_size_after);
        failures++;
    }
    if (cdc_store_sealed_count(store) != 3) {
        fprintf(stderr, "store-protocol FAIL: sealed count lost by compaction\n");
        failures++;
    }
    cdc_store_close(store);
    store = NULL;

    /* reopening must resume from the base and keep appending coherently */
    if (cdc_store_open(dir, &store, NULL) != CDC_STORE_OK ||
        cdc_store_sealed_count(store) != 3) {
        fprintf(stderr, "store-protocol FAIL: reopen after compaction\n");
        return 1;
    }
    {
        char resumed[80];
        if (cdc_store_replay(store, resumed, sizeof(resumed)) != CDC_STORE_OK ||
            strcmp(resumed, before) != 0) {
            fprintf(stderr, "store-protocol FAIL: replay identity after reopen\n");
            failures++;
        }
    }
    if (!store_commit_txn(store, 4) || cdc_store_sealed_count(store) != 4) {
        fprintf(stderr, "store-protocol FAIL: append after compaction\n");
        failures++;
    }

    /* fence: an armed writer whose view is stale must not commit */
    if (cdc_store_fence(store, 4) != CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: fence at current seal\n");
        failures++;
    }
    if (cdc_store_fence(store, 2) != CDC_STORE_ESTATE) {
        fprintf(stderr, "store-protocol FAIL: stale fence accepted\n");
        failures++;
    }
    if (cdc_store_open(dir, &other, NULL) != CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: second handle\n");
        return 1;
    }
    /* both writers believe the log ends at seal 4 */
    if (cdc_store_fence(store, 4) != CDC_STORE_OK ||
        cdc_store_fence(other, 4) != CDC_STORE_OK) {
        fprintf(stderr, "store-protocol FAIL: concurrent fences\n");
        failures++;
    }
    if (!store_commit_txn(store, 5)) {
        fprintf(stderr, "store-protocol FAIL: winner commit\n");
        failures++;
    }
    {
        /* the loser's fence is now stale: its commit must be refused and
         * must leave the log byte-identical */
        long size_before_loser = file_size(log_path);
        char payload[32];
        cdc_store_status status;
        snprintf(payload, sizeof(payload), "stale-writer");
        cdc_store_stage(other, payload, strlen(payload));
        status = cdc_store_commit(other);
        if (status != CDC_STORE_ESTATE) {
            fprintf(stderr, "store-protocol FAIL: stale writer committed (%s)\n",
                    cdc_store_status_name(status));
            failures++;
        }
        if (file_size(log_path) != size_before_loser) {
            fprintf(stderr, "store-protocol FAIL: stale commit wrote bytes\n");
            failures++;
        }
    }
    cdc_store_close(other);
    cdc_store_close(store);
    store = NULL;

    /* a tampered snapshot must fail closed, never seed a wrong base */
    if (!read_file_bytes(snap_path, &snap_bytes, &snap_size)) {
        fprintf(stderr, "store-protocol FAIL: read snapshot\n");
        return 1;
    }
    {
        size_t offset;
        int rejected = 0, checked = 0;
        for (offset = 0; offset < snap_size; offset++) {
            uint8_t original = snap_bytes[offset];
            cdc_store *probe = NULL;
            snap_bytes[offset] ^= 0xff;
            write_file_bytes(snap_path, snap_bytes, snap_size);
            if (cdc_store_open(dir, &probe, NULL) == CDC_STORE_ECORRUPT &&
                probe == NULL) {
                rejected++;
            } else {
                fprintf(stderr,
                        "store-protocol FAIL: snapshot byte %zu accepted\n",
                        offset);
                cdc_store_close(probe);
                failures++;
            }
            checked++;
            snap_bytes[offset] = original;
        }
        write_file_bytes(snap_path, snap_bytes, snap_size);
        printf("store-protocol snapshot sweep: %d/%d bytes fail closed\n",
               rejected, checked);
    }
    free(snap_bytes);
    if (failures) {
        return 1;
    }
    printf("store-protocol ok snapshot=1 compact=1 fence=1 "
           "replay-identity-preserved=1\n");
    return 0;
}

/* ---- store corruption counterexamples (review B1/B2) ---------------- */

static int read_file_bytes(const char *path, uint8_t **out, size_t *size) {
    FILE *fp = fopen(path, "rb");
    long end;
    if (!fp) {
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    end = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    *out = malloc((size_t)end ? (size_t)end : 1);
    if (!*out || fread(*out, 1, (size_t)end, fp) != (size_t)end) {
        free(*out);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    *size = (size_t)end;
    return 1;
}

static int write_file_bytes(const char *path, const uint8_t *bytes,
                            size_t size) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return 0;
    }
    if (fwrite(bytes, 1, size, fp) != size) {
        fclose(fp);
        return 0;
    }
    return fclose(fp) == 0;
}

enum {
    STORE_FRAMING_SIZE = 4 + 1 + 8 + 4,
    STORE_HEADER_SIZE = STORE_FRAMING_SIZE + 2 * CDC_DIGEST_SIZE
};

/* One corruption case: mutate one byte, expect open to fail closed with
 * ECORRUPT, no handle, recovered_out untouched (0), and the log bytes
 * byte-identical (3122af5 re-review contract). */
static int corrupt_case(const char *dir, const char *log_path,
                        const char *name, const uint8_t *orig, size_t size,
                        size_t offset, uint8_t new_value, int verbose) {
    uint8_t *mutated;
    uint8_t *after = NULL;
    size_t after_size = 0;
    cdc_store *store = NULL;
    int recovered = -1;
    cdc_store_status status;
    int ok = 1;

    mutated = malloc(size);
    if (!mutated) {
        return 0;
    }
    memcpy(mutated, orig, size);
    mutated[offset] = new_value;
    if (mutated[offset] == orig[offset]) {
        mutated[offset] ^= 0xff;
    }
    if (!write_file_bytes(log_path, mutated, size)) {
        free(mutated);
        return 0;
    }
    status = cdc_store_open(dir, &store, &recovered);
    if (status != CDC_STORE_ECORRUPT) {
        fprintf(stderr, "store-corrupt FAIL %s offset=%zu: open -> %s\n",
                name, offset, cdc_store_status_name(status));
        ok = 0;
    }
    if (store != NULL) {
        fprintf(stderr, "store-corrupt FAIL %s offset=%zu: handle\n", name,
                offset);
        cdc_store_close(store);
        ok = 0;
    }
    if (recovered != 0) {
        fprintf(stderr,
                "store-corrupt FAIL %s offset=%zu: recovered=%d\n", name,
                offset, recovered);
        ok = 0;
    }
    if (!read_file_bytes(log_path, &after, &after_size) ||
        after_size != size || memcmp(after, mutated, size) != 0) {
        fprintf(stderr, "store-corrupt FAIL %s offset=%zu: log mutated\n",
                name, offset);
        ok = 0;
    }
    free(after);
    free(mutated);
    if (ok && verbose) {
        printf("store-corrupt ok case=%s offset=%zu\n", name, offset);
    }
    return ok;
}

static int cmd_store_corrupt(const char *base) {
    char src_dir[512], src_log[600];
    char sweep_dir[512], sweep_log[600];
    uint8_t *orig = NULL;
    size_t size = 0;
    size_t swept = 0;
    int failures = 0;
    cdc_store *store = NULL;

    /* reference store: 2 sealed transactions, 3 events each */
    if (!store_reference_digest(base, "corrupt_src", 2, src_log,
                                sizeof(src_log))) {
        fprintf(stderr, "store-corrupt FAIL: reference store\n");
        return 1;
    }
    snprintf(src_dir, sizeof(src_dir), "%s/corrupt_src", base);
    snprintf(src_log, sizeof(src_log), "%s/log.cdcstore", src_dir);
    if (!read_file_bytes(src_log, &orig, &size)) {
        fprintf(stderr, "store-corrupt FAIL: read reference log\n");
        return 1;
    }
    snprintf(sweep_dir, sizeof(sweep_dir), "%s/sweep", base);
    snprintf(sweep_log, sizeof(sweep_log), "%s/log.cdcstore", sweep_dir);
    if (mkdir(sweep_dir, 0777) != 0 && errno != EEXIST) {
        free(orig);
        return 1;
    }

    /* Full mutation matrix (re-review item 6): EVERY byte of the sealed
     * log — all magic, type, sequence, length, framing-tag, payload-digest,
     * payload, and seal bytes — flipped one at a time; each must fail
     * closed with the evidence untouched. */
    {
        size_t offset;
        for (offset = 0; offset < size; offset++) {
            if (!corrupt_case(sweep_dir, sweep_log, "sweep", orig, size,
                              offset, (uint8_t)(orig[offset] ^ 0xff), 0)) {
                failures++;
            }
            swept++;
        }
    }
    /* Re-review item 7: the exact high-byte length mutation, named. The
     * first DATA record's length field sits at offset 13..16; flipping
     * offset 16 declares a multi-megabyte payload in a small file. */
    failures += !corrupt_case(sweep_dir, sweep_log,
                              "high-byte-length", orig, size, 16,
                              (uint8_t)(orig[16] + 1), 1);

    /* control 1: unmutated copy opens clean with 2 seals */
    {
        char dir[512], log_path[600];
        int recovered = -1;
        snprintf(dir, sizeof(dir), "%s/control_clean", base);
        snprintf(log_path, sizeof(log_path), "%s/log.cdcstore", dir);
        mkdir(dir, 0777);
        write_file_bytes(log_path, orig, size);
        if (cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
            recovered != 0 || cdc_store_sealed_count(store) != 2) {
            fprintf(stderr, "store-corrupt FAIL: clean control\n");
            failures++;
        }
        cdc_store_close(store);
        store = NULL;
    }
    /* control 2: a fully valid but unsealed DATA tail (correct framing
     * tag, digest, and sequence) is recoverable (latch-or-hold),
     * distinguished from corruption */
    {
        char dir[512], log_path[600];
        uint8_t record[STORE_HEADER_SIZE + 5];
        uint8_t digest[CDC_DIGEST_SIZE];
        int recovered = -1;
        FILE *fp;
        snprintf(dir, sizeof(dir), "%s/control_tail", base);
        snprintf(log_path, sizeof(log_path), "%s/log.cdcstore", dir);
        mkdir(dir, 0777);
        write_file_bytes(log_path, orig, size);
        memcpy(record, "CDC2", 4);
        record[4] = 'D';
        record[5] = 7; /* event seq 7 (6 sealed events precede) */
        memset(record + 6, 0, 7);
        record[13] = 5; /* payload length 5, little-endian */
        memset(record + 14, 0, 3);
        cdc_digest(record, STORE_FRAMING_SIZE, record + STORE_FRAMING_SIZE);
        cdc_digest("extra", 5, digest);
        memcpy(record + STORE_FRAMING_SIZE + CDC_DIGEST_SIZE, digest,
               CDC_DIGEST_SIZE);
        memcpy(record + STORE_HEADER_SIZE, "extra", 5);
        fp = fopen(log_path, "ab");
        fwrite(record, 1, sizeof(record), fp);
        fclose(fp);
        if (cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
            recovered != 1 || cdc_store_sealed_count(store) != 2) {
            fprintf(stderr, "store-corrupt FAIL: unsealed-tail control\n");
            failures++;
        }
        cdc_store_close(store);
        store = NULL;
    }
    free(orig);
    if (failures) {
        return 1;
    }
    printf("store-corrupt ok swept=%zu named=1 controls=2\n", swept);
    return 0;
}

/* ---- store I/O-fault regressions (f1f68c0 re-review) ---------------- */

static int store_io_case(const char *name, const char *dir,
                         const uint8_t *expect_bytes, size_t expect_size) {
    cdc_store *store = NULL;
    int recovered = -1;
    cdc_store_status status = cdc_store_open(dir, &store, &recovered);
    int ok = 1;
    if (status != CDC_STORE_EIO) {
        fprintf(stderr, "store-io FAIL %s: open -> %s\n", name,
                cdc_store_status_name(status));
        ok = 0;
    }
    if (store != NULL) {
        fprintf(stderr, "store-io FAIL %s: handle returned\n", name);
        cdc_store_close(store);
        ok = 0;
    }
    if (expect_bytes != NULL) {
        char log_path[600];
        uint8_t *after = NULL;
        size_t after_size = 0;
        snprintf(log_path, sizeof(log_path), "%s/log.cdcstore", dir);
        if (!read_file_bytes(log_path, &after, &after_size) ||
            after_size != expect_size ||
            memcmp(after, expect_bytes, expect_size) != 0) {
            fprintf(stderr, "store-io FAIL %s: log mutated (size %zu)\n",
                    name, after_size);
            ok = 0;
        }
        free(after);
    }
    if (ok) {
        printf("store-io ok case=%s\n", name);
    }
    return ok;
}

static int cmd_store_io(const char *base) {
    char dir[512], log_path[600], digest_out[80];
    uint8_t *orig = NULL;
    size_t size = 0;
    int failures = 0;

    /* regression 1: log.cdcstore is a directory -> EIO, no handle */
    snprintf(dir, sizeof(dir), "%s/iodir", base);
    snprintf(log_path, sizeof(log_path), "%s/log.cdcstore", dir);
    mkdir(dir, 0777);
    mkdir(log_path, 0777);
    failures += !store_io_case("dir-as-log", dir, NULL, 0);

    /* reference store with 2 sealed transactions for the injection arms */
    if (!store_reference_digest(base, "io_src", 2, digest_out,
                                sizeof(digest_out))) {
        fprintf(stderr, "store-io FAIL: reference store\n");
        return 1;
    }
    snprintf(dir, sizeof(dir), "%s/io_src", base);
    snprintf(log_path, sizeof(log_path), "%s/log.cdcstore", dir);
    if (!read_file_bytes(log_path, &orig, &size)) {
        fprintf(stderr, "store-io FAIL: read reference log\n");
        return 1;
    }

    /* regression 2: injected mid-read fault before any seal (first scan
     * read) -> EIO, bytes unchanged */
    cdc_store_set_read_fail_after(1);
    failures += !store_io_case("fault-before-seal", dir, orig, size);
    cdc_store_set_read_fail_after(0);

    /* regression 3: injected mid-read fault after the first seal (each
     * txn = 3 DATA x 2 reads + 1 SEAL read = 7 reads; fault at read 8)
     * -> EIO, bytes unchanged, NO truncation of the sealed prefix */
    cdc_store_set_read_fail_after(8);
    failures += !store_io_case("fault-after-seal", dir, orig, size);
    cdc_store_set_read_fail_after(0);

    /* control: disarmed, the same store opens clean with 2 seals */
    {
        cdc_store *store = NULL;
        int recovered = -1;
        if (cdc_store_open(dir, &store, &recovered) != CDC_STORE_OK ||
            recovered != 0 || cdc_store_sealed_count(store) != 2) {
            fprintf(stderr, "store-io FAIL: disarmed control\n");
            failures++;
        }
        cdc_store_close(store);
    }
    free(orig);
    if (failures) {
        return 1;
    }
    printf("store-io ok cases=3 controls=1\n");
    return 0;
}

/* ---- reject --------------------------------------------------------- */

static int cmd_reject(int argc, char **argv) {
    int i;
    for (i = 0; i < argc; i++) {
        cdc_unit program;
        cdc_diag_list diags;
        cdc_diag_list_init(&diags);
        cdc_unit_parse_file(argv[i], &program, &diags);
        if (diags.errors == 0) {
            fprintf(stderr, "reject FAIL %s: accepted\n", argv[i]);
            return 1;
        }
        printf("frontend reject ok %s code=%s\n", base_name(argv[i]),
               diags.items ? diags.items[0].code : "CDC900");
        cdc_unit_free(&program);
        cdc_diag_list_free(&diags);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
                "usage: cdc_frontend_check "
                "dump|canon|roundtrip|attr-parity|bounds|oom|reject ...\n");
        return 2;
    }
    if (strcmp(argv[1], "dump") == 0) {
        return cmd_dump(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "canon") == 0) {
        return cmd_canon(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "roundtrip") == 0) {
        return cmd_roundtrip(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "attr-parity") == 0) {
        return cmd_attr_parity(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "bounds") == 0) {
        return cmd_bounds();
    }
    if (strcmp(argv[1], "oom") == 0 && argc >= 3) {
        return cmd_oom(argv[2]);
    }
    if (strcmp(argv[1], "reject") == 0) {
        return cmd_reject(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "abi-diag") == 0 && argc >= 3) {
        return cmd_abi_diag(argv[2]);
    }
    if (strcmp(argv[1], "abi-io") == 0 && argc >= 4) {
        return cmd_abi_io(argv[2], argv[3]);
    }
    if (strcmp(argv[1], "io-mid-read") == 0 && argc >= 3) {
        return cmd_io_mid_read(argv[2]);
    }
    if (strcmp(argv[1], "oom-abi") == 0 && argc >= 3) {
        return cmd_oom_abi(argv[2]);
    }
    if (strcmp(argv[1], "store-crash") == 0 && argc >= 3) {
        return cmd_store_crash(argv[2]);
    }
    if (strcmp(argv[1], "store-check") == 0 && argc >= 3) {
        return cmd_store_check(argv[2]);
    }
    if (strcmp(argv[1], "store-corrupt") == 0 && argc >= 3) {
        return cmd_store_corrupt(argv[2]);
    }
    if (strcmp(argv[1], "store-protocol") == 0 && argc >= 3) {
        return cmd_store_protocol(argv[2]);
    }
    if (strcmp(argv[1], "store-io") == 0 && argc >= 3) {
        return cmd_store_io(argv[2]);
    }
    if (strcmp(argv[1], "digest-vectors") == 0 && argc >= 3) {
        return cmd_digest_vectors(argv[2]);
    }
    if (strcmp(argv[1], "digest-file") == 0 && argc >= 3) {
        return cmd_digest_file(argc - 2, argv + 2);
    }
    fprintf(stderr, "cdc_frontend_check: unknown mode '%s'\n", argv[1]);
    return 2;
}

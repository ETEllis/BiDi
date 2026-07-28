/* `cdc build` — Phase I proof-carrying bundle (gate CT5 seed).
 *
 * A bundle is the canonical serialization of an explicit source set, and a
 * manifest that binds everything a consumer needs to re-check it: the
 * grammar and ABI versions, the corpus identity (D21), every source's
 * content digest, the artifact digest, and the contract verdict the
 * corpus carried when it was bundled.
 *
 * Two refusals are the point of the command:
 *
 *  - A RED corpus is never bundled. An artifact is a claim that this
 *    source set stood behind its expectations; emitting one from a
 *    failing tree would manufacture that claim.
 *  - A bundle that does not re-verify is never emitted. The bundle is
 *    reparsed and its contract report compared against the sources'
 *    report field-for-field (modulo the [file:line] provenance suffix,
 *    which legitimately differs — the checks, verdicts, and evaluated
 *    labels must not).
 *
 * Outputs are written temp-then-rename, so a refused or crashed build
 *  never leaves a partial artifact wearing the real name. Both outputs are
 * deterministic — no timestamps, no absolute paths — which is what lets
 * the gate build twice and byte-compare (D21's reproducibility, extended
 * to artifacts).
 *
 * `cdc build --check` re-derives every digest and reports typed
 * mismatches: `source-drift` (a source no longer matches its manifest
 * line), `artifact-mismatch` (the bundle and the manifest disagree —
 * which side moved cannot be attributed from inside, and is not
 * pretended), and `manifest-malformed`. */
#define _POSIX_C_SOURCE 200809L

#include "cmd_build.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cdc_abi.h"
#include "../cdc_digest.h"
#include "cdc_manifest.h"

#define BUNDLE_NAME "cdc-bundle.cdc"
#define MANIFEST_NAME "cdc-bundle.manifest"

static const char *build_dir(void) {
    const char *dir = getenv("CDC_BUILD_DIR");
    return dir && dir[0] ? dir : "build";
}

static const char *base_name(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

/* Writes `text` to dir/name via temp-then-rename; 0 on failure. */
static int write_atomic(const char *dir, const char *name, const char *text,
                        size_t length) {
    char temp[1024];
    char final_path[1024];
    FILE *fp;
    if ((size_t)snprintf(temp, sizeof(temp), "%s/%s.tmp", dir, name) >=
            sizeof(temp) ||
        (size_t)snprintf(final_path, sizeof(final_path), "%s/%s", dir,
                         name) >= sizeof(final_path)) {
        return 0;
    }
    fp = fopen(temp, "wb");
    if (!fp) {
        return 0;
    }
    if (fwrite(text, 1, length, fp) != length || fflush(fp) != 0) {
        fclose(fp);
        remove(temp);
        return 0;
    }
    fclose(fp);
    if (rename(temp, final_path) != 0) {
        remove(temp);
        return 0;
    }
    return 1;
}

/* Contract-report equality modulo the [file:line] provenance suffix on
 * check lines. Everything else — banners, evaluated labels, verdicts,
 * order, and the summary counts — must be byte-identical. */
static int reports_equal_modulo_source(const char *a, const char *b) {
    while (*a || *b) {
        const char *ea = strchr(a, '\n');
        const char *eb = strchr(b, '\n');
        size_t la = ea ? (size_t)(ea - a) : strlen(a);
        size_t lb = eb ? (size_t)(eb - b) : strlen(b);
        size_t ca = la, cb = lb;
        if ((la >= 5 && strncmp(a, "  OK ", 5) == 0) ||
            (la >= 7 && strncmp(a, "  FAIL ", 7) == 0)) {
            const char *cut = NULL;
            const char *scan = a;
            for (;;) {
                const char *hit = memchr(scan, '[', (size_t)(a + la - scan));
                if (!hit) {
                    break;
                }
                cut = hit;
                scan = hit + 1;
            }
            if (cut && cut > a + 3 && strncmp(cut - 3, "   [", 4) == 0) {
                ca = (size_t)(cut - 3 - a);
            }
        }
        if ((lb >= 5 && strncmp(b, "  OK ", 5) == 0) ||
            (lb >= 7 && strncmp(b, "  FAIL ", 7) == 0)) {
            const char *cut = NULL;
            const char *scan = b;
            for (;;) {
                const char *hit = memchr(scan, '[', (size_t)(b + lb - scan));
                if (!hit) {
                    break;
                }
                cut = hit;
                scan = hit + 1;
            }
            if (cut && cut > b + 3 && strncmp(cut - 3, "   [", 4) == 0) {
                cb = (size_t)(cut - 3 - b);
            }
        }
        if (ca != cb || memcmp(a, b, ca) != 0) {
            return 0;
        }
        a += la + (ea ? 1 : 0);
        b += lb + (eb ? 1 : 0);
    }
    return 1;
}

static long count_lines(const char *text) {
    long n = 0;
    for (; *text; text++) {
        if (*text == '\n') {
            n++;
        }
    }
    return n;
}

/* Reads a whole file (bounded); 0 on failure. Caller frees. */
static int read_all(const char *path, char **out, size_t *out_length) {
    FILE *fp = fopen(path, "rb");
    char *buffer = NULL;
    size_t size = 0, cap = 0;
    if (!fp) {
        return 0;
    }
    for (;;) {
        size_t got;
        if (size == cap) {
            size_t next = cap ? cap * 2 : 1 << 16;
            char *grown;
            if (next > (64u << 20)) {
                free(buffer);
                fclose(fp);
                return 0;
            }
            grown = realloc(buffer, next);
            if (!grown) {
                free(buffer);
                fclose(fp);
                return 0;
            }
            buffer = grown;
            cap = next;
        }
        got = fread(buffer + size, 1, cap - size, fp);
        size += got;
        if (got == 0) {
            if (ferror(fp)) {
                free(buffer);
                fclose(fp);
                return 0;
            }
            break;
        }
    }
    fclose(fp);
    /* NUL-terminate: consumers walk this with string functions, and a
     * buffer that ends exactly at the last byte would send them into the
     * heap. The terminator is not part of the content length. */
    {
        char *terminated = realloc(buffer, size + 1);
        if (!terminated) {
            free(buffer);
            return 0;
        }
        terminated[size] = '\0';
        buffer = terminated;
    }
    *out = buffer;
    *out_length = size;
    return 1;
}

/* --check: strict-parse the manifest, then re-derive EVERYTHING it binds
 * — per-source digests, corpus, artifact, statement and check counts —
 * and compare. A header used to be "format only"; now every field is a
 * claim this function verifies (second 2026-07-28 review, finding 3). */
static int build_check(int file_count, char **files) {
    char manifest_path[1024];
    char bundle_path[1024];
    char *manifest = NULL;
    size_t manifest_length = 0;
    static cdc_manifest parsed;
    char parse_error[128];
    int mismatches = 0;
    uint8_t digest[CDC_DIGEST_SIZE];
    char hex[96];
    long statements = 0;
    long checks = 0;
    int i;

    snprintf(manifest_path, sizeof(manifest_path), "%s/%s", build_dir(),
             MANIFEST_NAME);
    snprintf(bundle_path, sizeof(bundle_path), "%s/%s", build_dir(),
             BUNDLE_NAME);
    if (!read_all(manifest_path, &manifest, &manifest_length)) {
        fprintf(stderr, "cdc build --check: manifest-missing (%s)\n",
                manifest_path);
        return 1;
    }
    if (!cdc_manifest_parse(manifest, manifest_length, CDC_MANIFEST_BUNDLE,
                            cdc_abi_version(), NULL, &parsed, parse_error,
                            sizeof(parse_error))) {
        fprintf(stderr, "cdc build --check: manifest-malformed (%s)\n",
                parse_error);
        free(manifest);
        return 1;
    }
    free(manifest);
    if (parsed.files != file_count) {
        fprintf(stderr,
                "cdc build --check: manifest-malformed (files=%ld but %d "
                "sources were given)\n",
                parsed.files, file_count);
        return 1;
    }

    /* ---- per-source digests: record set == given set, byte for byte -- */
    for (i = 0; i < parsed.record_count; i++) {
        int j;
        for (j = 0; j < file_count; j++) {
            if (strcmp(base_name(files[j]), parsed.record_name[i]) == 0) {
                break;
            }
        }
        if (j == file_count) {
            fprintf(stderr,
                    "cdc build --check: source-drift (%s is in the "
                    "manifest but not in the given set)\n",
                    parsed.record_name[i]);
            mismatches++;
            continue;
        }
        if (!cdc_digest_file(files[j], digest)) {
            fprintf(stderr,
                    "cdc build --check: source-drift (%s unreadable)\n",
                    parsed.record_name[i]);
            mismatches++;
            continue;
        }
        cdc_digest_hex(digest, hex, sizeof(hex));
        if (strcmp(hex, parsed.record_digest[i]) != 0) {
            fprintf(stderr, "cdc build --check: source-drift (%s)\n",
                    parsed.record_name[i]);
            mismatches++;
        }
    }

    /* ---- corpus ------------------------------------------------------ */
    if (!cdc_digest_corpus((const char *const *)files, (size_t)file_count,
                           digest)) {
        fprintf(stderr,
                "cdc build --check: source-drift (corpus unreadable)\n");
        mismatches++;
    } else {
        cdc_digest_hex(digest, hex, sizeof(hex));
        if (strcmp(hex, parsed.corpus) != 0) {
            fprintf(stderr, "cdc build --check: source-drift (corpus)\n");
            mismatches++;
        }
    }

    /* ---- artifact ---------------------------------------------------- */
    if (!cdc_digest_file(bundle_path, digest)) {
        fprintf(stderr, "cdc build --check: artifact-mismatch (bundle "
                        "unreadable)\n");
        mismatches++;
    } else {
        cdc_digest_hex(digest, hex, sizeof(hex));
        if (strcmp(hex, parsed.artifact) != 0) {
            /* which side moved cannot be attributed from inside, and is
             * not pretended */
            fprintf(stderr, "cdc build --check: artifact-mismatch (bundle "
                            "and manifest disagree)\n");
            mismatches++;
        }
    }

    /* ---- statement and check counts: re-derived, never trusted ------- */
    if (mismatches == 0) {
        cdc_runtime *runtime = NULL;
        cdc_result *vectors = NULL;
        if (cdc_runtime_create(&runtime) != CDC_OK) {
            fprintf(stderr, "cdc build --check: runtime creation failed\n");
            return 1;
        }
        for (i = 0; i < file_count; i++) {
            cdc_program *program = NULL;
            cdc_status status =
                cdc_program_parse(files[i], NULL, 0, &program);
            if (status != CDC_OK || cdc_program_error_count(program) > 0) {
                fprintf(stderr,
                        "cdc build --check: source-drift (%s no longer "
                        "parses)\n",
                        base_name(files[i]));
                cdc_program_destroy(program);
                cdc_runtime_destroy(runtime);
                return 1;
            }
            statements += (long)cdc_program_statement_count(program);
            if (cdc_runtime_load(runtime, program) != CDC_OK) {
                fprintf(stderr, "cdc build --check: load failed (%s)\n",
                        base_name(files[i]));
                cdc_program_destroy(program);
                cdc_runtime_destroy(runtime);
                return 1;
            }
        }
        if (cdc_runtime_vectors(runtime, NULL, &vectors) != CDC_OK) {
            fprintf(stderr, "cdc build --check: vector export failed\n");
            cdc_runtime_destroy(runtime);
            return 1;
        }
        checks = count_lines(cdc_result_text(vectors));
        cdc_result_destroy(vectors);
        cdc_runtime_destroy(runtime);
        if (statements != parsed.statements) {
            fprintf(stderr,
                    "cdc build --check: manifest-malformed (statements=%ld "
                    "but the sources carry %ld)\n",
                    parsed.statements, statements);
            return 1;
        }
        if (checks != parsed.checks) {
            fprintf(stderr,
                    "cdc build --check: manifest-malformed (checks=%ld but "
                    "the sources carry %ld)\n",
                    parsed.checks, checks);
            return 1;
        }
    }
    if (mismatches) {
        return 1;
    }
    printf("cdc build check ok files=%d corpus=verified artifact=verified "
           "counts=verified\n",
           file_count);
    return 0;
}

int cdc_cmd_build(int argc, char **argv) {
    int check = 0;
    char **files;
    int file_count = 0;
    cdc_runtime *runtime = NULL;
    cdc_result *report = NULL;
    cdc_result *vectors = NULL;
    char *bundle = NULL;
    size_t bundle_length = 0;
    FILE *bundle_mem;
    char *manifest = NULL;
    size_t manifest_length = 0;
    FILE *manifest_mem;
    long statements = 0;
    long checks = 0;
    uint8_t digest[CDC_DIGEST_SIZE];
    char corpus_hex[96], artifact_hex[96];
    uint32_t abi = cdc_abi_version();
    int i;
    int rc = 1;

    if (argc >= 1 && strcmp(argv[0], "--check") == 0) {
        check = 1;
        argv++;
        argc--;
    }
    if (argc < 1) {
        fprintf(stderr, "usage: cdc build [--check] <files...>\n");
        return 2;
    }
    files = argv;
    file_count = argc;
    for (i = 0; i < file_count; i++) {
        if (files[i][0] == '-') {
            fprintf(stderr, "cdc build: unknown option %s\n", files[i]);
            return 2;
        }
    }
    if (check) {
        return build_check(file_count, files);
    }

    if (!cdc_digest_corpus((const char *const *)files, (size_t)file_count,
                           digest)) {
        fprintf(stderr, "cdc build: a source is unreadable; a partial "
                        "corpus is never bundled\n");
        return 1;
    }
    cdc_digest_hex(digest, corpus_hex, sizeof(corpus_hex));

    if (cdc_runtime_create(&runtime) != CDC_OK) {
        fprintf(stderr, "cdc build: runtime creation failed\n");
        return 1;
    }
    bundle_mem = open_memstream(&bundle, &bundle_length);
    manifest_mem = open_memstream(&manifest, &manifest_length);
    if (!bundle_mem || !manifest_mem) {
        fprintf(stderr, "cdc build: allocation failed\n");
        goto done;
    }

    for (i = 0; i < file_count; i++) {
        cdc_program *program = NULL;
        char *canonical = NULL;
        size_t canonical_length = 0;
        uint8_t file_digest[CDC_DIGEST_SIZE];
        char file_hex[96];
        cdc_status status = cdc_program_parse(files[i], NULL, 0, &program);
        if (status != CDC_OK || cdc_program_error_count(program) > 0) {
            fprintf(stderr, "cdc build: %s: %s (%zu diagnostics)\n",
                    files[i], cdc_status_name(status),
                    program ? cdc_program_error_count(program) : 0);
            cdc_program_destroy(program);
            goto done;
        }
        statements += (long)cdc_program_statement_count(program);
        if (cdc_program_canonical_bytes(program, &canonical,
                                        &canonical_length) != CDC_OK) {
            fprintf(stderr, "cdc build: %s: canonicalization failed\n",
                    files[i]);
            cdc_program_destroy(program);
            goto done;
        }
        fwrite(canonical, 1, canonical_length, bundle_mem);
        cdc_bytes_free(canonical);
        if (!cdc_digest_file(files[i], file_digest)) {
            fprintf(stderr, "cdc build: %s: unreadable\n", files[i]);
            cdc_program_destroy(program);
            goto done;
        }
        cdc_digest_hex(file_digest, file_hex, sizeof(file_hex));
        fprintf(manifest_mem, "source %s %s\n", base_name(files[i]),
                file_hex);
        /* the runtime takes ownership of the program */
        if (cdc_runtime_load(runtime, program) != CDC_OK) {
            fprintf(stderr, "cdc build: %s: load failed\n", files[i]);
            cdc_program_destroy(program);
            goto done;
        }
    }

    if (cdc_runtime_verify(runtime, NULL, &report) != CDC_OK) {
        fprintf(stderr, "cdc build: contract evaluation failed\n");
        goto done;
    }
    if (cdc_result_error_count(report) > 0) {
        fprintf(stderr,
                "cdc build: refused — the corpus has %zu failing "
                "expectation(s); an artifact is a claim the sources stood "
                "behind their contract, and a red tree cannot make it\n",
                cdc_result_error_count(report));
        goto done;
    }
    if (cdc_runtime_vectors(runtime, NULL, &vectors) != CDC_OK) {
        fprintf(stderr, "cdc build: vector export failed\n");
        goto done;
    }
    checks = count_lines(cdc_result_text(vectors));

    fclose(bundle_mem);
    bundle_mem = NULL;

    /* Parity: the bundle must re-verify with the same checks, verdicts,
     * and labels as the sources it claims to carry. */
    {
        cdc_program *reparsed = NULL;
        cdc_runtime *bundle_runtime = NULL;
        cdc_result *bundle_report = NULL;
        cdc_status status =
            cdc_program_parse(NULL, bundle, bundle_length, &reparsed);
        if (status != CDC_OK || cdc_program_error_count(reparsed) > 0) {
            fprintf(stderr, "cdc build: bundle does not reparse (%s)\n",
                    cdc_status_name(status));
            cdc_program_destroy(reparsed);
            goto done;
        }
        if (cdc_runtime_create(&bundle_runtime) != CDC_OK ||
            cdc_runtime_verify(bundle_runtime, reparsed, &bundle_report) !=
                CDC_OK) {
            fprintf(stderr, "cdc build: bundle re-verification failed\n");
            cdc_runtime_destroy(bundle_runtime);
            goto done;
        }
        if (cdc_result_error_count(bundle_report) > 0 ||
            !reports_equal_modulo_source(cdc_result_text(report),
                                         cdc_result_text(bundle_report))) {
            fprintf(stderr,
                    "cdc build: bundle-parity mismatch — the bundle does "
                    "not carry the same contract verdict as its sources\n");
            cdc_result_destroy(bundle_report);
            cdc_runtime_destroy(bundle_runtime);
            goto done;
        }
        cdc_result_destroy(bundle_report);
        cdc_runtime_destroy(bundle_runtime);
    }

    cdc_digest(bundle, bundle_length, digest);
    cdc_digest_hex(digest, artifact_hex, sizeof(artifact_hex));

    /* header first, then the per-source lines already buffered, then the
     * corpus and artifact bindings */
    {
        char *body;
        size_t body_length;
        char header[256];
        FILE *final_mem;
        char *final_text = NULL;
        size_t final_length = 0;
        /* a memstream's buffer/length are only final after fclose */
        fclose(manifest_mem);
        manifest_mem = NULL;
        body = manifest;
        body_length = manifest_length;
        manifest = NULL; /* ownership moves to body */
        snprintf(header, sizeof(header),
                 "cdc-bundle v=1 grammar=1 abi=%u.%u files=%d "
                 "statements=%ld checks=%ld\n",
                 abi >> 16, abi & 0xffffu, file_count, statements, checks);
        final_mem = open_memstream(&final_text, &final_length);
        if (!final_mem) {
            free(body);
            goto done;
        }
        fputs(header, final_mem);
        fwrite(body, 1, body_length, final_mem);
        free(body);
        fprintf(final_mem, "corpus %s\n", corpus_hex);
        fprintf(final_mem, "artifact %s\n", artifact_hex);
        fclose(final_mem);
        /* The emitter eats its own cooking: a manifest the strict parser
         * refuses is never written wearing the real name. */
        {
            static cdc_manifest self_check;
            char parse_error[128];
            if (!cdc_manifest_parse(final_text, final_length,
                                    CDC_MANIFEST_BUNDLE, abi, NULL,
                                    &self_check, parse_error,
                                    sizeof(parse_error))) {
                fprintf(stderr,
                        "cdc build: internal error — emitted manifest "
                        "fails its own parser (%s)\n",
                        parse_error);
                free(final_text);
                goto done;
            }
        }
        if (!write_atomic(build_dir(), BUNDLE_NAME, bundle, bundle_length) ||
            !write_atomic(build_dir(), MANIFEST_NAME, final_text,
                          final_length)) {
            fprintf(stderr, "cdc build: could not write artifacts\n");
            free(final_text);
            goto done;
        }
        free(final_text);
    }

    printf("cdc build ok files=%d statements=%ld checks=%ld/%ld "
           "corpus=%s artifact=%s\n",
           file_count, statements, checks, checks, corpus_hex, artifact_hex);
    rc = 0;

done:
    if (bundle_mem) {
        fclose(bundle_mem);
    }
    if (manifest_mem) {
        fclose(manifest_mem);
    }
    free(bundle);
    free(manifest);
    cdc_result_destroy(report);
    cdc_result_destroy(vectors);
    cdc_runtime_destroy(runtime);
    return rc;
}

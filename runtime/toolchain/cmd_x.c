/* `cdc x` — Phase I package runner (gate CT5 seed).
 *
 * Runs one member of an INSTALLED package through the fused single-process
 * executor, after verifying that the package on disk is exactly the
 * package that was installed:
 *
 *  - every manifest member re-digests to its recorded digest (a tampered
 *    or drifted member is refused BY NAME before anything executes);
 *  - the directory contains exactly the manifest's members — an
 *    unmanifested `.cdc` file is refused, because code that was never
 *    validated or journaled must not sit one typo away from execution;
 *  - the corpus identity recomputes to the recorded value;
 *  - the entry is a plain member basename. Path separators are rejected,
 *    which is what makes `cdc x name ../../evil.cdc` a parse error of the
 *    request rather than a property of filesystem layout.
 *
 * TRUSTED-LOCAL-ONLY. `cdc x` executes code the operator installed from a
 * local directory, nothing else: no network, no registries, no archives,
 * no auto-install of missing packages. A hostile package is OUT OF SCOPE
 * at this gate — the sealed capability environment and hostile-package
 * counterexamples are CT5, and until CT5 passes, that boundary is stated
 * here and in the docs rather than papered over. Verification above is
 * drift/tamper DETECTION for trusted content, not a sandbox. */
#define _POSIX_C_SOURCE 200809L

#include "cmd_x.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cdc_digest.h"
#include "cdc_manifest.h"

int cdc_native_main(int argc, char **argv);

#define MAX_MEMBERS 256

static const char *modules_root(void) {
    const char *root = getenv("CDC_MODULES");
    return root && root[0] ? root : "cdc_modules";
}

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
            if (next > (16u << 20)) {
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

static int name_compare(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

int cdc_cmd_x(int argc, char **argv) {
    const char *name;
    const char *entry;
    char package_dir[1024];
    char manifest_path[1100];
    char *manifest = NULL;
    size_t manifest_length = 0;
    char *members[MAX_MEMBERS];
    char recorded[MAX_MEMBERS][96];
    int member_count = 0;
    char recorded_corpus[96] = "";
    int entry_found = 0;
    int i;
    int rc = 1;

    if (argc < 2 || argv[0][0] == '-' || argv[1][0] == '-') {
        fprintf(stderr, "usage: cdc x <package> <entry.cdc>\n");
        return 2;
    }
    name = argv[0];
    entry = argv[1];
    if (strchr(name, '/') || strchr(entry, '/')) {
        fprintf(stderr, "cdc x refused: package and entry are plain names, "
                        "never paths\n");
        return 1;
    }
    if ((size_t)snprintf(package_dir, sizeof(package_dir), "%s/%s",
                         modules_root(), name) >= sizeof(package_dir) ||
        (size_t)snprintf(manifest_path, sizeof(manifest_path),
                         "%s/.manifest", package_dir) >=
            sizeof(manifest_path)) {
        fprintf(stderr, "cdc x: path too long\n");
        return 2;
    }
    if (!read_all(manifest_path, &manifest, &manifest_length)) {
        fprintf(stderr, "cdc x refused: %s is not an installed package "
                        "(no manifest)\n",
                name);
        return 1;
    }

    /* ---- strict manifest parse (the ONE shared parser) ---------------
     * The header is a binding claim, not decoration: version, package
     * name, and member count are all verified, and a manifest with a
     * missing, duplicated, or mutated header never reaches execution
     * (second 2026-07-28 review, finding 3). */
    {
        static cdc_manifest parsed;
        char parse_error[128];
        if (!cdc_manifest_parse(manifest, manifest_length,
                                CDC_MANIFEST_PACKAGE, 0, name, &parsed,
                                parse_error, sizeof(parse_error))) {
            fprintf(stderr, "cdc x refused: manifest-malformed (%s)\n",
                    parse_error);
            goto done;
        }
        for (i = 0; i < parsed.record_count; i++) {
            members[member_count] = strdup(parsed.record_name[i]);
            if (!members[member_count]) {
                goto done;
            }
            snprintf(recorded[member_count], sizeof(recorded[member_count]),
                     "%s", parsed.record_digest[i]);
            member_count++;
        }
        snprintf(recorded_corpus, sizeof(recorded_corpus), "%s",
                 parsed.corpus);
    }

    /* ---- the directory must be exactly the manifest ------------------- */
    {
        char *present[MAX_MEMBERS];
        int present_count = 0;
        DIR *dir = opendir(package_dir);
        struct dirent *dent;
        if (!dir) {
            fprintf(stderr, "cdc x refused: %s unreadable\n", package_dir);
            goto done;
        }
        while ((dent = readdir(dir)) != NULL) {
            size_t length = strlen(dent->d_name);
            if (dent->d_name[0] == '.' || length < 5 ||
                strcmp(dent->d_name + length - 4, ".cdc") != 0) {
                continue;
            }
            if (present_count >= MAX_MEMBERS) {
                closedir(dir);
                fprintf(stderr, "cdc x refused: too many members\n");
                goto done;
            }
            present[present_count] = strdup(dent->d_name);
            if (!present[present_count]) {
                closedir(dir);
                goto done;
            }
            present_count++;
        }
        closedir(dir);
        qsort(present, (size_t)present_count, sizeof(present[0]),
              name_compare);
        /* member ordering is enforced by the strict parser */
        if (present_count != member_count) {
            fprintf(stderr,
                    "cdc x refused: %s carries %d .cdc file(s) but the "
                    "manifest names %d — unmanifested code does not run\n",
                    name, present_count, member_count);
            for (i = 0; i < present_count; i++) {
                free(present[i]);
            }
            goto done;
        }
        for (i = 0; i < member_count; i++) {
            if (strcmp(present[i], members[i]) != 0) {
                fprintf(stderr,
                        "cdc x refused: member set mismatch (%s vs %s)\n",
                        present[i], members[i]);
                for (i = 0; i < present_count; i++) {
                    free(present[i]);
                }
                goto done;
            }
        }
        for (i = 0; i < present_count; i++) {
            free(present[i]);
        }
    }

    /* ---- every member re-digests to its recorded digest --------------- */
    {
        const char *paths[MAX_MEMBERS];
        char stored[MAX_MEMBERS][1300];
        uint8_t digest[CDC_DIGEST_SIZE];
        char hex[96];
        for (i = 0; i < member_count; i++) {
            snprintf(stored[i], sizeof(stored[i]), "%s/%s", package_dir,
                     members[i]);
            paths[i] = stored[i];
            if (!cdc_digest_file(stored[i], digest)) {
                fprintf(stderr, "cdc x refused: %s unreadable\n",
                        members[i]);
                goto done;
            }
            cdc_digest_hex(digest, hex, sizeof(hex));
            if (strcmp(hex, recorded[i]) != 0) {
                fprintf(stderr,
                        "cdc x refused: member %s does not match its "
                        "manifest digest (tamper or drift; reinstall)\n",
                        members[i]);
                goto done;
            }
            if (strcmp(members[i], entry) == 0) {
                entry_found = 1;
            }
        }
        if (!cdc_digest_corpus(paths, (size_t)member_count, digest)) {
            fprintf(stderr, "cdc x refused: corpus unreadable\n");
            goto done;
        }
        cdc_digest_hex(digest, hex, sizeof(hex));
        if (strcmp(hex, recorded_corpus) != 0) {
            fprintf(stderr, "cdc x refused: corpus identity mismatch\n");
            goto done;
        }
    }
    if (!entry_found) {
        fprintf(stderr, "cdc x refused: %s is not a member of %s\n", entry,
                name);
        goto done;
    }

    printf("cdc x package=%s entry=%s members=%d manifest=verified "
           "trusted=local\n",
           name, entry, member_count);
    fflush(stdout);
    {
        char entry_path[1300];
        char *fused_argv[4];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", package_dir,
                 entry);
        fused_argv[0] = (char *)"cdc";
        fused_argv[1] = (char *)"fused";
        fused_argv[2] = entry_path;
        fused_argv[3] = NULL;
        rc = cdc_native_main(3, fused_argv);
    }
    if (rc == 0) {
        printf("cdc x ok package=%s entry=%s\n", name, entry);
    }

done:
    free(manifest);
    for (i = 0; i < member_count; i++) {
        free(members[i]);
    }
    return rc;
}

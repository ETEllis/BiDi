/* `cdc install` — Phase I crash-durable package install (gate CT5 seed).
 *
 * A package is a directory of `.cdc` members. Installation is
 * latch-or-hold on the durable substrate:
 *
 *   validate  every member parses through the grammar-1 frontend, and the
 *             package's OWN contract passes. A package carrying failing
 *             expectations is HELD and nothing is written. A package with
 *             ZERO expectations is refused outright: zero executed checks
 *             carry no evidence (the same rule review B4 imposed on the
 *             test gate), and installing unevidenced code silently would
 *             be the package-manager version of a merged pass total.
 *   journal   one sealed cdc_store transaction records every member —
 *             basename, content digest, and the content itself — plus the
 *             package's corpus identity. The store's sealed transactions
 *             ARE the install journal; there is no second durability
 *             mechanism to disagree with the first (D16's machinery is
 *             the machinery).
 *   latch     members and the package manifest are materialized into a
 *             staging directory, fsynced, and renamed into place. A crash
 *             anywhere leaves either no package directory or a complete
 *             one — never a partial tree wearing the real name.
 *
 * The crash window that remains is BETWEEN journal and latch: a sealed
 * journal entry whose directory never appeared. That window is documented
 * rather than hidden — re-running the install heals it (the journal gains
 * a second sealed transaction; the directory latches), and the invariant
 * that actually matters — no partial directory, no lying journal — holds
 * at every boundary, which the kill matrix in scripts/verify.sh executes
 * rather than asserts (CDC_INSTALL_KILL = before-journal | after-journal |
 * before-latch).
 *
 * Reinstalling an identical package is an idempotent no-op (no journal
 * growth); a divergent reinstall is refused typed rather than silently
 * replaced. Sources are trusted-local directories only: no network, no
 * registries, no archives — that surface waits on the CT5 hostile-package
 * gates and is stated, not smuggled. */
#define _POSIX_C_SOURCE 200809L

#include "cmd_install.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../cdc_abi.h"
#include "../cdc_digest.h"
#include "../cdc_receipt.h"
#include "../cdc_store.h"

#define MAX_MEMBERS 256

static const char *modules_root(void) {
    const char *root = getenv("CDC_MODULES");
    return root && root[0] ? root : "cdc_modules";
}

/* Deterministic kill hook for the crash matrix: the process genuinely
 * dies at a named boundary, exactly like the store's own kill matrix. */
static void kill_hook(const char *phase) {
    const char *want = getenv("CDC_INSTALL_KILL");
    if (want && strcmp(want, phase) == 0) {
        raise(SIGKILL);
    }
}

static int ensure_dir(const char *path) {
    char work[1024];
    size_t i;
    int written = snprintf(work, sizeof(work), "%s", path);
    if (written < 0 || (size_t)written >= sizeof(work)) {
        return 0;
    }
    for (i = 1; work[i]; i++) {
        if (work[i] != '/') {
            continue;
        }
        work[i] = '\0';
        if (mkdir(work, 0777) != 0 && errno != EEXIST) {
            return 0;
        }
        work[i] = '/';
    }
    return mkdir(work, 0777) == 0 || errno == EEXIST;
}

static int sync_file(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    if (fsync(fd) != 0) {
        close(fd);
        return 0;
    }
    close(fd);
    return 1;
}

/* Removes a staging directory we own: regular entries, then the dir. */
static void remove_staging(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    char child[1200];
    if (!dir) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        unlink(child);
    }
    closedir(dir);
    rmdir(path);
}

static int name_compare(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
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

static void emit_install_receipt(const char *name, cdc_outcome outcome,
                                 const char *reason, int durable,
                                 long sealed, long events, long generation) {
    void *stream = cdc_receipt_open_env();
    cdc_receipt receipt;
    if (!stream) {
        return;
    }
    cdc_receipt_init(&receipt);
    snprintf(receipt.kind, sizeof(receipt.kind), "install");
    snprintf(receipt.job, sizeof(receipt.job), "%s", name);
    snprintf(receipt.op, sizeof(receipt.op),
             outcome == CDC_OUTCOME_ACCEPTED ? "latch" : "hold");
    receipt.outcome = outcome;
    snprintf(receipt.reason, sizeof(receipt.reason), "%s", reason);
    receipt.durable = durable;
    receipt.sealed = sealed;
    receipt.events = events;
    receipt.generation = generation;
    if (cdc_receipt_emit(stream, &receipt) < 0) {
        fprintf(stderr, "cdc install: ill-formed receipt (internal)\n");
    }
    cdc_receipt_close(stream);
}

int cdc_cmd_install(int argc, char **argv) {
    const char *package_dir;
    const char *name;
    char *members[MAX_MEMBERS];
    int member_count = 0;
    char member_path[1200];
    char dest[1024], staging[1060], journal_dir[1060], manifest_path[1100];
    char *manifest_text = NULL;
    size_t manifest_length = 0;
    FILE *manifest_mem;
    uint8_t digest[CDC_DIGEST_SIZE];
    char corpus_hex[96];
    long sealed_after = 0, events_after = 0, generation_after = 0;
    int i;
    int rc = 1;

    if (argc < 1 || argv[0][0] == '-') {
        fprintf(stderr, "usage: cdc install <package-dir>\n");
        return 2;
    }
    package_dir = argv[0];
    {
        const char *slash = strrchr(package_dir, '/');
        name = slash ? slash + 1 : package_dir;
    }
    if (name[0] == '\0' || name[0] == '.') {
        fprintf(stderr, "cdc install: package directory must have a plain "
                        "basename\n");
        return 2;
    }
    if ((size_t)snprintf(dest, sizeof(dest), "%s/%s", modules_root(),
                         name) >= sizeof(dest) ||
        (size_t)snprintf(staging, sizeof(staging), "%s/.staging-%s",
                         modules_root(), name) >= sizeof(staging) ||
        (size_t)snprintf(journal_dir, sizeof(journal_dir), "%s/.journal",
                         modules_root()) >= sizeof(journal_dir)) {
        fprintf(stderr, "cdc install: path too long\n");
        return 2;
    }

    /* ---- enumerate members (sorted, deterministic) ------------------- */
    {
        DIR *dir = opendir(package_dir);
        struct dirent *entry;
        if (!dir) {
            fprintf(stderr, "cdc install: cannot open %s\n", package_dir);
            return 1;
        }
        while ((entry = readdir(dir)) != NULL) {
            size_t length = strlen(entry->d_name);
            if (entry->d_name[0] == '.' || length < 5 ||
                strcmp(entry->d_name + length - 4, ".cdc") != 0) {
                continue;
            }
            if (member_count >= MAX_MEMBERS) {
                fprintf(stderr, "cdc install: too many members\n");
                closedir(dir);
                return 1;
            }
            members[member_count] = strdup(entry->d_name);
            if (!members[member_count]) {
                closedir(dir);
                return 1;
            }
            member_count++;
        }
        closedir(dir);
    }
    if (member_count == 0) {
        fprintf(stderr, "cdc install: %s has no .cdc members; an empty "
                        "package is refused\n",
                package_dir);
        return 1;
    }
    qsort(members, (size_t)member_count, sizeof(members[0]), name_compare);

    /* ---- validate: frontend + the package's own contract ------------- */
    {
        cdc_runtime *runtime = NULL;
        cdc_result *vectors = NULL;
        long check_count;
        if (cdc_runtime_create(&runtime) != CDC_OK) {
            fprintf(stderr, "cdc install: runtime creation failed\n");
            goto free_members;
        }
        for (i = 0; i < member_count; i++) {
            cdc_program *program = NULL;
            cdc_status status;
            snprintf(member_path, sizeof(member_path), "%s/%s", package_dir,
                     members[i]);
            status = cdc_program_parse(member_path, NULL, 0, &program);
            if (status != CDC_OK || cdc_program_error_count(program) > 0) {
                fprintf(stderr,
                        "cdc install held name=%s reason=member-rejected "
                        "member=%s (%s, %zu diagnostics; nothing written)\n",
                        name, members[i], cdc_status_name(status),
                        program ? cdc_program_error_count(program) : 0);
                cdc_program_destroy(program);
                cdc_runtime_destroy(runtime);
                emit_install_receipt(name, CDC_OUTCOME_HELD,
                                     "member-rejected", 0, CDC_RECEIPT_NA,
                                     CDC_RECEIPT_NA, CDC_RECEIPT_NA);
                goto free_members;
            }
            if (cdc_runtime_load(runtime, program) != CDC_OK) {
                fprintf(stderr, "cdc install: load failed for %s\n",
                        members[i]);
                cdc_program_destroy(program);
                cdc_runtime_destroy(runtime);
                goto free_members;
            }
        }
        if (cdc_runtime_vectors(runtime, NULL, &vectors) != CDC_OK) {
            fprintf(stderr, "cdc install: contract evaluation failed\n");
            cdc_runtime_destroy(runtime);
            goto free_members;
        }
        check_count = 0;
        {
            const char *text = cdc_result_text(vectors);
            for (; *text; text++) {
                if (*text == '\n') {
                    check_count++;
                }
            }
        }
        if (check_count == 0) {
            fprintf(stderr,
                    "cdc install refused name=%s reason=zero-evidence (the "
                    "package declares no expectations; zero executed checks "
                    "carry no evidence, so there is nothing to stand behind "
                    "an install)\n",
                    name);
            cdc_result_destroy(vectors);
            cdc_runtime_destroy(runtime);
            goto free_members;
        }
        if (cdc_result_error_count(vectors) > 0) {
            fprintf(stderr,
                    "cdc install held name=%s reason=closure-violation "
                    "(package expectations fail; nothing written)\n",
                    name);
            cdc_result_destroy(vectors);
            cdc_runtime_destroy(runtime);
            emit_install_receipt(name, CDC_OUTCOME_HELD, "closure-violation",
                                 0, CDC_RECEIPT_NA, CDC_RECEIPT_NA,
                                 CDC_RECEIPT_NA);
            goto free_members;
        }
        cdc_result_destroy(vectors);
        cdc_runtime_destroy(runtime);
    }

    /* ---- manifest text (also the idempotence key) -------------------- */
    manifest_mem = open_memstream(&manifest_text, &manifest_length);
    if (!manifest_mem) {
        goto free_members;
    }
    fprintf(manifest_mem, "cdc-package v=1 name=%s files=%d\n", name,
            member_count);
    {
        const char *paths[MAX_MEMBERS];
        char stored_paths[MAX_MEMBERS][1200];
        for (i = 0; i < member_count; i++) {
            char hex[96];
            snprintf(stored_paths[i], sizeof(stored_paths[i]), "%s/%s",
                     package_dir, members[i]);
            paths[i] = stored_paths[i];
            if (!cdc_digest_file(stored_paths[i], digest)) {
                fprintf(stderr, "cdc install: %s unreadable\n",
                        stored_paths[i]);
                fclose(manifest_mem);
                free(manifest_text);
                goto free_members;
            }
            cdc_digest_hex(digest, hex, sizeof(hex));
            fprintf(manifest_mem, "member %s %s\n", members[i], hex);
        }
        if (!cdc_digest_corpus(paths, (size_t)member_count, digest)) {
            fclose(manifest_mem);
            free(manifest_text);
            goto free_members;
        }
        cdc_digest_hex(digest, corpus_hex, sizeof(corpus_hex));
        fprintf(manifest_mem, "corpus %s\n", corpus_hex);
    }
    fclose(manifest_mem);

    /* ---- idempotence / divergence ------------------------------------ */
    {
        struct stat st;
        if (stat(dest, &st) == 0) {
            char existing_path[1100];
            char *existing = NULL;
            size_t existing_length = 0;
            snprintf(existing_path, sizeof(existing_path), "%s/.manifest",
                     dest);
            if (read_all(existing_path, &existing, &existing_length) &&
                existing_length == manifest_length &&
                memcmp(existing, manifest_text, manifest_length) == 0) {
                printf("cdc install ok name=%s files=%d "
                       "already-installed=identical\n",
                       name, member_count);
                free(existing);
                free(manifest_text);
                rc = 0;
                goto free_members;
            }
            free(existing);
            fprintf(stderr,
                    "cdc install refused name=%s "
                    "reason=already-installed-divergent (an installed "
                    "package is never silently replaced; remove %s first)\n",
                    name, dest);
            free(manifest_text);
            goto free_members;
        }
    }

    if (!ensure_dir(modules_root())) {
        fprintf(stderr, "cdc install: cannot create %s\n", modules_root());
        free(manifest_text);
        goto free_members;
    }
    remove_staging(staging); /* a stale staging dir is ours; clear it */

    kill_hook("before-journal");

    /* ---- journal: one sealed transaction is the install record ------- */
    {
        cdc_store *journal = NULL;
        cdc_store_status status;
        if (cdc_store_open(journal_dir, &journal, NULL) != CDC_STORE_OK) {
            fprintf(stderr, "cdc install: journal store unavailable\n");
            free(manifest_text);
            goto free_members;
        }
        for (i = 0; i < member_count && journal; i++) {
            char *content = NULL;
            size_t content_length = 0;
            char *payload = NULL;
            size_t payload_length = 0;
            FILE *payload_mem;
            char hex[96];
            snprintf(member_path, sizeof(member_path), "%s/%s", package_dir,
                     members[i]);
            if (!read_all(member_path, &content, &content_length) ||
                !cdc_digest_file(member_path, digest)) {
                fprintf(stderr, "cdc install: %s unreadable\n", member_path);
                free(content);
                cdc_store_rollback(journal);
                cdc_store_close(journal);
                free(manifest_text);
                goto free_members;
            }
            cdc_digest_hex(digest, hex, sizeof(hex));
            payload_mem = open_memstream(&payload, &payload_length);
            if (!payload_mem) {
                free(content);
                cdc_store_rollback(journal);
                cdc_store_close(journal);
                free(manifest_text);
                goto free_members;
            }
            fprintf(payload_mem, "member %s %s %s\n", name, members[i], hex);
            fwrite(content, 1, content_length, payload_mem);
            fclose(payload_mem);
            free(content);
            status = cdc_store_stage(journal, payload, payload_length);
            free(payload);
            if (status != CDC_STORE_OK) {
                fprintf(stderr, "cdc install: journal staging failed (%s)\n",
                        cdc_store_status_name(status));
                cdc_store_rollback(journal);
                cdc_store_close(journal);
                free(manifest_text);
                goto free_members;
            }
        }
        {
            char seal_line[512];
            int written = snprintf(seal_line, sizeof(seal_line),
                                   "package %s corpus=%s files=%d", name,
                                   corpus_hex, member_count);
            status = cdc_store_stage(journal, seal_line, (size_t)written);
            if (status == CDC_STORE_OK) {
                status = cdc_store_commit(journal);
            }
            if (status != CDC_STORE_OK) {
                fprintf(stderr, "cdc install: journal commit failed (%s); "
                                "nothing latched\n",
                        cdc_store_status_name(status));
                cdc_store_rollback(journal);
                cdc_store_close(journal);
                free(manifest_text);
                goto free_members;
            }
        }
        sealed_after = (long)cdc_store_sealed_count(journal);
        events_after = (long)cdc_store_event_count(journal);
        generation_after = (long)cdc_store_generation(journal);
        cdc_store_close(journal);
    }

    kill_hook("after-journal");

    /* ---- latch: staging + fsync + rename ----------------------------- */
    if (!ensure_dir(staging)) {
        fprintf(stderr, "cdc install: cannot create staging\n");
        free(manifest_text);
        goto free_members;
    }
    for (i = 0; i < member_count; i++) {
        char *content = NULL;
        size_t content_length = 0;
        char out_path[1300];
        FILE *out;
        snprintf(member_path, sizeof(member_path), "%s/%s", package_dir,
                 members[i]);
        snprintf(out_path, sizeof(out_path), "%s/%s", staging, members[i]);
        if (!read_all(member_path, &content, &content_length)) {
            free(manifest_text);
            goto staging_failed;
        }
        out = fopen(out_path, "wb");
        if (!out || fwrite(content, 1, content_length, out) !=
                        content_length ||
            fflush(out) != 0) {
            if (out) {
                fclose(out);
            }
            free(content);
            free(manifest_text);
            goto staging_failed;
        }
        fclose(out);
        free(content);
        if (!sync_file(out_path)) {
            free(manifest_text);
            goto staging_failed;
        }
    }
    snprintf(manifest_path, sizeof(manifest_path), "%s/.manifest", staging);
    {
        FILE *out = fopen(manifest_path, "wb");
        if (!out ||
            fwrite(manifest_text, 1, manifest_length, out) !=
                manifest_length ||
            fflush(out) != 0) {
            if (out) {
                fclose(out);
            }
            free(manifest_text);
            goto staging_failed;
        }
        fclose(out);
        if (!sync_file(manifest_path) || !sync_file(staging)) {
            free(manifest_text);
            goto staging_failed;
        }
    }
    free(manifest_text);
    manifest_text = NULL;

    kill_hook("before-latch");

    if (rename(staging, dest) != 0) {
        fprintf(stderr, "cdc install: latch failed (%s exists or rename "
                        "denied)\n",
                dest);
        goto staging_failed;
    }
    sync_file(modules_root());

    emit_install_receipt(name, CDC_OUTCOME_ACCEPTED, "none", 1, sealed_after,
                         events_after, generation_after);
    printf("cdc install ok name=%s files=%d sealed=%ld corpus=%s\n", name,
           member_count, sealed_after, corpus_hex);
    rc = 0;
    goto free_members;

staging_failed:
    remove_staging(staging);
free_members:
    for (i = 0; i < member_count; i++) {
        free(members[i]);
    }
    return rc;
}

/* `cdc install` — Phase I crash-durable package install (gate CT5 seed).
 *
 * A package is a directory of `.cdc` members. Installation is
 * latch-or-hold on the durable substrate:
 *
 *   capture   every member is read ONCE into immutable attempt-owned
 *             bytes. Digests, validation, the journal, and the staged
 *             tree all derive from those bytes — never from a second
 *             read — so a source mutating mid-install cannot split the
 *             installed identity between what was digested and what was
 *             written (second 2026-07-28 review, finding 4).
 *   validate  every member parses through the grammar-1 frontend, and the
 *             package's OWN contract passes. A package carrying failing
 *             expectations is HELD and nothing is written. A package with
 *             ZERO expectations is refused outright: zero executed checks
 *             carry no evidence (the same rule review B4 imposed on the
 *             test gate), and installing unevidenced code silently would
 *             be the package-manager version of a merged pass total.
 *   lock      a package-scoped fcntl lock (`.locks/<name>.lock`) is held
 *             from the existing-state check through activation, so two
 *             installers of one package serialize: the winner installs,
 *             the loser re-checks under the lock and becomes either an
 *             idempotent observation (identical) or a typed refusal
 *             (divergent). Without it, both passed the check before
 *             either latched and the journal gained DUPLICATE
 *             transactions for one logical install. The lock file is
 *             per-package, so unrelated installs do not serialize.
 *   journal   one sealed cdc_store transaction records every member —
 *             basename, content digest, and the content itself — plus the
 *             package's corpus identity. The store's sealed transactions
 *             ARE the install journal; there is no second durability
 *             mechanism to disagree with the first (D16's machinery is
 *             the machinery).
 *   latch     members and the package manifest are materialized into a
 *             staging directory UNIQUE TO THIS ATTEMPT
 *             (`.staging-<name>.<pid>` — a fixed path let attempts scrub
 *             each other's half-written trees), fsynced, and renamed into
 *             place; the modules directory is then fsynced and that
 *             result is CHECKED — `durable=1` is only ever emitted after
 *             the sync that makes it true. A crash anywhere leaves either
 *             no package directory or a complete one — never a partial
 *             tree wearing the real name.
 *
 * The crash window that remains is BETWEEN journal and latch: a sealed
 * journal entry whose directory never appeared. That window is documented
 * rather than hidden — re-running the install heals it — and the kill
 * matrix in scripts/verify.sh executes the boundaries rather than
 * asserting them (CDC_INSTALL_KILL = before-journal | after-journal |
 * before-latch | after-latch, the last one dying between the rename and
 * the directory sync).
 *
 * Reinstalling an identical package is an idempotent no-op (no journal
 * growth) that re-syncs the modules directory, so it also heals an
 * unproven latch. A divergent reinstall is refused typed rather than
 * silently replaced; an installed manifest the strict parser refuses is
 * its own typed refusal (the installed state is suspect — nothing is
 * compared against it). Sources are trusted-local directories only: no
 * network, no registries, no archives — that surface waits on the CT5
 * hostile-package gates and is stated, not smuggled.
 *
 * CDC_INSTALL_PAUSE_AFTER_CAPTURE=<fifo> is a test-only rendezvous: the
 * install blocks on the fifo after capture and before the lock, so the
 * concurrency gates can hold two attempts at the same boundary and
 * release them together. Never set in production. */
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
#include "cdc_manifest.h"

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

/* Test-only rendezvous (see header comment). Blocks on the fifo until
 * the orchestrating test releases this attempt. */
static void pause_hook(void) {
    const char *fifo = getenv("CDC_INSTALL_PAUSE_AFTER_CAPTURE");
    char byte;
    ssize_t n;
    int fd;
    if (!fifo || !fifo[0]) {
        return;
    }
    fd = open(fifo, O_RDONLY);
    if (fd < 0) {
        return;
    }
    do {
        n = read(fd, &byte, 1);
    } while (n < 0 && errno == EINTR);
    close(fd);
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

/* Takes the package-scoped install lock, blocking until it is granted.
 * Scope, stated: `cdc install` runs once per process, so fcntl's
 * per-process ownership is exactly the serialization the CLI surface
 * needs; if installs ever become an in-process library call, this lock
 * needs the store's shared-coordination treatment (D29). */
static int package_lock(const char *name) {
    char lock_dir[1100], lock_path[1300];
    struct flock lock;
    int fd;
    if ((size_t)snprintf(lock_dir, sizeof(lock_dir), "%s/.locks",
                         modules_root()) >= sizeof(lock_dir) ||
        !ensure_dir(lock_dir) ||
        (size_t)snprintf(lock_path, sizeof(lock_path), "%s/%s.lock",
                         lock_dir, name) >= sizeof(lock_path)) {
        return -1;
    }
    fd = open(lock_path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        return -1;
    }
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    while (fcntl(fd, F_SETLKW, &lock) != 0) {
        if (errno != EINTR) {
            close(fd);
            return -1;
        }
    }
    return fd;
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

/* Prunes staging directories left by DEAD attempts of THIS package.
 * Called under the package lock, so no live attempt of this package can
 * own one; other packages' staging is never touched. */
static void prune_stale_staging(const char *name) {
    char prefix[1100];
    char victim[1300];
    DIR *dir;
    struct dirent *entry;
    int written = snprintf(prefix, sizeof(prefix), ".staging-%s.", name);
    if (written < 0 || (size_t)written >= sizeof(prefix)) {
        return;
    }
    dir = opendir(modules_root());
    if (!dir) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, prefix, (size_t)written) != 0) {
            continue;
        }
        snprintf(victim, sizeof(victim), "%s/%s", modules_root(),
                 entry->d_name);
        remove_staging(victim);
    }
    closedir(dir);
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

static int write_bytes_synced(const char *path, const char *bytes,
                              size_t length) {
    FILE *out = fopen(path, "wb");
    if (!out) {
        return 0;
    }
    if (fwrite(bytes, 1, length, out) != length || fflush(out) != 0) {
        fclose(out);
        return 0;
    }
    fclose(out);
    return sync_file(path);
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
    char *captured[MAX_MEMBERS];
    size_t captured_length[MAX_MEMBERS];
    uint8_t captured_digest[MAX_MEMBERS][CDC_DIGEST_SIZE];
    char captured_hex[MAX_MEMBERS][96];
    int member_count = 0;
    char member_path[1200];
    char dest[1024], staging[1080], journal_dir[1060], manifest_path[1140];
    char *manifest_text = NULL;
    size_t manifest_length = 0;
    FILE *manifest_mem;
    uint8_t digest[CDC_DIGEST_SIZE];
    char corpus_hex[96];
    long sealed_after = 0, events_after = 0, generation_after = 0;
    int lock_fd = -1;
    int staged_created = 0;
    int i;
    int rc = 1;

    for (i = 0; i < MAX_MEMBERS; i++) {
        captured[i] = NULL;
    }
    if (argc < 1 || argv[0][0] == '-') {
        fprintf(stderr, "usage: cdc install <package-dir>\n");
        return 2;
    }
    /* D17's stream contract applies here too: when CDC_RECEIPTS is set,
     * the stream exists EAGERLY, so "no receipt" (a killed or refused
     * attempt) is an empty stream — distinguishable from "no stream". */
    {
        void *stream = cdc_receipt_open_env();
        if (stream) {
            cdc_receipt_close(stream);
        }
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
        (size_t)snprintf(staging, sizeof(staging), "%s/.staging-%s.%ld",
                         modules_root(), name,
                         (long)getpid()) >= sizeof(staging) ||
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

    /* ---- capture: one read, immutable attempt-owned bytes ------------ */
    for (i = 0; i < member_count; i++) {
        snprintf(member_path, sizeof(member_path), "%s/%s", package_dir,
                 members[i]);
        if (!read_all(member_path, &captured[i], &captured_length[i])) {
            fprintf(stderr, "cdc install: %s unreadable\n", member_path);
            goto cleanup;
        }
        cdc_digest(captured[i], captured_length[i], captured_digest[i]);
        cdc_digest_hex(captured_digest[i], captured_hex[i],
                       sizeof(captured_hex[i]));
    }

    /* ---- validate FROM THE CAPTURED BYTES ---------------------------- */
    {
        cdc_runtime *runtime = NULL;
        cdc_result *vectors = NULL;
        long check_count;
        if (cdc_runtime_create(&runtime) != CDC_OK) {
            fprintf(stderr, "cdc install: runtime creation failed\n");
            goto cleanup;
        }
        for (i = 0; i < member_count; i++) {
            cdc_program *program = NULL;
            cdc_status status = cdc_program_parse(
                NULL, captured[i], captured_length[i], &program);
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
                goto cleanup;
            }
            if (cdc_runtime_load(runtime, program) != CDC_OK) {
                fprintf(stderr, "cdc install: load failed for %s\n",
                        members[i]);
                cdc_program_destroy(program);
                cdc_runtime_destroy(runtime);
                goto cleanup;
            }
        }
        if (cdc_runtime_vectors(runtime, NULL, &vectors) != CDC_OK) {
            fprintf(stderr, "cdc install: contract evaluation failed\n");
            cdc_runtime_destroy(runtime);
            goto cleanup;
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
            goto cleanup;
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
            goto cleanup;
        }
        cdc_result_destroy(vectors);
        cdc_runtime_destroy(runtime);
    }

    /* ---- manifest text (also the idempotence key), from captures ----- */
    manifest_mem = open_memstream(&manifest_text, &manifest_length);
    if (!manifest_mem) {
        goto cleanup;
    }
    fprintf(manifest_mem, "cdc-package v=1 name=%s files=%d\n", name,
            member_count);
    for (i = 0; i < member_count; i++) {
        fprintf(manifest_mem, "member %s %s\n", members[i],
                captured_hex[i]);
    }
    if (!cdc_digest_corpus_pairs((const char *const *)members,
                                 (const uint8_t(*)[CDC_DIGEST_SIZE])
                                     captured_digest,
                                 (size_t)member_count, digest)) {
        fclose(manifest_mem);
        goto cleanup;
    }
    cdc_digest_hex(digest, corpus_hex, sizeof(corpus_hex));
    fprintf(manifest_mem, "corpus %s\n", corpus_hex);
    fclose(manifest_mem);
    /* self-check: never write a manifest the shared strict parser refuses */
    {
        static cdc_manifest self_check;
        char parse_error[128];
        if (!cdc_manifest_parse(manifest_text, manifest_length,
                                CDC_MANIFEST_PACKAGE, 0, name, &self_check,
                                parse_error, sizeof(parse_error))) {
            fprintf(stderr,
                    "cdc install: internal error — composed manifest fails "
                    "its own parser (%s)\n",
                    parse_error);
            goto cleanup;
        }
    }

    pause_hook(); /* test-only rendezvous; no-op in production */

    if (!ensure_dir(modules_root())) {
        fprintf(stderr, "cdc install: cannot create %s\n", modules_root());
        goto cleanup;
    }

    /* ---- package-scoped lock: check-then-act becomes one action ------ */
    lock_fd = package_lock(name);
    if (lock_fd < 0) {
        fprintf(stderr, "cdc install: cannot take the package lock\n");
        goto cleanup;
    }
    prune_stale_staging(name);

    /* ---- existing state, decided UNDER the lock ---------------------- */
    {
        struct stat st;
        if (stat(dest, &st) == 0) {
            char existing_path[1100];
            char *existing = NULL;
            size_t existing_length = 0;
            static cdc_manifest installed;
            char parse_error[128];
            snprintf(existing_path, sizeof(existing_path), "%s/.manifest",
                     dest);
            if (!read_all(existing_path, &existing, &existing_length) ||
                !cdc_manifest_parse(existing, existing_length,
                                    CDC_MANIFEST_PACKAGE, 0, name,
                                    &installed, parse_error,
                                    sizeof(parse_error))) {
                fprintf(stderr,
                        "cdc install refused name=%s "
                        "reason=installed-manifest-malformed (the installed "
                        "state is suspect and is never compared against; "
                        "remove %s to recover)\n",
                        name, dest);
                free(existing);
                goto cleanup;
            }
            if (existing_length == manifest_length &&
                memcmp(existing, manifest_text, manifest_length) == 0) {
                free(existing);
                /* Idempotent observation — and re-syncing the modules
                 * directory here means a re-run heals a latch whose sync
                 * never got to run. */
                if (!sync_file(modules_root())) {
                    fprintf(stderr, "cdc install refused name=%s "
                                    "reason=latch-sync-failed\n",
                            name);
                    goto cleanup;
                }
                printf("cdc install ok name=%s files=%d "
                       "already-installed=identical\n",
                       name, member_count);
                rc = 0;
                goto cleanup;
            }
            free(existing);
            fprintf(stderr,
                    "cdc install refused name=%s "
                    "reason=already-installed-divergent (an installed "
                    "package is never silently replaced; remove %s first)\n",
                    name, dest);
            goto cleanup;
        }
    }

    kill_hook("before-journal");

    /* ---- journal: one sealed transaction is the install record ------- */
    {
        cdc_store *journal = NULL;
        cdc_store_status status;
        if (cdc_store_open(journal_dir, &journal, NULL) != CDC_STORE_OK) {
            fprintf(stderr, "cdc install: journal store unavailable\n");
            goto cleanup;
        }
        for (i = 0; i < member_count; i++) {
            char *payload = NULL;
            size_t payload_length = 0;
            FILE *payload_mem = open_memstream(&payload, &payload_length);
            if (!payload_mem) {
                cdc_store_rollback(journal);
                cdc_store_close(journal);
                goto cleanup;
            }
            fprintf(payload_mem, "member %s %s %s\n", name, members[i],
                    captured_hex[i]);
            fwrite(captured[i], 1, captured_length[i], payload_mem);
            fclose(payload_mem);
            status = cdc_store_stage(journal, payload, payload_length);
            free(payload);
            if (status != CDC_STORE_OK) {
                fprintf(stderr, "cdc install: journal staging failed (%s)\n",
                        cdc_store_status_name(status));
                cdc_store_rollback(journal);
                cdc_store_close(journal);
                goto cleanup;
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
                goto cleanup;
            }
        }
        sealed_after = (long)cdc_store_sealed_count(journal);
        events_after = (long)cdc_store_event_count(journal);
        generation_after = (long)cdc_store_generation(journal);
        cdc_store_close(journal);
    }

    kill_hook("after-journal");

    /* ---- latch: unique staging + fsync + rename + CHECKED dir sync --- */
    if (!ensure_dir(staging)) {
        fprintf(stderr, "cdc install: cannot create staging\n");
        goto cleanup;
    }
    staged_created = 1;
    for (i = 0; i < member_count; i++) {
        char out_path[1400];
        snprintf(out_path, sizeof(out_path), "%s/%s", staging, members[i]);
        if (!write_bytes_synced(out_path, captured[i],
                                captured_length[i])) {
            goto cleanup;
        }
    }
    snprintf(manifest_path, sizeof(manifest_path), "%s/.manifest", staging);
    if (!write_bytes_synced(manifest_path, manifest_text,
                            manifest_length) ||
        !sync_file(staging)) {
        goto cleanup;
    }

    kill_hook("before-latch");

    if (rename(staging, dest) != 0) {
        fprintf(stderr, "cdc install: latch failed (%s exists or rename "
                        "denied)\n",
                dest);
        goto cleanup;
    }
    staged_created = 0; /* the staging tree IS dest now */

    kill_hook("after-latch");

    /* The rename is not durable until the directory that carries it is.
     * This result used to be ignored; durable=1 was a claim, not an
     * observation. */
    if (!sync_file(modules_root())) {
        fprintf(stderr,
                "cdc install refused name=%s reason=latch-sync-failed "
                "(the package is in place but its durability is unproven; "
                "re-run to heal)\n",
                name);
        emit_install_receipt(name, CDC_OUTCOME_HELD, "latch-sync-failed", 0,
                             sealed_after, events_after, generation_after);
        goto cleanup;
    }

    emit_install_receipt(name, CDC_OUTCOME_ACCEPTED, "none", 1, sealed_after,
                         events_after, generation_after);
    printf("cdc install ok name=%s files=%d sealed=%ld corpus=%s\n", name,
           member_count, sealed_after, corpus_hex);
    rc = 0;

cleanup:
    if (staged_created) {
        remove_staging(staging);
    }
    if (lock_fd >= 0) {
        close(lock_fd);
    }
    free(manifest_text);
    for (i = 0; i < member_count; i++) {
        free(members[i]);
        free(captured[i]);
    }
    return rc;
}

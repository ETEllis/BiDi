/* See cdc_manifest.h for the closed grammar this file enforces. */
#define _POSIX_C_SOURCE 200809L

#include "cdc_manifest.h"

#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t error_size, const char *what) {
    if (error && error_size) {
        snprintf(error, error_size, "%s", what);
    }
}

static int is_digits(const char *s) {
    if (!s || !*s) {
        return 0;
    }
    for (; *s; s++) {
        if (*s < '0' || *s > '9') {
            return 0;
        }
    }
    return 1;
}

static int parse_count(const char *s, long *out) {
    long value = 0;
    if (!is_digits(s) || strlen(s) > 9) {
        return 0;
    }
    for (; *s; s++) {
        value = value * 10 + (*s - '0');
    }
    *out = value;
    return 1;
}

static int is_digest(const char *s) {
    size_t i;
    if (!s || strncmp(s, "blake3:", 7) != 0 || strlen(s) != 7 + 64) {
        return 0;
    }
    for (i = 7; i < 7 + 64; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            return 0;
        }
    }
    return 1;
}

static int is_record_name(const char *s) {
    size_t length = s ? strlen(s) : 0;
    if (length < 5 || length >= CDC_MANIFEST_NAME_MAX) {
        return 0;
    }
    if (s[0] == '.' || strchr(s, '/') != NULL) {
        return 0;
    }
    return strcmp(s + length - 4, ".cdc") == 0;
}

/* Returns the value of a "key=value" token or NULL if the key differs.
 * The value may not be empty. */
static const char *kv(const char *token, const char *key) {
    size_t key_length = strlen(key);
    if (strncmp(token, key, key_length) != 0 || token[key_length] != '=') {
        return NULL;
    }
    return token[key_length + 1] ? token + key_length + 1 : NULL;
}

/* Splits `line` (mutated in place) on single spaces into at most `max`
 * tokens. Returns the token count, or -1 on empty tokens (double spaces,
 * leading/trailing space) or overflow — both malformed. */
static int split_tokens(char *line, char **tokens, int max) {
    int count = 0;
    char *cursor = line;
    if (*cursor == '\0') {
        return -1;
    }
    for (;;) {
        char *space;
        if (count >= max) {
            return -1;
        }
        tokens[count++] = cursor;
        space = strchr(cursor, ' ');
        if (!space) {
            break;
        }
        *space = '\0';
        cursor = space + 1;
        if (*cursor == '\0' || *cursor == ' ') {
            return -1; /* trailing or doubled space */
        }
    }
    return count;
}

int cdc_manifest_parse(const char *text, size_t length,
                       cdc_manifest_kind kind, uint32_t expected_abi,
                       const char *expected_name, cdc_manifest *out,
                       char *error, size_t error_size) {
    const char *cursor = text;
    const char *end_of_text;
    int line_number = 0;
    int header_seen = 0;
    int corpus_seen = 0;
    int artifact_seen = 0;
    long declared_files = -1;

    set_error(error, error_size, "unparsed");
    if (!text || !out || length == 0) {
        set_error(error, error_size, "empty manifest");
        return 0;
    }
    if (kind == CDC_MANIFEST_PACKAGE && !expected_name) {
        set_error(error, error_size, "internal: expected name missing");
        return 0;
    }
    if (text[length - 1] != '\n') {
        set_error(error, error_size, "final line not newline-terminated");
        return 0;
    }
    if (strlen(text) != length) {
        set_error(error, error_size, "embedded NUL byte");
        return 0;
    }
    memset(out, 0, sizeof(*out));
    out->kind = kind;
    end_of_text = text + length;

    while (cursor < end_of_text) {
        char line[1200];
        char *tokens[8];
        int token_count;
        const char *newline = strchr(cursor, '\n');
        size_t line_length = (size_t)(newline - cursor);
        line_number++;
        if (line_length >= sizeof(line)) {
            set_error(error, error_size, "overlong line");
            return 0;
        }
        memcpy(line, cursor, line_length);
        line[line_length] = '\0';
        cursor = newline + 1;

        token_count = split_tokens(line, tokens, 8);
        if (token_count < 0) {
            char what[64];
            snprintf(what, sizeof(what), "malformed spacing (line %d)",
                     line_number);
            set_error(error, error_size, what);
            return 0;
        }

        /* ---- header: line 1, exactly once ----------------------------- */
        if (strcmp(tokens[0], "cdc-bundle") == 0 ||
            strcmp(tokens[0], "cdc-package") == 0) {
            const char *value;
            if (header_seen) {
                set_error(error, error_size, "duplicate header");
                return 0;
            }
            if (line_number != 1) {
                set_error(error, error_size, "header is not line 1");
                return 0;
            }
            header_seen = 1;
            if (kind == CDC_MANIFEST_BUNDLE) {
                long files = 0;
                unsigned major = 0, minor = 0;
                char extra = 0;
                if (strcmp(tokens[0], "cdc-bundle") != 0) {
                    set_error(error, error_size, "wrong manifest kind");
                    return 0;
                }
                if (token_count != 7) {
                    set_error(error, error_size,
                              "header field count (want v grammar abi "
                              "files statements checks)");
                    return 0;
                }
                value = kv(tokens[1], "v");
                if (!value || strcmp(value, "1") != 0) {
                    set_error(error, error_size, "field=v (must be 1)");
                    return 0;
                }
                value = kv(tokens[2], "grammar");
                if (!value || strcmp(value, "1") != 0) {
                    set_error(error, error_size,
                              "field=grammar (must be 1)");
                    return 0;
                }
                out->grammar = 1;
                value = kv(tokens[3], "abi");
                if (!value ||
                    sscanf(value, "%u.%u%c", &major, &minor, &extra) != 2) {
                    set_error(error, error_size, "field=abi (malformed)");
                    return 0;
                }
                if (major != (expected_abi >> 16) ||
                    minor != (expected_abi & 0xffffu)) {
                    set_error(error, error_size,
                              "field=abi (does not match this toolchain)");
                    return 0;
                }
                out->abi_major = major;
                out->abi_minor = minor;
                value = kv(tokens[4], "files");
                if (!value || !parse_count(value, &files) || files < 1 ||
                    files > CDC_MANIFEST_MAX_RECORDS) {
                    set_error(error, error_size, "field=files");
                    return 0;
                }
                declared_files = files;
                out->files = files;
                value = kv(tokens[5], "statements");
                if (!value || !parse_count(value, &out->statements)) {
                    set_error(error, error_size, "field=statements");
                    return 0;
                }
                value = kv(tokens[6], "checks");
                if (!value || !parse_count(value, &out->checks)) {
                    set_error(error, error_size, "field=checks");
                    return 0;
                }
            } else {
                long files = 0;
                if (strcmp(tokens[0], "cdc-package") != 0) {
                    set_error(error, error_size, "wrong manifest kind");
                    return 0;
                }
                if (token_count != 4) {
                    set_error(error, error_size,
                              "header field count (want v name files)");
                    return 0;
                }
                value = kv(tokens[1], "v");
                if (!value || strcmp(value, "1") != 0) {
                    set_error(error, error_size, "field=v (must be 1)");
                    return 0;
                }
                value = kv(tokens[2], "name");
                if (!value || strlen(value) >= sizeof(out->name) ||
                    strchr(value, '/') != NULL || value[0] == '.') {
                    set_error(error, error_size, "field=name (malformed)");
                    return 0;
                }
                if (strcmp(value, expected_name) != 0) {
                    set_error(error, error_size,
                              "field=name (does not match this package)");
                    return 0;
                }
                snprintf(out->name, sizeof(out->name), "%s", value);
                value = kv(tokens[3], "files");
                if (!value || !parse_count(value, &files) || files < 1 ||
                    files > CDC_MANIFEST_MAX_RECORDS) {
                    set_error(error, error_size, "field=files");
                    return 0;
                }
                declared_files = files;
                out->files = files;
            }
            continue;
        }

        if (!header_seen) {
            set_error(error, error_size, "missing header (line 1)");
            return 0;
        }

        /* ---- records: exactly files= of them, then corpus (, artifact) */
        if ((kind == CDC_MANIFEST_BUNDLE &&
             strcmp(tokens[0], "source") == 0) ||
            (kind == CDC_MANIFEST_PACKAGE &&
             strcmp(tokens[0], "member") == 0)) {
            if (corpus_seen) {
                set_error(error, error_size, "record after corpus line");
                return 0;
            }
            if (out->record_count >= declared_files) {
                set_error(error, error_size,
                          "more records than files= declares");
                return 0;
            }
            if (token_count != 3 || !is_record_name(tokens[1]) ||
                !is_digest(tokens[2])) {
                char what[64];
                snprintf(what, sizeof(what), "malformed record (line %d)",
                         line_number);
                set_error(error, error_size, what);
                return 0;
            }
            if (kind == CDC_MANIFEST_PACKAGE && out->record_count > 0 &&
                strcmp(out->record_name[out->record_count - 1], tokens[1]) >=
                    0) {
                set_error(error, error_size,
                          "members not strictly ascending");
                return 0;
            }
            if (kind == CDC_MANIFEST_BUNDLE) {
                int i;
                for (i = 0; i < out->record_count; i++) {
                    if (strcmp(out->record_name[i], tokens[1]) == 0) {
                        set_error(error, error_size,
                                  "duplicate source record");
                        return 0;
                    }
                }
            }
            snprintf(out->record_name[out->record_count],
                     sizeof(out->record_name[0]), "%s", tokens[1]);
            snprintf(out->record_digest[out->record_count],
                     sizeof(out->record_digest[0]), "%s", tokens[2]);
            out->record_count++;
            continue;
        }

        if (strcmp(tokens[0], "corpus") == 0) {
            if (corpus_seen) {
                set_error(error, error_size, "duplicate corpus line");
                return 0;
            }
            if (out->record_count != declared_files) {
                set_error(error, error_size,
                          "corpus before all declared records");
                return 0;
            }
            if (token_count != 2 || !is_digest(tokens[1])) {
                set_error(error, error_size, "field=corpus (malformed)");
                return 0;
            }
            corpus_seen = 1;
            snprintf(out->corpus, sizeof(out->corpus), "%s", tokens[1]);
            continue;
        }

        if (kind == CDC_MANIFEST_BUNDLE &&
            strcmp(tokens[0], "artifact") == 0) {
            if (!corpus_seen) {
                set_error(error, error_size, "artifact before corpus");
                return 0;
            }
            if (artifact_seen) {
                set_error(error, error_size, "duplicate artifact line");
                return 0;
            }
            if (token_count != 2 || !is_digest(tokens[1])) {
                set_error(error, error_size, "field=artifact (malformed)");
                return 0;
            }
            artifact_seen = 1;
            snprintf(out->artifact, sizeof(out->artifact), "%s", tokens[1]);
            continue;
        }

        {
            char what[64];
            snprintf(what, sizeof(what), "unknown line (line %d)",
                     line_number);
            set_error(error, error_size, what);
            return 0;
        }
    }

    if (!header_seen) {
        set_error(error, error_size, "missing header");
        return 0;
    }
    if (out->record_count != declared_files) {
        set_error(error, error_size, "fewer records than files= declares");
        return 0;
    }
    if (!corpus_seen) {
        set_error(error, error_size, "missing corpus line");
        return 0;
    }
    if (kind == CDC_MANIFEST_BUNDLE && !artifact_seen) {
        set_error(error, error_size, "missing artifact line");
        return 0;
    }
    /* Trailer position is enforced by construction: corpus rejects late
     * records, artifact requires corpus, and any line AFTER the trailer
     * would have to be a record (refused above), a duplicate corpus/
     * artifact (refused), or unknown (refused). */
    set_error(error, error_size, "ok");
    return 1;
}

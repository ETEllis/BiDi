#include "cdc_source.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cdc_source_fail(const char *message) {
    fprintf(stderr, "cdc-source: %s\n", message);
    exit(1);
}

int cdc_starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

void cdc_trim(char *s) {
    size_t n;
    char *p = s;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
    n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

void cdc_trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

void cdc_strip_comment(char *s) {
    char *hash = strchr(s, '#');
    if (hash) {
        *hash = '\0';
    }
    cdc_trim(s);
}

void cdc_first_token_after(const char *line, const char *prefix, char *out, size_t out_size) {
    const char *p = line + strlen(prefix);
    size_t i = 0;
    while (*p && !isspace((unsigned char)*p)) {
        if (i + 1 >= out_size) {
            cdc_source_fail("token too long");
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
}

/* Reads the first `key=` attribute on the line.
 *
 * The key must begin a TOKEN: at the start of the line, or immediately
 * after whitespace. Without that boundary this matched any substring, so
 * reading `gain` from
 *
 *     field f1 action-gain=9.0 gain=1.0 dt=0.125
 *
 * returned 9.0 — the tail of a DIFFERENT attribute whose name happens to
 * end in the key. The value was confidently wrong rather than missing, so
 * nothing downstream could notice. The current corpus does not trip it
 * (the frontend differential reports collision=0), but any source that
 * declares an attribute whose name ends with another attribute's name
 * would silently mis-read it.
 *
 * Value extraction is unchanged — raw, up to whitespace, quotes included —
 * because the runtime's consumers depend on that exact behavior and the
 * grammar-1 frontend deliberately differs there (see the `quoting`
 * divergence class in attr-parity). This repairs the key match only. */
int cdc_read_attr(const char *line, const char *key, char *out, size_t out_size) {
    size_t key_len = strlen(key);
    const char *p = line;
    size_t i = 0;

    for (;;) {
        p = strstr(p, key);
        if (!p) {
            return 0;
        }
        if ((p == line || isspace((unsigned char)p[-1])) &&
            p[key_len] == '=') {
            break; /* a whole token, not the tail of a longer name */
        }
        p += key_len;
    }
    p += key_len + 1;
    while (*p && !isspace((unsigned char)*p)) {
        if (i + 1 >= out_size) {
            cdc_source_fail("attribute too long");
        }
        out[i++] = *p++;
    }
    out[i] = '\0';
    return 1;
}

double cdc_read_double_attr(const char *line, const char *key, double fallback) {
    char value[64];
    if (!cdc_read_attr(line, key, value, sizeof(value))) {
        return fallback;
    }
    return atof(value);
}

int cdc_read_int_attr(const char *line, const char *key, int fallback) {
    char value[64];
    if (!cdc_read_attr(line, key, value, sizeof(value))) {
        return fallback;
    }
    return atoi(value);
}

void cdc_copy_attr(const char *line, const char *key, char *out, size_t out_size, const char *fallback) {
    if (!cdc_read_attr(line, key, out, out_size)) {
        snprintf(out, out_size, "%s", fallback);
    }
}

int cdc_close_enough(double actual, double expected, double tolerance) {
    return fabs(actual - expected) <= tolerance;
}

void cdc_expect_string(const char *actual, const char *expected, const char *message) {
    if (strcmp(actual, expected) != 0) {
        cdc_source_fail(message);
    }
}

void cdc_expect_int(int actual, int expected, const char *message) {
    if (actual != expected) {
        cdc_source_fail(message);
    }
}

void cdc_expect_double(double actual, double expected, double tolerance, const char *message) {
    if (!cdc_close_enough(actual, expected, tolerance)) {
        cdc_source_fail(message);
    }
}

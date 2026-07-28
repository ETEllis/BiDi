/* Strict canonical manifest parser — the ONE reader for both toolchain
 * manifest formats (second 2026-07-28 review, finding 3):
 *
 *   bundle   cdc-bundle v=1 grammar=1 abi=<maj>.<min> files=<n>
 *                statements=<n> checks=<n>
 *            source <base> <blake3:64-hex>      x n
 *            corpus <blake3:64-hex>
 *            artifact <blake3:64-hex>
 *
 *   package  cdc-package v=1 name=<name> files=<n>
 *            member <base> <blake3:64-hex>      x n
 *            corpus <blake3:64-hex>
 *
 * The grammar is CLOSED. Exactly one header, and it is line 1. Header
 * keys appear in exactly the order above, all present, none repeated,
 * none unknown. v must be 1; a bundle's grammar must be 1 and its abi
 * must equal the linked ABI; a package's name must equal the expected
 * package name. files= must equal the record count. Records are followed
 * by exactly one corpus line (and, for bundles, exactly one artifact
 * line) and then end-of-file: no blank lines, no unknown lines, no
 * trailing garbage, and every line newline-terminated. Record names are
 * plain `.cdc` basenames; package members must be strictly ascending
 * (installs emit them sorted). Digests are `blake3:` + 64 lowercase hex.
 *
 * Before this parser, headers were "format only": `grammar=999
 * abi=999.0` was accepted, a REMOVED header was accepted, and a package
 * manifest claiming a different name and member count executed. A
 * manifest is a contract; a reader that skips its binding fields is not
 * reading it. cdc build (emit + --check), cdc install (emit +
 * installed-state read), and cdc x all parse through this one function;
 * emitters self-check their own output with it before writing. */
#ifndef CDC_MANIFEST_H
#define CDC_MANIFEST_H

#include <stddef.h>
#include <stdint.h>

#define CDC_MANIFEST_MAX_RECORDS 256
#define CDC_MANIFEST_NAME_MAX 512
#define CDC_MANIFEST_DIGEST_MAX 96

typedef enum {
    CDC_MANIFEST_BUNDLE = 1,
    CDC_MANIFEST_PACKAGE = 2
} cdc_manifest_kind;

typedef struct {
    cdc_manifest_kind kind;
    long files;
    long statements; /* bundle only */
    long checks;     /* bundle only */
    unsigned grammar; /* bundle only */
    unsigned abi_major, abi_minor; /* bundle only */
    char name[CDC_MANIFEST_NAME_MAX]; /* package only */
    char record_name[CDC_MANIFEST_MAX_RECORDS][CDC_MANIFEST_NAME_MAX];
    char record_digest[CDC_MANIFEST_MAX_RECORDS][CDC_MANIFEST_DIGEST_MAX];
    int record_count;
    char corpus[CDC_MANIFEST_DIGEST_MAX];
    char artifact[CDC_MANIFEST_DIGEST_MAX]; /* bundle only */
} cdc_manifest;

/* Strict parse of `text` (length bytes, NUL-terminated) as `kind`.
 * expected_abi is the packed ABI version a bundle must carry (pass
 * cdc_abi_version(); ignored for packages). expected_name is the package
 * name the manifest must claim (required for packages; ignored for
 * bundles). Returns 1 and fills *out on success; returns 0 and writes a
 * typed reason naming the offending field or line into `error`
 * otherwise. Nothing is trusted from a manifest this function refused. */
int cdc_manifest_parse(const char *text, size_t length,
                       cdc_manifest_kind kind, uint32_t expected_abi,
                       const char *expected_name, cdc_manifest *out,
                       char *error, size_t error_size);

#endif

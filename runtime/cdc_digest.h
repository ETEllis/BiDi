#ifndef CDC_DIGEST_H
#define CDC_DIGEST_H

#include <stddef.h>
#include <stdint.h>

#include "cdc_blake3.h"

/* Canonical content digest: BLAKE3, 32 bytes (Amendment A3). This closes
 * the DECISIONS D2 interim-digest gate — every identity this module
 * produces is now the canonical algorithm, not an interim stand-in, and
 * digests are labeled "blake3:" so the algorithm is always explicit in
 * evidence.
 *
 * The implementation is the vendored dependency-free BLAKE3 in
 * runtime/cdc_blake3.c, verified against the reference vectors in
 * tests/fixtures/digest/blake3_vectors.txt (single-block, block-boundary,
 * chunk-boundary, and multi-level tree inputs, checked under plain and
 * sanitizer builds).
 *
 * Ed25519 signing of release/package/replica/authority statements over
 * these digests remains queued (see RESUME_HERE); nothing in the toolchain
 * claims signature verification today. */

enum { CDC_DIGEST_SIZE = CDC_BLAKE3_OUT_LEN };

typedef struct {
    cdc_blake3_hasher hasher;
} cdc_digest_ctx;

void cdc_digest_init(cdc_digest_ctx *ctx);
void cdc_digest_update(cdc_digest_ctx *ctx, const void *data, size_t size);
void cdc_digest_final(cdc_digest_ctx *ctx, uint8_t out[CDC_DIGEST_SIZE]);

/* One-shot convenience. */
void cdc_digest(const void *data, size_t size,
                uint8_t out[CDC_DIGEST_SIZE]);

/* Renders "blake3:<64 hex>" into out (needs >= 72 bytes). */
void cdc_digest_hex(const uint8_t digest[CDC_DIGEST_SIZE], char *out,
                    size_t out_size);

/* Digest a whole file; returns 0 on read failure. */
int cdc_digest_file(const char *path, uint8_t out[CDC_DIGEST_SIZE]);

#endif

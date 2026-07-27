#ifndef CDC_DIGEST_H
#define CDC_DIGEST_H

#include <stddef.h>
#include <stdint.h>

/* INTERIM content digest (DECISIONS D2): SHA-256 until the vendored BLAKE3
 * lands. Every digest this module produces is an interim identity; records
 * carrying them are re-digested when BLAKE3 arrives, and no public claim
 * may rest on an interim digest. The store labels its digests "sha256:" so
 * the algorithm is always explicit in evidence. */

enum { CDC_DIGEST_SIZE = 32 };

typedef struct {
    uint32_t state[8];
    uint64_t length;
    uint8_t buffer[64];
    size_t buffered;
} cdc_digest_ctx;

void cdc_digest_init(cdc_digest_ctx *ctx);
void cdc_digest_update(cdc_digest_ctx *ctx, const void *data, size_t size);
void cdc_digest_final(cdc_digest_ctx *ctx, uint8_t out[CDC_DIGEST_SIZE]);

/* One-shot convenience. */
void cdc_digest(const void *data, size_t size,
                uint8_t out[CDC_DIGEST_SIZE]);

/* Renders "sha256:<64 hex>" into out (needs >= 72 bytes). */
void cdc_digest_hex(const uint8_t digest[CDC_DIGEST_SIZE], char *out,
                    size_t out_size);

#endif

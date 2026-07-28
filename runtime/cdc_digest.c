/* Canonical digest surface over the vendored BLAKE3 (see cdc_digest.h). */
#include "cdc_digest.h"

#include <stdio.h>
#include <string.h>

void cdc_digest_init(cdc_digest_ctx *ctx) {
    cdc_blake3_init(&ctx->hasher);
}

void cdc_digest_update(cdc_digest_ctx *ctx, const void *data, size_t size) {
    cdc_blake3_update(&ctx->hasher, data, size);
}

void cdc_digest_final(cdc_digest_ctx *ctx, uint8_t out[CDC_DIGEST_SIZE]) {
    cdc_blake3_final(&ctx->hasher, out);
}

void cdc_digest(const void *data, size_t size,
                uint8_t out[CDC_DIGEST_SIZE]) {
    cdc_digest_ctx ctx;
    cdc_digest_init(&ctx);
    cdc_digest_update(&ctx, data, size);
    cdc_digest_final(&ctx, out);
}

void cdc_digest_hex(const uint8_t digest[CDC_DIGEST_SIZE], char *out,
                    size_t out_size) {
    static const char HEX[] = "0123456789abcdef";
    size_t i;
    if (out_size < 7 + CDC_DIGEST_SIZE * 2 + 1) {
        if (out_size > 0) {
            out[0] = '\0';
        }
        return;
    }
    memcpy(out, "blake3:", 7);
    for (i = 0; i < CDC_DIGEST_SIZE; i++) {
        out[7 + i * 2] = HEX[digest[i] >> 4];
        out[7 + i * 2 + 1] = HEX[digest[i] & 0x0f];
    }
    out[7 + CDC_DIGEST_SIZE * 2] = '\0';
}

int cdc_digest_file(const char *path, uint8_t out[CDC_DIGEST_SIZE]) {
    FILE *fp = fopen(path, "rb");
    cdc_digest_ctx ctx;
    uint8_t buffer[8192];
    size_t got;

    if (!fp) {
        return 0;
    }
    cdc_digest_init(&ctx);
    while ((got = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        cdc_digest_update(&ctx, buffer, got);
    }
    if (ferror(fp)) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    cdc_digest_final(&ctx, out);
    return 1;
}

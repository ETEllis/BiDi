/* SHA-256 (FIPS 180-4), compact public-domain-style implementation.
 * INTERIM digest per DECISIONS D2 — see cdc_digest.h. */
#include "cdc_digest.h"

#include <stdio.h>
#include <string.h>

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void compress(cdc_digest_ctx *ctx, const uint8_t block[64]) {
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i;
    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               (uint32_t)block[i * 4 + 3];
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^
                      (w[i - 15] >> 3);
        uint32_t s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^
                      (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    for (i = 0; i < 64; i++) {
        uint32_t s1 = ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + s1 + ch + K[i] + w[i];
        uint32_t s0 = ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void cdc_digest_init(cdc_digest_ctx *ctx) {
    static const uint32_t INIT[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    memcpy(ctx->state, INIT, sizeof(INIT));
    ctx->length = 0;
    ctx->buffered = 0;
}

void cdc_digest_update(cdc_digest_ctx *ctx, const void *data, size_t size) {
    const uint8_t *p = data;
    ctx->length += size;
    while (size > 0) {
        size_t take = 64 - ctx->buffered;
        if (take > size) {
            take = size;
        }
        memcpy(ctx->buffer + ctx->buffered, p, take);
        ctx->buffered += take;
        p += take;
        size -= take;
        if (ctx->buffered == 64) {
            compress(ctx, ctx->buffer);
            ctx->buffered = 0;
        }
    }
}

void cdc_digest_final(cdc_digest_ctx *ctx, uint8_t out[CDC_DIGEST_SIZE]) {
    uint64_t bits = ctx->length * 8;
    uint8_t pad = 0x80;
    uint8_t zero = 0;
    uint8_t len_bytes[8];
    int i;
    cdc_digest_update(ctx, &pad, 1);
    while (ctx->buffered != 56) {
        ctx->length--; /* padding is not message length */
        cdc_digest_update(ctx, &zero, 1);
    }
    ctx->length--;
    for (i = 0; i < 8; i++) {
        len_bytes[i] = (uint8_t)(bits >> (56 - i * 8));
    }
    ctx->length -= 8;
    cdc_digest_update(ctx, len_bytes, 8);
    for (i = 0; i < 8; i++) {
        out[i * 4] = (uint8_t)(ctx->state[i] >> 24);
        out[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        out[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        out[i * 4 + 3] = (uint8_t)ctx->state[i];
    }
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
    if (out_size < 8 + CDC_DIGEST_SIZE * 2 + 1) {
        if (out_size > 0) {
            out[0] = '\0';
        }
        return;
    }
    memcpy(out, "sha256:", 7);
    for (i = 0; i < CDC_DIGEST_SIZE; i++) {
        out[7 + i * 2] = HEX[digest[i] >> 4];
        out[7 + i * 2 + 1] = HEX[digest[i] & 0x0f];
    }
    out[7 + CDC_DIGEST_SIZE * 2] = '\0';
}

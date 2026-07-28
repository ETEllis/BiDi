#ifndef CDC_BLAKE3_H
#define CDC_BLAKE3_H

#include <stddef.h>
#include <stdint.h>

/* Portable BLAKE3 (hash mode, 32-byte output) — the canonical content
 * digest required by Amendment A3, closing the DECISIONS D2 interim-digest
 * gate. Vendored as a single dependency-free translation unit so the
 * runtime keeps its no-external-dependency property.
 *
 * Scope: unkeyed hash mode with 32-byte output. Keyed hashing, key
 * derivation, and extended (XOF) output are NOT implemented — they are not
 * needed by the store or the manifest surface, and an unimplemented mode is
 * absent rather than wrong. Streaming update is supported.
 *
 * Verification: runtime/cdc_frontend_check.c `digest-vectors` checks this
 * implementation against tests/fixtures/digest/blake3_vectors.txt, which
 * covers single-block, block-boundary, chunk-boundary, and multi-level
 * tree inputs. */

enum { CDC_BLAKE3_OUT_LEN = 32 };

typedef struct {
    uint32_t cv[8];
    uint64_t chunk_counter;
    uint8_t block[64];
    uint8_t block_len;
    uint8_t blocks_compressed;
} cdc_blake3_chunk;

typedef struct {
    cdc_blake3_chunk chunk;
    uint32_t cv_stack[54][8];
    uint8_t cv_stack_len;
} cdc_blake3_hasher;

void cdc_blake3_init(cdc_blake3_hasher *hasher);
void cdc_blake3_update(cdc_blake3_hasher *hasher, const void *input,
                       size_t len);
void cdc_blake3_final(const cdc_blake3_hasher *hasher,
                      uint8_t out[CDC_BLAKE3_OUT_LEN]);

#endif

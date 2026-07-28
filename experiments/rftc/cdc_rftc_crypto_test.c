#include "../../runtime/cdc_blake3.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    size_t length;
    const char *hex;
} vector;

static const vector VECTORS[] = {
    {0, "92b2b75604ed3c761f9d6f62392c8a9227ad0ea3f09573e783f1498a4ed60d26"},
    {3, "39e67b76b5a007d4921969779fe666da67b5213b096084ab674742f0d5ec62b9"},
    {64, "ba8ced36f327700d213f120b1a207a3b8c04330528586f414d09f2f7d9ccb7e6"},
    {1024, "75c46f6f3d9eb4f55ecaaee480db732e6c2105546f1e675003687c31719c7ba4"},
    {1025, "357dc55de0c7e382c900fd6e320acc04146be01db6a8ce7210b7189bd664ea69"},
    {2048, "879cf1fa2ea0e79126cb1063617a05b6ad9d0b696d0d757cf053439f60a99dd1"},
    {8192, "dc9637c8845a770b4cbf76b8daec0eebf7dc2eac11498517f08d44c8fc00d58a"},
    {16384, "9e9fc4eb7cf081ea7c47d1807790ed211bfec56aa25bb7037784c13c4b707b0d"},
};

static void hex_encode(const uint8_t bytes[CDC_BLAKE3_OUT_LEN],
                       char out[CDC_BLAKE3_OUT_LEN * 2 + 1]) {
    static const char HEX[] = "0123456789abcdef";
    size_t i;
    for (i = 0; i < CDC_BLAKE3_OUT_LEN; i++) {
        out[i * 2] = HEX[bytes[i] >> 4];
        out[i * 2 + 1] = HEX[bytes[i] & 15];
    }
    out[CDC_BLAKE3_OUT_LEN * 2] = '\0';
}

int main(void) {
    static const uint8_t key[] =
        "whats the Elvish word for friend";
    uint8_t input[16384];
    size_t i;

    for (i = 0; i < sizeof(input); i++) {
        input[i] = (uint8_t)(i % 251);
    }
    for (i = 0; i < sizeof(VECTORS) / sizeof(VECTORS[0]); i++) {
        cdc_blake3_hasher hasher;
        uint8_t digest[CDC_BLAKE3_OUT_LEN];
        char hex[CDC_BLAKE3_OUT_LEN * 2 + 1];
        size_t offset = 0;
        cdc_blake3_init_keyed(&hasher, key);
        while (offset < VECTORS[i].length) {
            size_t take = VECTORS[i].length - offset;
            if (take > 137) {
                take = 137;
            }
            cdc_blake3_update(&hasher, input + offset, take);
            offset += take;
        }
        cdc_blake3_final(&hasher, digest);
        hex_encode(digest, hex);
        if (strcmp(hex, VECTORS[i].hex) != 0) {
            fprintf(stderr, "keyed BLAKE3 mismatch length=%zu\n",
                    VECTORS[i].length);
            return 1;
        }
    }
    puts("RFTC keyed BLAKE3: official vectors PASS");
    return 0;
}

/* cdc — unified toolchain driver (Amendment A9: dispatcher + command
 * modules). This layer consumes ONLY the stable ABI (cdc_abi.h); it never
 * includes internal frontend or runtime headers. Commands land in their
 * amendment phases; unimplemented commands fail closed with a typed notice
 * and exit code 3 rather than pretending. */
#include <stdio.h>
#include <string.h>

#include "../cdc_abi.h"
#include "cmd_test.h"
#include "cmd_verify.h"

/* Transitional passthrough entry points (gate CT2): the legacy runtimes'
 * guarded mains, linked with CDC_NATIVE_NO_MAIN / CDC_BRIDGE_NO_MAIN so the
 * unified binary reproduces the standalone CLIs byte-identically. They
 * retire when the modes route through the ABI execution surface. */
int cdc_native_main(int argc, char **argv);
int cdc_bridge_main(int argc, char **argv);

static int is_native_verb(const char *verb) {
    static const char *const VERBS[] = {
        "run",     "compile", "interpret", "prove",   "surface",
        "council", "evolve",  "universal", "persist", "replay",
        "fused",
    };
    size_t i;
    for (i = 0; i < sizeof(VERBS) / sizeof(VERBS[0]); i++) {
        if (strcmp(verb, VERBS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int cmd_version(void) {
    uint32_t version = cdc_abi_version();
    printf("cdc abi=%u.%u grammar=1\n", version >> 16, version & 0xffffu);
    return 0;
}

static int cmd_unavailable(const char *name, const char *phase) {
    fprintf(stderr, "cdc %s: lands with %s (see CDC_TOOLCHAIN_PLAN.md)\n",
            name, phase);
    return 3;
}

static void usage(FILE *stream) {
    fputs("usage: cdc <command> [args]\n"
          "commands: verify version run test install build x\n",
          stream);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return 2;
    }
    if (strcmp(argv[1], "verify") == 0) {
        return cdc_cmd_verify(argc - 2, argv + 2);
    }
    if (strcmp(argv[1], "version") == 0) {
        return cmd_version();
    }
    if (strcmp(argv[1], "run") == 0 && argc == 3) {
        /* Phase C: cdc run is the fused single-process executor — one
         * parse, one live Runtime, every declared stage family. The
         * legacy single-mode behavior remains available as the explicit
         * mode verbs below. */
        char *fused_argv[4];
        fused_argv[0] = argv[0];
        fused_argv[1] = (char *)"fused";
        fused_argv[2] = argv[2];
        fused_argv[3] = NULL;
        return cdc_native_main(3, fused_argv);
    }
    if (is_native_verb(argv[1]) && argc >= 3) {
        return cdc_native_main(argc, argv);
    }
    if (strcmp(argv[1], "bridge") == 0 && argc >= 3) {
        return cdc_bridge_main(argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "test") == 0) {
        return cdc_cmd_test(argc - 2, argv + 2);
    }
    if (is_native_verb(argv[1]) || strcmp(argv[1], "bridge") == 0) {
        usage(stderr);
        return 2;
    }
    if (strcmp(argv[1], "install") == 0 || strcmp(argv[1], "build") == 0 ||
        strcmp(argv[1], "x") == 0) {
        return cmd_unavailable(argv[1], "Phase I");
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "help") == 0) {
        usage(stdout);
        return 0;
    }
    fprintf(stderr, "cdc: unknown command '%s'\n", argv[1]);
    usage(stderr);
    return 2;
}

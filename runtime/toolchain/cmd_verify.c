#include "cmd_verify.h"

#include <stdio.h>
#include <string.h>

#include "../cdc_abi.h"

static int verify_parse(int argc, char **argv) {
    int i;
    size_t statements = 0;
    for (i = 0; i < argc; i++) {
        cdc_program *program = NULL;
        cdc_status status = cdc_program_parse(argv[i], NULL, 0, &program);
        if (status == CDC_ERR_PARSE) {
            cdc_result *result = NULL;
            fprintf(stderr, "cdc verify: %s rejected\n", argv[i]);
            if (cdc_program_diagnostics(program, &result) == CDC_OK) {
                char *json = NULL;
                size_t length = 0;
                if (cdc_result_serialize(result, &json, &length) == CDC_OK) {
                    fwrite(json, 1, length, stderr);
                    fputc('\n', stderr);
                    cdc_bytes_free(json);
                }
                cdc_result_destroy(result);
            }
            cdc_program_destroy(program);
            return 1;
        }
        if (status != CDC_OK) {
            fprintf(stderr, "cdc verify: %s: %s\n", argv[i],
                    cdc_status_name(status));
            cdc_program_destroy(program);
            return 1;
        }
        statements += cdc_program_statement_count(program);
        cdc_program_destroy(program);
    }
    printf("cdc verify parse ok files=%d statements=%zu\n", argc,
           statements);
    return 0;
}

/* --contract: the bootloader-parity contract report over a file set.
 * Output is byte-identical to `python3 cdc_boot.py <files>` for valid
 * corpora (gate toolchain-verify-parity). */
static int verify_contract(int argc, char **argv) {
    cdc_runtime *runtime = NULL;
    cdc_result *result = NULL;
    int i;
    int rc = 1;

    if (cdc_runtime_create(&runtime) != CDC_OK) {
        fprintf(stderr, "cdc verify: runtime creation failed\n");
        return 1;
    }
    for (i = 0; i < argc; i++) {
        cdc_program *program = NULL;
        cdc_status status = cdc_program_parse(argv[i], NULL, 0, &program);
        if (status != CDC_OK) {
            fprintf(stderr, "cdc verify: %s: %s\n", argv[i],
                    cdc_status_name(status));
            cdc_program_destroy(program);
            cdc_runtime_destroy(runtime);
            return 1;
        }
        status = cdc_runtime_load(runtime, program);
        if (status != CDC_OK) {
            fprintf(stderr, "cdc verify: %s: load %s\n", argv[i],
                    cdc_status_name(status));
            cdc_program_destroy(program);
            cdc_runtime_destroy(runtime);
            return 1;
        }
    }
    if (cdc_runtime_verify(runtime, NULL, &result) != CDC_OK) {
        fprintf(stderr, "cdc verify: contract evaluation failed\n");
        cdc_runtime_destroy(runtime);
        return 1;
    }
    fputs(cdc_result_text(result), stdout);
    rc = cdc_result_error_count(result) == 0 ? 0 : 1;
    cdc_result_destroy(result);
    cdc_runtime_destroy(runtime);
    return rc;
}

int cdc_cmd_verify(int argc, char **argv) {
    if (argc >= 1 && strcmp(argv[0], "--parse") == 0) {
        if (argc < 2) {
            fprintf(stderr, "cdc verify --parse: no files given\n");
            return 2;
        }
        return verify_parse(argc - 1, argv + 1);
    }
    if (argc >= 1 && strcmp(argv[0], "--contract") == 0) {
        if (argc < 2) {
            fprintf(stderr, "cdc verify --contract: no files given\n");
            return 2;
        }
        return verify_contract(argc - 1, argv + 1);
    }
    fprintf(stderr,
            "cdc verify: available: cdc verify --parse <files...> | "
            "cdc verify --contract <files...>\n");
    return 3;
}

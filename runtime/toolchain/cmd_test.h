#ifndef CDC_TOOLCHAIN_CMD_TEST_H
#define CDC_TOOLCHAIN_CMD_TEST_H

/* `cdc test` command module (Phase C, gate CT3 seed): typed A7 policy over
 * per-file mode execution — commit/hold/nest/fail counted separately,
 * holds matched against declared expect-status=held jobs, unexpected holds
 * fail --gate, merged totals forbidden. */
int cdc_cmd_test(int argc, char **argv);

#endif

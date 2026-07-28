#ifndef CDC_TOOLCHAIN_CMD_VERIFY_H
#define CDC_TOOLCHAIN_CMD_VERIFY_H

/* `cdc verify` command module (Amendment A9 layout).
 * At this stage only the parse surface is live: `cdc verify --parse
 * <files...>` parses every file through the stable ABI and fails on any
 * diagnostic error. Full contract evaluation (registry closure and expect
 * evaluation, replacing the bootloader through gate toolchain-verify-parity)
 * lands with Phase C. */
int cdc_cmd_verify(int argc, char **argv);

#endif

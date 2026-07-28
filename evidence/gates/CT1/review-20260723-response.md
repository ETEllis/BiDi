# Review response — CODEX PR#3 adversarial gate review (2026-07-23)

| item | repair | permanent test |
|---|---|---|
| Defect 1 heap overflow | two-pass render, exact alloc, in-bounds strip (cdc_abi.c cdc_program_diagnostics) | verify.sh: abi-diag >512B path (plain + ASan), oom-abi sweep (plain + ASan), overflow fails closed |
| Defect 2 empty-program acceptance | O_NONBLOCK open + fstat S_ISREG + ferror + 64MiB bound (cdc_parser.c); CDC001-004 -> CDC_ERR_IO (cdc_abi.c) | verify.sh: dir//dev/null/FIFO/unreadable -> io, no handle; io-mid-read CDC003; empty file valid; cdc verify --parse . fails |
| C1 stale RESUME_HERE | rewritten from current head with exact next sequence | n/a (doc) |
| C2 identity branch | D8: statements scoped to freeze; merge into PR branch as combined tip + combined gate | combined-tree verify.sh run recorded below after merge |
| C3 inexact statement gate | derived-exact gate: records + end lines | verify.sh grep statements=EXPECTED$ |
| Gate language | BUILD_STATE 'Exact gate states' section | n/a (doc) |

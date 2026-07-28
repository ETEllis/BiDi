# RESUME_HERE — BiDi/CDC lane: baton complete in this repository

Updated 2026-07-28. **Every component executable in this repository is
COMPLETE and gated.** What remains is queued-with-reason, blocked on
external access, or awaiting one operator decision — all listed below with
their exact blockers. Read in order: `CDC_TOOLCHAIN_PLAN.md` (with
Amendment Record) → the two operator-held documents (2026-07-22 amendment;
2026-07-23 adversarial review) → `CDC_MEMORY_MANIFOLD_INTERFACE.md` →
`docs/build/BUILD_STATE.md` → `docs/build/DECISIONS.md` (D1–D28).

## Current head (2026-07-28)

- `main` = `3e851ff` — **PR #3 merged** under the baton's Section 1
  authorization. PR #3 is finished and must not be reused.
- Work branch `claude/bun-equivalent-build-plan-lxe772`, open as **draft
  PR #4**, carries the whole post-merge arc. Rather than pinning SHAs that
  drift, the arc is indexed by its decision record: BLAKE3 + product
  surfaces + store protocol (D13–D14), persistence as a language form
  (D15), the 2026-07-28 review repair — store generations, real
  interprocess serialization, head-bound provenance, required macOS CI
  lane, claims withdrawn (D16), CT2/CT3/CT0 closure — typed receipts,
  ordered vectors with a chained trace digest, lifecycle contract,
  whole-binary sanitizers, closure witnesses, corpus-bound verdicts
  (D17–D21), the deletion gates through the scanner (D22–D27), and
  Phase I — `cdc build` / `cdc install` / `cdc x` (D28).
- Full `./scripts/verify.sh` green locally at this head, including the
  Phase I gate section and the sanitized Phase I sweep.
- The 2026-07-28 REQUEST CHANGES review pinned `1ea1ddd`; that head was
  never merged and every finding is repaired with permanent
  counterexamples (D16 records the withdrawn claims). **Do not merge PR #4
  without operator sign-off.**

## COMPLETE in this repository (all hard-gated in `./scripts/verify.sh`)

1. **Canonical frontend + ABI** (grammar 1, ABI 1.3), differential oracle,
   adversarial/allocator/sanitizer sweeps (D5, D7, D13, D17).
2. **Unified `cdc` driver**: verify/run/test/build/install/x + legacy verb
   passthrough, byte-identical to the standalone binaries.
3. **Durable store**: sealed transactions, typed recovery, generations
   with atomic activation, triple fence token, attest-from-byte-0;
   in-process + SIGKILL crash matrices; 8-boundary transition matrix;
   fresh-after-crash (D10–D16, D27); same-application coordination — a
   process-local refcounted coordination object (one mutex + one fcntl
   descriptor per store per process) serializing open recovery, commit,
   snapshot, compact, and reset across handles AND processes, gated by
   the five-check `store-samep` suite with a permanent per-handle probe
   build required to fail 5/5, plus ASan/UBSan and a guarded TSan lane
   (D29).
4. **Persistence as a language form**: `store`/`persist` directives gated
   by the same commit barrier; durable/replay observed, never declared
   (D15).
5. **Typed effect receipts, parity vectors, lifecycle contract, closure
   witnesses, corpus-bound verdicts, same-machine reproducible builds**
   (D17–D21). CT0, CT2, CT3 CLOSED.
6. **Deletion gates**: legacy scanner deleted after a real defect fix and
   a pinned migration precondition; both runtimes migrated to `cdc_stmt`
   accessors with differential evidence; `attr-parity` retired;
   `attr-boundary` repointed; `cdc_boot.py --dump` deliberately kept as
   the last independent oracle (D22–D26).
7. **Phase I**: `cdc build` proof-carrying bundles (deterministic,
   cross-checked, refusal-first); `cdc install` crash-durable journaled
   installs on the cdc_store substrate (four-boundary kill matrix,
   held-writes-nothing, zero-evidence refusal, idempotent/divergent
   reinstalls, typed receipts); `cdc x` manifest-verified trusted-local
   execution (D28). Hardened by the second review round (D30):
   capture-once immutable member bytes, package-scoped install lock with
   deterministic concurrent-install gates, attempt-unique staging,
   checked final directory sync before `durable=1`, one strict shared
   manifest parser with a permanent field-mutation sweep, portable
   counts with the macOS CI lane running the full native suite, and the
   coordination registry's last-close window closed with its own probe
   build.

## QUEUED, with recorded reasons (not blockers, not claims)

- **Ed25519 keyed authentication + external anchor** — integrity tags are
  unkeyed (corruption detection, not forgery resistance); whole-file
  generation rollback is detectable only against an externally retained
  anchor. Stated in D16; queued behind CT5-adjacent key management.
- **CT5: sealed capability environment + hostile-package
  counterexamples** — until these pass, `cdc x` is TRUSTED-LOCAL-ONLY
  (no network, no registries, no archives; verification is drift/tamper
  detection, not a sandbox). Stated in code, docs, and matrix (D28).
- **`package.cdc` manifest layer, versioning, lockfile** — packages are
  plain directories installed by name; versioned coexistence arrives with
  the manifest layer (plan Amendment Record).
- **`cycles=N` per-cycle expectation families** — inline expect-* pins
  first-cycle state; a language-design item, not a runtime bug (D19 era).
- **Fuzzing beyond the deterministic corpus** — queued for CT1 full
  closure.

## AWAITING EDWARD (one decision; nothing proceeds without it)

- **`cdc_boot.py` deletion** — `docs/build/BOOTLOADER_DELETION_PROPOSAL.md`
  lays out freeze (A) / replace-then-delete (B) / delete-now (C) with the
  exact `kernel.cdc` gate diff. Until he chooses, the status quo IS
  option A: the bootloader stays, `python-files == 1` stays, and the
  toolchain-verify-parity gate keeps both honest. **Do not touch the
  kernel floor.**

## BLOCKED EXTERNAL (unchanged)

1. `ETEllis/Memory-Manifold` repository access — blocks Phases E–H. The
   interface contract v1.0.3 is the binding surface; the cdc_store sealed
   transaction + replay-digest model is the intended persistence core.
2. `ETEllis/GIST` access + the Superposition source bundle — blocks
   Phase K and GIST Track M.
3. A macOS host — the required `macos-14` CI lane is the Swift compiler
   of record for `ui/macos/CDCStudio`.
4. **PC6 stays HARD-PAUSED.** Nothing in this lane may resume it.

## Hard rules in force (unchanged)

Repair-and-continue; amendment + review govern (D1/D8). Every commit
green through `./scripts/verify.sh`; byte-identical outputs for legacy
paths until recorded removal gates. Unreadable/non-regular sources are
CDC_ERR_IO; diagnostics never truncate. Balanced ternary stays typed;
merged pass totals forbidden; durable mutation behind commit semantics;
single writer per worktree; interface versions change only by new digest;
no identifier reuse. No public claim rests on an interim digest. Always
run `./scripts/regen_provenance.sh` and re-stage before committing.

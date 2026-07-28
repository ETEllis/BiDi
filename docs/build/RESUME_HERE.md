# RESUME_HERE — BiDi/CDC lane: C3 execution baton

Updated 2026-07-28. **PR #8 is merged. ABI 1.5 local recursive execution and
authenticated exact-history restart are implemented on the active branch and
the rapid/sanitizer/concurrency and maximal formal gates are green. PR
promotion, exact-head CI, and actual cross-host reconciliation remain active
work.** Read in order:
`CDC_TOOLCHAIN_PLAN.md` (with
Amendment Record) → the two operator-held documents (2026-07-22 amendment;
2026-07-23 adversarial review) → `CDC_MEMORY_MANIFOLD_INTERFACE.md` →
`docs/build/BUILD_STATE.md` → `docs/build/DECISIONS.md` (D1–D39).

## Current head (2026-07-28)

- `main` = `79a508a` — **PR #8 merged** after both exact-head CI lanes passed.
- Work branch `codex/c3-recursive-cells` carries D37-D38 and ABI 1.5.
- The completed post-merge arc is indexed by its decision record: BLAKE3 + product
  surfaces + store protocol (D13–D14), persistence as a language form
  (D15), the 2026-07-28 review repair — store generations, real
  interprocess serialization, head-bound provenance, required macOS CI
  lane, claims withdrawn (D16), CT2/CT3/CT0 closure — typed receipts,
  ordered vectors with a chained trace digest, lifecycle contract,
  whole-binary sanitizers, closure witnesses, corpus-bound verdicts
  (D17–D21), the deletion gates through the scanner (D22–D27), Phase I —
  `cdc build` / `cdc install` / `cdc x` (D28), same-application store
  coordination (D29), the second review and Darwin reproducibility repairs
  (D30–D32), the frozen bootloader-oracle decision (D33), and the
   separately classified RFTC logical-cell crucible (D34), and the
   six-form/R1-R6 C3 language ingress (D35), and the authenticated
   authority/transport supervisor boundary (D36), and the sealed recursive
   scheduler plus authenticated restart journal (D37), the atomic
   content-bound frame-epoch/outcome-record repair (D38), and the expanded
   multiscale Relational Closure end-state/claim dependency graph (D39).
- `./scripts/verify_rftc.sh` is green on the active D38 source, including
  release, ASan/UBSan, and ThreadSanitizer lanes. Full
  `./scripts/verify.sh --require-formal` is green after staging every new
  source/evidence file and regenerating the head-bound provenance manifest.
- The 2026-07-28 REQUEST CHANGES review pinned `1ea1ddd`; that head was
  never merged and every finding is repaired with permanent
  counterexamples (D16 records the withdrawn claims).

## COMPLETE in this repository (all hard-gated in `./scripts/verify.sh`)

1. **Canonical frontend + ABI** (grammar 1, ABI 1.5), differential oracle,
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
8. **RFTC foundational crucible**: seven separately failing classical
   witnesses, deterministic and alternate-seed evidence, sanitizer
   execution, 256-seed rapid and 4,096-seed stress profiles, plus a
   self-contained evidence-bound UI (D34). This completes the entry gate,
   not the distributed six-form runtime described in
   `docs/rftc/RFTC_FULL_BUILD_SPEC.md`.
9. **RFTC C3 language ingress**: `frame`, `reduce`, `complex`, `topology`,
   `authority`, and `transport` parse through the canonical native frontend
   and frozen oracle under the explicit R1-R6 registry allocation; malformed
   and unknown forms fail closed (D35). PR #7 merged this ingress.
10. **C3 authenticated local control plane**: canonical MAC-bound wire
    envelopes, scoped/versioned authority leases, nonce/replay defense,
    partitions and causal ordering, and a serialized exactly-once supervisor
    admission surface are implemented behind ABI 1.4 (D36). The maximal
    formal gate includes cross-process wire, capacity failure, forged ticket,
    concurrent duplicate, every-byte wire mutation, sanitizer, and
    ThreadSanitizer counterexamples.
11. **C3 local recursive execution and restart**: immutable canonical frame
    seals, strict topology and witnessed sector/frame transitions,
    provenance-preserving cells, canonical sealed-forest scheduling, `CDCP`
    observation/witness payloads, typed terminal application rejection, and
    complete signed-envelope journaling/replay are implemented behind ABI 1.5
    (D37-D38). Frame epochs are atomic and predecessor-bound; composite
    receipts bind child identity; imported state is canonically revalidated;
    replay reapplies the configured peer policy; stale commands remain
    retryable; and terminal outcomes are durable. Release, ASan/UBSan, and
    ThreadSanitizer rapid lanes are green.
12. **Expanded end-state and claim graph**: the microscopic/quantum,
    distributed mesh, horizon-foundry, cosmological-atlas, biological
    observer/AI, and product/release lanes are frozen under one operator and
    separate C/QS/QH/R/Q/H/B/G evidence gates (D39). These are build and
    falsification obligations, not current scientific claims.

## QUEUED, with recorded reasons (not blockers, not claims)

- **RFTC multi-host C3 completion** — the bounded local execution kernel and
  durable restart boundary exist under D37. Next bind it to a real
  network-session driver, mixed-version/partition harness, cross-host
  reconciliation receipts, and operator surfaces. Until that evidence exists,
  artifacts may say "C3-capable local kernel" but not claim C3.
- **Ed25519 identity, key rotation, network session authentication, and
  external anchor** — D36 adds shared-key keyed-BLAKE3 authenticity to RFTC
  envelopes; it does not provide public-key identity, non-repudiation, mTLS,
  or whole-file rollback protection against an adversary controlling both the
  store and retained anchor. These remain CT5-adjacent key-management work.
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

## DECIDED BY EDWARD (2026-07-28)

- **Bootloader: Option A executed (D33).** `cdc_boot.py` is FROZEN as a
  CI-only differential oracle; the kernel floor is `python-files == 0`
  with the oracle exempt by name (rendered in every report);
  `bootloader minimal` still pins the raw file set to exactly
  `[cdc_boot.py]`; Gate 5 is closed on the runtime dependency. Deleting
  the oracle later requires a NEW operator decision.
- **PR #4 approved and MERGED** at pinned head `6784ab5` (merge commit
  `2947673`); both CI lanes green, independent macOS verification by the
  operator.

## BLOCKED EXTERNAL (unchanged)

1. `ETEllis/Memory-Manifold` repository access — blocks Phases E–H. The
   interface contract v1.3.0 is the binding surface; the cdc_store sealed
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

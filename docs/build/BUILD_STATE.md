# BUILD_STATE — BiDi/CDC lane

Updated at every accepted gate boundary. Companion files: `RESUME_HERE.md`
(exact continuation), `DECISIONS.md` (append-only decision record),
`../../evidence/gates/<gate-id>/` (evidence bundles).

## Current source identity

- repository: `ETEllis/BiDi-Coherence-Delta-Calculus` (GitHub remote)
- branch: `claude/bun-equivalent-build-plan-lxe772` (draft PR #3)
- baseline at Phase A freeze: `origin/main` = `8cfe48fdb71e53af78411471869c064e6c650c63`;
  work-branch HEAD entering Phase A = `99747e0a63ad14ad243934c122da73ec94a57940`
  (adds `CDC_TOOLCHAIN_PLAN.md`)
- environment: Claude Code remote container — Linux 6.18.5, cc (Ubuntu 13.3.0),
  Python 3.11.15, Lean pin `leanprover/lean4:4.31.0` (CI); full identity in
  `evidence/gates/CT0/toolchain.txt`
- tracked-file digests: `evidence/gates/CT0/source-identity.txt` (git blob ids)
  and `evidence/gates/CT0/sha256-manifest.txt` (interim SHA-256, see D2)

## Last completed phase and gate

- **Foundation merged (2026-07-28).** PR #3 merged at pinned head `82ab066`
  → merge commit `3e851ff` on `main`, after verifying remote head exactness,
  a completed/success required check, and clean mergeability. Branch
  restarted from the merged main per ADR D12; continuation is draft PR #4.

- **Canonical BLAKE3 — D2 CLOSED (`e4ba69b`).** Vendored dependency-free
  implementation; 31 reference vectors verified one-shot and through
  irregular streaming splits under plain and ASan builds; evidence
  re-digested by the implementation itself; interim sha256 manifest retained
  unmodified. All store suites green under the new identities.

- **Product surfaces landed (`41286c5`).** Canonical design tokens derived
  from the shipped identity system; macOS CDC Studio (SwiftUI, SwiftPM, zero
  dependencies, drives the real binary); self-contained web console whose
  embedded record byte-matches `demo/replay.json`; `scripts/verify_ui.sh`
  gating token parity, self-containment, and app structure. Swift compilation
  is an external macOS step (recorded boundary); the console was
  render-verified in Chromium with zero page errors and zero external
  requests.

- **2026-07-23 adversarial-review repairs: COMPLETE (this commit).** Both
  blocking defects fixed with permanent counterexamples hard-gated in
  `verify.sh`:
  - Defect 1 (diagnostic serialization heap overflow): two-pass render in
    `cdc_program_diagnostics` — size with `render(NULL,0)`, checked-add,
    exact allocation, in-bounds newline strip; no truncation. Tests:
    >512-byte-path rejected fixture serializes complete JSON containing the
    full path (also under ASan/UBSan); `oom-abi` allocation-failure sweep
    over the whole ABI diagnostic pipeline (leak-checked by the sanitizer
    binary); size arithmetic fails closed.
  - Defect 2 (unreadable/special paths accepted as empty programs):
    `cdc_unit_parse_file` now opens `O_RDONLY|O_NONBLOCK`, `fstat`s,
    rejects non-regular inputs by type before reading (CDC002), checks
    `ferror` on every short read (CDC003), enforces a 64 MiB source bound
    (CDC004); ABI maps CDC001–CDC004 to `CDC_ERR_IO`, never to memory.
    Tests: directory / `/dev/null` / FIFO (non-blocking) / unreadable file
    (non-root) → `io` with no handle; mid-read directory-fd stream →
    CDC003; zero-byte regular file specified and tested as a VALID empty
    unit; allocation failure stays `CDC_ERR_MEMORY`.
  - Review C3: the statement gate is now exact and derived
    (records + structural `end` lines = `statements`), recomputed from the
    corpus on every run.

- **2026-07-24 independent-review repairs (B1–B7): COMPLETE (this
  commit).** All seven merge blockers from the bb3531b review repaired
  and permanently gated (see DECISIONS D9 for the item-by-item record):
  the store integrity contract now fails closed on committed-prefix
  corruption with evidence preserved (6 byte-flip counterexamples, 2
  controls, plain + ASan); sequence and seal digests verified; hold
  authorization bound to executable statement identity (spoof fixture);
  zero-run gates fail; incomplete reducer families fail typed; the
  identity 3D verifier creates its log directory; counts are
  BSD-portable. Re-review gate items 2–7 are demonstrably green in
  verify.sh; item 1 (fresh macOS run) needs an operator-side or CI
  macOS lane (queued).

- **Phase D seed — durable store substrate (gates CT4/MM1): LIVE
  (previous commit).** `runtime/cdc_store.{h,c}` (full protocol declared; reference
  backend: append-only DATA+SEAL transaction log, digest-checked records,
  torn/unsealed-tail recovery by truncation, fsync of file AND directory,
  in-memory staging with commit/rollback) + `runtime/cdc_digest.{h,c}`
  (interim sha256 per D2, FIPS vectors self-tested). verify.sh gates:
  replay determinism across independent stores; typed statuses
  (ESTATE/EUNSUPPORTED fail closed for empty commits and
  snapshot/compact/fence); and the **crash matrix — injected failure at
  every one of the 7 commit write/flush/sync boundaries recovers to
  exactly the old (3) or new (4) sealed state, never partial** (plain +
  ASan). Open items and their order live in RESUME_HERE (BLAKE3 swap,
  snapshot/compact/fence, kill-based injection, .cdc-declared
  persistence jobs, BiDi-decision wiring).

- **Fused executor — cdc run (gate CT3): LIVE (previous commit).** One
  parse, one live Runtime, every declared stage family; universal
  closure path for universal-bearing sources; multi-stage chaining
  (council_bridge: council + evolution over one state, previously two
  invocations); fail-closed on no-stage sources; parity-gated against
  the native fused mode. cycles=N stays queued with its recorded reason
  (inline expectations pin first-cycle state).

- **Phase C seed — typed test runner (gate CT3): LIVE (earlier commit).**
  `cdc test [--gate] <files...>` (runtime/toolchain/cmd_test.c, ABI 1.2
  statement introspection): free discovery (every job carries inline
  expectations), per-file mode selection over the eight runtime families,
  forked-child execution of the linked runtime (no external process
  spawning), and the A7 typed policy — commit/hold/nest/fail counted
  SEPARATELY, every hold matched against a declared expect-status=held
  job, unexpected holds fail the gate even when the runtime exits 0
  (proven by the tracked silent_hold.cdc fixture: runtime green, gate
  red with expected=0 unexpected=1 fail=0). Executable-corpus gate:
  runs=23 commit=11 hold=5 (all expected) nest=10 fail=0, exact-gated in
  verify.sh. CT3 remaining: cancellation/budgets/deterministic-mode
  contract, per-check ordered vector export, fused `cdc run`.

- **Unified-driver passthrough parity: LIVE (previous commit).** The guarded
  runtime mains compile as `cdc_native_main` / `cdc_bridge_main` under
  their NO_MAIN macros (zero duplication; standalone CLIs untouched), and
  `build/cdc` links BOTH runtimes plus the ABI/registry stack into the
  one binary. verify.sh gates byte-identical stdout+stderr AND exit codes
  between `build/cdc <verb> ...` and the standalone binaries across 11
  mode invocations (run/compile/interpret/prove/surface/council/evolve/
  universal on their canonical sources, plus `cdc bridge verify` and
  `cdc bridge lookup-dyadic`). CT2 remaining: ordered per-check execution
  vectors for run/test (Phase C) and ownership-sanitizer sweep of the
  full unified binary.

- **Gate toolchain-verify-parity: LIVE (previous commit).** The native
  contract evaluator (`runtime/cdc_registry.{h,c}`, ~950 lines: registry
  collect + eval_expect + report mirroring `cdc_boot.py` exactly,
  including Python list-repr label formatting and sorted-witness
  framework-completeness details) is wired through ABI 1.1
  (`cdc_runtime_load` takes ownership; `cdc_runtime_verify` returns the
  report via `cdc_result_text`) and exposed as
  `cdc verify --contract <files...>`. verify.sh gates BYTE-IDENTICAL
  reports against the bootloader in success mode (full corpus, both exit
  0) and failure mode (fixture with failing expects, identical FAIL
  lines, both exit 1). This is the recorded deletion gate for
  `cdc_boot.py` (Mandate Gate 5): the Python file retires after the
  parity gate holds green for a full release cycle.

- **A11 bridge boundary: COMPLETE (commit after the merge).**
  `runtime/cdc_bridge_runtime.c` main is now guarded by
  `CDC_BRIDGE_NO_MAIN`, mirroring the native runtime; verify.sh proves
  both runtimes compile with entry points excluded (linkability for the
  unified binary). The standalone bridge CLI is byte-identical (all its
  verify.sh greps unchanged). Remaining for full CT2: un-static the
  runtime entry points behind the ABI, passthrough verbs in `build/cdc`
  with canonical-equivalent output, ordered per-check parity vectors.

- **Integration gate (review C2): COMPLETE (this merge commit).**
  `origin/codex/mobius-u-identity-system` (`313f0a1`, merge base `8cfe48f`)
  merged into the PR branch as the combined tip. Overlaps resolved:
  `scripts/verify.sh` carries both the identity asset/3D sections and the
  toolchain CT1/CT2/counterexample sections; `.gitignore` carries
  `*.blend1` plus the root-scoped build ignores; `README.md` merged
  without conflict. Combined-tree `./scripts/verify.sh`: **ALL CHECKS
  PASS** (identity assets 10 svg + 6 contracts + motion lab; 3D
  interchange validated in Blender-less mode; toolchain and
  counterexample gates unchanged: files=18 statements=5287). The
  identity lane's `tools/blender/*.py` sits outside the root-scoped host
  boundary (D8).

### Exact gate states (review language, corrected)

- PR CI: **PASS** at every pushed head (latest reviewed: `5a81400`).
- CT0: **PARTIAL** — provenance recorded; binary reproducibility and
  embedded verdict identity open.
- CT1: **PASS** — differential evidence plus both 2026-07-23 public-boundary
  counterexamples incorporated and green.
- CT2: **SEED/PARTIAL** — ABI 1.0 + driver skeleton; execute/verify, bridge
  no-main, runtime conversion, ordered per-check parity open.
- CT3–CT5, MM0–MM9, RG0: **NOT STARTED**. The plan commit is not a gate.

### Earlier (superseded framing corrected by the above)

- **Phase B step 3 (first half) — stable ABI + driver skeleton: COMPLETE
  (this commit).** `runtime/cdc_abi.{h,c}` (ABI 1.0: documented ownership/
  lifetime/thread-safety/determinism/error contract; parse, diagnostics,
  canonical bytes, deterministic JSON result serialization; execute/verify
  declared and failing closed with CDC_ERR_STATE until Phase C = ABI 1.1),
  `runtime/toolchain/` dispatcher + `cmd_verify.c` per A9 (consumes ONLY the
  ABI), built as `build/cdc`; internal AST type renamed `cdc_unit` so the
  ABI owns the public `cdc_program` name. verify.sh gates: `cdc version`,
  `cdc verify --parse` over all 18 root files (5287 statements), typed JSON
  rejection of invalid fixtures, unimplemented commands fail closed.
  Interface contract bumped to 1.0.1 (grammar 1 + ABI 1.0 recorded).
  Remaining in step 3: convert the native/bridge runtime consumers to the
  ABI (their `cdc_source.c` line scanning retires through the CT1/CT2
  removal gates).

- **Phase B steps 1–2 — canonical frontend + differential oracle: COMPLETE
  (this commit).** New modules `runtime/cdc_diagnostic.{c,h}`,
  `runtime/cdc_lexer.{c,h}`, `runtime/cdc_ast.{c,h}`,
  `runtime/cdc_parser.{c,h}` (grammar-version 1: collecting typed
  diagnostics, source spans, canonical serialization; acceptance
  byte-compatible with grammar 0 including the D5 comment quirk), harness
  `runtime/cdc_frontend_check.c`, gated `cdc_boot.py --dump` (D7), invalid
  corpus `tests/fixtures/frontend/`, and the `verify.sh` CT1 section.
- **Gate CT1 — partial.** Green: differential dump byte-identical over all
  18 root files (5286 records), roundtrip, attr-parity
  (47665 checked, 0 collisions/duplicates/failures; 4863 known
  quoting-class divergences), 12-case adversarial bounds, allocator-failure
  injection (765 runs), ASan/UBSan pass, 6/6 rejection-parity fixtures.
  Open for CT1 PASS: arbitrary-input fuzzing beyond the deterministic
  corpus, and "no production command uses the legacy substring parser" —
  the native/bridge runtimes still parse with `cdc_source.c`; conversion
  happens with the ABI (Phase B steps 3–7). Evidence:
  `evidence/gates/CT1/differential-summary.txt`.

### Earlier

- **Phase A — Freeze, provenance, and cross-repo contract: COMPLETE (this commit)**
  for the items executable in this repository/environment:
  - A.1/A.2 baseline tagged by digest record (no git tag pushed; the freeze is
    the evidence bundle + this file, per D6 single-branch discipline);
    toolchain identity recorded.
  - A.3 `CDC_MEMORY_MANIFOLD_INTERFACE.md` v1.0.0 published (digest in
    `evidence/gates/CT0/interface-digest.txt`); the Memory Manifold repository
    must carry the same content at the same digest before its Phase E starts.
  - A.4 machine-readable version registry: interface §1 (grammar 0→1,
    abi 0→1, evidence-format 1, mm-schema 0→1).
  - A.5 existing verification fixtures preserved untouched (all root `.cdc`
    files, `scripts/verify.sh` negative fixtures, Lean/Coq mirrors); inventory
    is the CT0 manifest.
  - A.6–A.9 (legacy Python archaeology, dual-schema inventory, ledger-first
    declaration, greenfield tree creation) are **Memory Manifold repository
    items — deferred**: that repository is not in this session's scope (see
    Deviations). The amendment §13 clean-room findings stand as the interim
    inventory of record, including the `payload_json`/`content` defect to be
    captured as an archaeological fixture at first access.
- **Gate CT0 — partial.** Provenance and toolchain identity recorded and
  reproducible from a clean checkout of this branch. Full CT0 (reproducible
  native binaries; manifest digest embedded in every verdict) completes when
  Phase B/C verdict emission exists. No PASS is claimed yet.

## Exact commands and results (Phase A)

```text
git rev-parse HEAD                      -> 99747e0a63ad14ad243934c122da73ec94a57940 (pre-Phase-A)
git rev-parse origin/main               -> 8cfe48fdb71e53af78411471869c064e6c650c63
./scripts/verify.sh                     -> "All checks passed." (local; Lean/Coq/Tectonic skipped, not installed here)
CI run 29960029272 (ci.yml, --require-formal) on 99747e0 -> in progress at freeze time
```

## Material deviations and rationale

1. **Remote container, not operator-local checkout.** The amendment's local
   paths (`/Users/edwardellis/...`, `/Volumes/ET External/...`) do not exist
   here; `ls /Volumes` → no such directory. Baseline recorded from the GitHub
   remote instead (D4). Reconciliation against the unpushed local branch
   `codex/mobius-u-identity-system` @ `313f0a1` is queued for whenever it is
   pushed or its digests are supplied.
2. **Superposition live tree unreachable.** §3.4's read-before-adapter list
   cannot be read from this container. Not blocking Phases A–D; required
   before Phase K/adapter work. The PC6 hard pause is preserved (nothing here
   can start it, and nothing will claim device evidence).
3. **Memory Manifold repository not in session scope.** GitHub access is
   scoped to this repository; repo-listing approval was requested and not yet
   granted. Phases E–H cannot land in their owning repository from this
   session until it is added. This lane proceeds A→D here; E–H follow when
   access exists (or transfer to the integration lane per amendment §14).
4. **Single work branch** `claude/bun-equivalent-build-plan-lxe772` for all
   phases in this lane (D6), instead of per-phase branches.
5. **Interim digests** (D2): git SHA-1 + SHA-256 until BLAKE3 is vendored.

## Next executable action

Phase B steps 3 (second half) through 7: (a) add `CDC_BRIDGE_NO_MAIN` to
`runtime/cdc_bridge_runtime.c` (A11) and link both legacy runtimes into
`build/cdc` as passthrough verbs with byte-identical output (the existing
verify.sh greps are the parity gate); (b) implement grammar-1 registry +
expect evaluation inside `cdc verify` (C mirror of `cdc_boot.py` semantics)
and gate its pass/total plus per-check parity vector against the bootloader
(gate toolchain-verify-parity, per-check format per interface §7); (c) then
Phase C `cdc run`/`cdc test` on the ABI execution surface (ABI 1.1);
(d) legacy scanner + `--dump` deletion through the recorded gates. Fuzzing
beyond the deterministic corpus remains queued for CT1 PASS.

## External blockers

- Memory Manifold repository access (blocks Phases E–H in this lane).
- Superposition tree access or supplied document digests (blocks Phase K
  conformance work; does not block A–D).
- Operator push of `codex/mobius-u-identity-system` (blocks reconciliation
  against the amendment's recorded local HEAD; does not block A–D).

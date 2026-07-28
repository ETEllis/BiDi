# RESUME_HERE — BiDi/CDC lane: full baton to completion

Updated 2026-07-24. This is the complete continuation contract from the
current head through the end of the amendment's Phase L, written for
whichever agent holds the baton next (per amendment §14 the integration
lane inherits when this lane's capacity ends). Read in order:
`CDC_TOOLCHAIN_PLAN.md` (with Amendment Record) → the two operator-held
documents (2026-07-22 amendment; 2026-07-23 adversarial review) →
`CDC_MEMORY_MANIFOLD_INTERFACE.md` → `docs/build/BUILD_STATE.md` →
`docs/build/DECISIONS.md` (D1–D8).

## Current head (2026-07-28)

- `main` = `3e851ff` — **PR #3 merged** under the baton's Section 1
  authorization (all three preconditions verified at the pinned head
  `82ab066`; merge commit preserves commit identities). Do not reuse PR #3.
- Work branch `claude/bun-equivalent-build-plan-lxe772`, restarted from the
  merged `main` (ADR D12), now at `41286c5`, open as **draft PR #4**:
  - `e4ba69b` — canonical BLAKE3 landed; **D2 interim-digest gate CLOSED**
    (31 reference vectors one-shot + streaming, plain + ASan; evidence
    re-digested by the vendored implementation; interim manifest retained).
  - `41286c5` — **product surfaces**: `ui/design/mobius-tokens.json`
    (single source of truth), `ui/macos/CDCStudio` (SwiftUI, zero deps),
    `ui/web/console` (self-contained, replay-bound), and
    `scripts/verify_ui.sh` wired into the main gate.
- Full `./scripts/verify.sh` green locally at `41286c5`; CI running.
- **UI is now a gate requirement**: no product gate closes without its
  surface locked to the canonical tokens and its data bound to real runtime
  output.

## External actions needed (not blockers for CDC work)

1. `ETEllis/Memory-Manifold` repository access — the `add_repo` request was
   denied by this environment's classifier. Blocks Memory Phases A/E–H only.
2. `ETEllis/GIST` access and the Superposition source bundle — blocks Phase K
   and GIST Track M only.
3. A macOS host to compile `ui/macos/CDCStudio` (no Swift toolchain in the
   Linux container; the gate builds it automatically where `swift` exists).

## Where the build stands (exact)

- **CT0 PARTIAL** — provenance recorded; binary reproducibility + embedded
  verdict identity open (closes in step 4 below).
- **CT1 PASS** — frontend differential + both review counterexamples,
  hard-gated incl. ASan/UBSan.
- **CT2 SUBSTANTIALLY LIVE** — ABI 1.2; toolchain-verify-parity
  (byte-identical bootloader reports, success + failure); unified binary
  passthrough parity (12 modes, exit codes included). Open: ordered
  per-check vector export; full-binary sanitizer sweep.
- **CT3 SUBSTANTIALLY LIVE** — `cdc test --gate` with the A7 typed policy
  (separate commit/hold/nest/fail; undeclared holds fail the gate even at
  runtime exit 0 — silent_hold.cdc fixture); `cdc run` is the fused
  single-process executor (universal closure path; multi-stage chaining
  over one live Runtime; fail-closed). Open: cancellation/budget/
  deterministic-mode contract; per-check vectors; cycles=N (blocked on
  per-cycle expectation families — inline expect-* pins first-cycle
  state; this is a language-design item, not a runtime bug).
- **CT4/MM1 SEEDED** — `cdc_store` reference backend: append-only sealed
  transactions, torn-tail recovery, fsync(file+dir) boundaries, interim
  sha256 identities (D2), replay determinism, attest; **crash matrix
  green: injection at all 7 commit boundaries recovers to exactly old or
  new state** (plain + ASan). Open: snapshot/compact/fence (declared,
  fail closed), BLAKE3 vendoring + re-digest, kill-based (out-of-process)
  crash injection, .cdc-declared persistence jobs, BiDi-decision wiring.
- **PC6 HARD-PAUSED** (unchanged, untouchable from this lane).
- PR #3: draft, all commits green through the combined identity+toolchain
  gate. **Do not merge without operator sign-off.**

## Baton: remaining work in dependency order

1. **Phase D completion (this repo).** Vendor BLAKE3 (reference C), swap
   `cdc_digest` to it, re-digest evidence (D2 closes). Implement
   snapshot/compact/fence with the same crash-matrix discipline. Add
   kill-based crash injection (spawn self, SIGKILL at boundaries) to
   verify.sh. Declare persistence ops as `.cdc` jobs (append/replay/
   attest exercised through `cdc run/test/verify`, per amendment D.6) —
   this is also where store mutations route through a BiDi commit
   decision (A10) rather than direct calls.
2. **CT2/CT3 closure (this repo).** Per-check ordered vector export
   (interface §7 format) from cdc test and cdc verify; lifecycle/
   cancellation/budget contract for cdc run; full-binary sanitizer sweep;
   then CT0 completion: reproducible-build check + manifest digest
   embedded in every verdict line.
3. **Deletion gates (this repo, after 2).** Migrate native/bridge runtime
   internals to the grammar-1 frontend (byte-identical outputs; greps are
   the net) → delete legacy scanner + `cdc_boot.py --dump`
   (frontend-differential-dump gate) → after toolchain-verify-parity
   holds a full release cycle, delete `cdc_boot.py` and renegotiate
   `python-files == 1` → `== 0` in kernel.cdc (operator-approved kernel
   change). External fuzzing lands here for CT1 full closure.
4. **Phase I (this repo).** `cdc install/build/x` per the plan +
   amendment A3/A4/A8: BLAKE3 manifests binding
   source/deps/toolchain/grammar-ABI/artifact/test/proof digests;
   crash-durable installs on the cdc_store substrate (the store's sealed
   transactions ARE the install journal); `cdc x` trusted-local-only
   until the sealed capability environment passes hostile-package gates
   (CT5).
5. **Phases E–H (Memory Manifold repo — NOT this repo).** Blocked in this
   environment until that repository is added to a session (or executed
   locally). The interface contract v1.0.3 (H12–H17, data contracts,
   store protocol, read/decide/apply semantics) is the binding surface;
   the cdc_store sealed-transaction + replay-digest model is the intended
   persistence core. MM0–MM6 gates as specified in the amendment.
6. **Phases J–L.** Product/SDK conformance (MM7), Superposition backend
   substitution (MM8 — requires the operator-side worktree; the adapter
   contract is interface §5), comparative/adversarial evidence (MM9),
   independent reproduction (RG0). These are integration-lane and
   operator-gated; nothing here may convert local evidence into device
   claims, and PC6 stays paused until its own gate.

## Hard rules in force (unchanged)

Repair-and-continue; amendment + review govern (D1/D8). Every commit
green through `./scripts/verify.sh`; byte-identical outputs for legacy
paths until recorded removal gates. Unreadable/non-regular sources are
CDC_ERR_IO; diagnostics never truncate. Balanced ternary stays typed;
merged pass totals forbidden; durable mutation behind commit semantics;
single writer per worktree; interface versions change only by new digest;
no identifier reuse. No public claim rests on an interim digest.

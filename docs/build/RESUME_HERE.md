# RESUME_HERE — BiDi/CDC lane: full baton to completion

Updated 2026-07-24. This is the complete continuation contract from the
current head through the end of the amendment's Phase L, written for
whichever agent holds the baton next (per amendment §14 the integration
lane inherits when this lane's capacity ends). Read in order:
`CDC_TOOLCHAIN_PLAN.md` (with Amendment Record) → the two operator-held
documents (2026-07-22 amendment; 2026-07-23 adversarial review) →
`CDC_MEMORY_MANIFOLD_INTERFACE.md` → `docs/build/BUILD_STATE.md` →
`docs/build/DECISIONS.md` (D1–D18).

## Current head (2026-07-28)

- `main` = `3e851ff` — **PR #3 merged** under the baton's Section 1
  authorization (all three preconditions verified at the pinned head
  `82ab066`; merge commit preserves commit identities). Do not reuse PR #3.
- Work branch `claude/bun-equivalent-build-plan-lxe772`, restarted from the
  merged `main` (ADR D12), open as **draft PR #4**:
  - `e4ba69b` — canonical BLAKE3 landed; **D2 interim-digest gate CLOSED**
    (31 reference vectors one-shot + streaming, plain + ASan; evidence
    re-digested by the vendored implementation; interim manifest retained).
  - `41286c5` — **product surfaces**: `ui/design/mobius-tokens.json`
    (single source of truth), `ui/macos/CDCStudio` (SwiftUI, zero deps),
    `ui/web/console` (self-contained, replay-bound), and
    `scripts/verify_ui.sh` wired into the main gate.
  - `0ea36ae` — CI repair: the CDC Studio build is gated on **Darwin**, not
    merely on a `swift` binary (the Ubuntu runner has swift, not SwiftUI).
  - `1722d96` — **store protocol complete**: snapshot, compact, and the
    compare-and-set fence, on a resumable replay chain (D14).
  - `1ea1ddd` — out-of-process **kill-based** crash matrix (SIGKILL in a
    forked child; recovery asserted by the surviving parent).
  - `bad2297` — **G10: persistence is a language form** (D15).
    `store`/`persist` source directives, capability `H6`,
    `framework_persistence.cdc`; `op=append` routes through the same
    `execute_commit` barrier, so durable mutation is unreachable except
    through an accepted commit decision.
  - **review repair (2026-07-28)** — store generations + interprocess
    lock, head-bound provenance, required macOS CI lane (D16).
- Full `./scripts/verify.sh` green locally at the current head.
- **UI is now a gate requirement**: no product gate closes without its
  surface locked to the canonical tokens and its data bound to real runtime
  output.

## External actions needed (not blockers for CDC work)

1. `ETEllis/Memory-Manifold` repository access — the `add_repo` request was
   denied by this environment's classifier. Blocks Memory Phases A/E–H only.
2. `ETEllis/GIST` access and the Superposition source bundle — blocks Phase K
   and GIST Track M only.
3. A macOS host to compile `ui/macos/CDCStudio`. There is no Swift
   toolchain in this container at all, so the required `macos-14` CI lane
   is the compiler of record — every Swift change here is unverified until
   that lane runs.

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
- **CT4/MM1 SUBSTANTIALLY LIVE** — `cdc_store`: append-only sealed
  transactions, tag-checked record framing (v2, UNKEYED — corruption not
  forgery), typed three-way recovery (torn tail / corrupt prefix / I/O
  fault), fsync(file+dir) boundaries, canonical BLAKE3 identities, replay
  determinism, attest covering the HEAD; **generations** — the compaction
  base lives in the log's HEAD, `snapshot` prepares and `compact` activates
  atomically (fsync/rename/dir-fsync), identity-bound by store uuid and
  monotonic generation (D16); **interprocess fcntl lock** held across
  re-scan+append+both fsyncs and across compaction, with the fence token a
  (generation, sealed, replay-state) triple (D16); crash matrix green at
  all 7 commit boundaries in-process **and** out-of-process via SIGKILL,
  plus 8 transition boundaries old-or-new-never-mixed (plain + ASan).
  **Persistence is a language form** (D15): `store`/`persist` directives,
  capability `H6`, `op=append` gated by the same balanced-ternary barrier
  as `commit`; `durable`/`replay` observed rather than declared.
  Open: keyed authentication (Ed25519 over the HEAD) and an external
  anchor so whole-file generation rollback is detectable; typed effect
  receipts / closure witnesses through the ABI; binding the episodic
  framework (`H3`) onto a declared store.
- **PC6 HARD-PAUSED** (unchanged, untouchable from this lane).
- PR #3: **merged** at `3e851ff` — finished, never to be reused. PR #4:
  draft, open. The 2026-07-28 review requested changes at `1ea1ddd` and
  that head must not be merged; the repair is D16. **Do not merge without
  operator sign-off.**

## Baton: remaining work in dependency order

1. **Phase D — COMPLETE.** BLAKE3 vendored and evidence re-digested (D2
   closed); snapshot/compact/fence landed on a resumable replay chain
   (D14); kill-based out-of-process crash injection landed; persistence
   declared as `.cdc` jobs with durable mutation routed through a BiDi
   commit decision (D15, amendment D.6 + A10). Remaining Phase-D-adjacent
   work has moved into step 2: typed effect receipts and closure witnesses
   through the ABI.
2. **CT2/CT3 closure (this repo).** Landed: typed effect receipts (D17,
   ABI 1.3) — outcomes are structured records, `cdc test` classifies from
   fields rather than prose, receipt/prose parity gated; ordered per-check
   parity vectors (D18) from `cdc verify --vectors` and
   `cdc test --vectors`, with ordering made load-bearing by the chained
   trace digest; lifecycle contract and whole-binary sanitizer sweep (D19)
   — budgets and cancellation at effect boundaries with the store proven
   intact at every stop point, determinism gated over prose/receipts/
   vectors, and the unified `cdc` binary now instrumented and required to
   agree with the plain build field-for-field.
   Closure witnesses landed too (D20): receipts carry
   `witness=<id> closure=<digest>` and the vector carries it through, so
   the section-7 record is complete — every field holds an honest value or
   an explicit "-".
   Remaining for CT2/CT3: CT0 completion — reproducible native binaries
   plus the manifest digest embedded in every verdict line.
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

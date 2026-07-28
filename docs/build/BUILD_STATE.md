# BUILD_STATE — BiDi/CDC lane

Updated at every accepted gate boundary. Companion files: `RESUME_HERE.md`
(exact continuation), `DECISIONS.md` (append-only decision record),
`../../evidence/gates/<gate-id>/` (evidence bundles).

## Current source identity

- repository: `ETEllis/BiDi-Coherence-Delta-Calculus` (GitHub remote)
- main: `79a508aa1441877aaeeafe415ef285983b186388` (PR #8 merge);
  active branch: `codex/c3-recursive-cells`
- baseline at Phase A freeze: `origin/main` = `8cfe48fdb71e53af78411471869c064e6c650c63`;
  work-branch HEAD entering Phase A = `99747e0a63ad14ad243934c122da73ec94a57940`
  (adds `CDC_TOOLCHAIN_PLAN.md`)
- environment: Claude Code remote container — Linux 6.18.5, cc (Ubuntu 13.3.0),
  Python 3.11.15, Lean pin `leanprover/lean4:4.31.0` (CI); full identity in
  `evidence/gates/CT0/toolchain.txt`
- tracked-file digests: `evidence/gates/CT0/source-identity.txt` (git blob ids)
  and `evidence/gates/CT0/sha256-manifest.txt` (interim SHA-256, see D2)

## Last completed phase and gate

- **D39 expanded end-state adopted (2026-07-28).** The runtime now carries a
  binding dependency plan for Models A-D, R0/QS/Q/H/B/G gates, horizon foundry
  and atlas experiments, and the biological transductive-observer/AI
  architecture. The same closure operator and failure semantics must survive
  every domain; cross-domain analogy alone earns nothing. These documents
  extend the build/research mission without promoting the current evidence.

- **D38 adversarial epoch/journal closure implemented and independently
  re-reviewed PASS (2026-07-28).** Canonical `CDJR` outcome records preserve
  accepted and terminally rejected causal history; restart reapplies the live
  peer policy; stale work remains exactly retryable; configuration-epoch
  migration is atomic and predecessor-bound; composite receipts bind child
  identity as well as exterior topology; and imported cell state is
  canonically revalidated. Permanent counterexamples reject wrong peer policy,
  mixed lineage, child rebinding, structural/state receipt tampering, and
  partial failed imports. The D38 rapid release, ASan/UBSan, and
  ThreadSanitizer gate is green. Maximal formal verification passes after
  staging every new source/evidence file and regenerating final provenance.

- **D37 C3 local durable execution kernel implemented; maximal gate green
  (2026-07-28).** On PR #8 merge `79a508a`, ABI 1.5 adds canonical immutable
  oriented frames, strict topology classification and witnessed transitions,
  provenance-preserving logical-cell state, a sealed bounded recursive
  scheduler, canonical `CDCP` command wire, typed application
  accept/hold/reject semantics, and an authenticated durable scheduler journal.
  Current `./scripts/verify_rftc.sh` is green in release, ASan/UBSan, and
  ThreadSanitizer lanes. Its permanent adversaries cover construction and
  arrival permutation, branch-cut ambiguity, unwitnessed transitions, old
  epochs, resource limits, concurrent duplicate ingress, terminal poison
  consumption, every store-commit crash boundary plus exact retry,
  deterministic fresh-scheduler restart,
  wrong replay key, duplicate delivery, committed corruption with zero
  scheduler mutation, and compacted-history refusal. The full formal/native/
  paper gate and provenance bundle are green on the active staged source.
  PR promotion, exact-head CI, and merge remain pending. This is a local
  classical kernel; actual cross-host session/reconciliation remains required
  for a C3 evidence label and no Q-level claim is made.

- **PR #8 MERGED — authenticated C3 local control plane (2026-07-28).**
  Exact head `54cff9a` passed both exact-head CI lanes; merge commit
  `79a508a`. ABI 1.4, D36, canonical shared-key envelopes, scoped authority,
  causal/partition defense, and serialized supervisor admission are now on
  main.

- **PR #7 MERGED — C3 six-form language ingress (2026-07-28).** Exact head
  `cdcf14ca1c691839b3fa56283b333b73cddeedf2` passed the full native/formal/
  paper lane and CDC Studio macOS lane; merge commit `4005a7b`. The active
  D36 branch layers the authenticated C3 control plane on that merged source.

- **C3 authenticated control-plane ABI merged and full gate green
  (2026-07-28, D36).** ABI 1.4 exports keyed canonical
  transport, scoped authority leases, replay/nonce and causal-order defense,
  and one opaque serialized supervisor admission boundary. Requested action
  and horizon are MAC-bound rather than supplied out of band. The maximal
  `./scripts/verify.sh --require-formal` gate passes official keyed BLAKE3
  vectors, malformed and tampered wire,
  an actual fork/socket cross-process round trip, partitions, causal gaps,
  wrong recipient/subject/frame/horizon/action/quorum/expiry, forged tickets,
  resource exhaustion before mutation, retry after commit failure, exactly-once
  simultaneous admission, ASan/UBSan, and ThreadSanitizer. This is shared-key
  authenticity, not Ed25519/mTLS or non-repudiation. Recursive local execution
  is implemented on the D37 branch; network-session identity,
  reconciliation, and cross-host C3 evidence remain open.

- **PR #6 MERGED — C1-C2 RFTC converged on the post-PR5 mainline
  (2026-07-28).** Exact head `f73a12c` passed the full native/formal/paper
  lane and CDC Studio macOS lane; merge commit `7b78cc5`. It includes the
  immutable frame reducer, relational record closure, sealed replay visitor,
  deterministic/alternate-seed/sanitizer crucible, and the seven-witness
  classical claim boundary.

- **C3 language ingress COMPLETE; distributed semantics remain active work
  (2026-07-28, D35).** The six RFTC forms (`frame`, `reduce`, `complex`,
  `topology`, `authority`, `transport`) are grammar-1 forms in the native
  parser, registry, canonical serializer, and frozen independent oracle.
  The explicit `R1-R6` capability range leaves D3's H12-H17 Memory Manifold
  reservation untouched. `rftc.cdc` declares the canonical boundary, and
  the RFTC gate proves all six forms parse with their attributes while
  identifier-free forms and unknown quantum-shaped aliases reject with typed
  diagnostics. PR #7 merged this ingress, PR #8 merged the authenticated
  authority/transport ABI, and D37 now implements the local recursive
  execution/restart kernel while cross-host C3 evidence remains open.

- **PR #4 MERGED; bootloader Option A executed (2026-07-28, D33).** The
  operator approved pinned head `6784ab5` after independent macOS
  verification (both weakened coordination builds caught, 20 clean
  concurrent-install trials, header forgeries failing closed) and both
  lanes green; merge commit `2947673`. The bootloader decision landed as
  Option A: `cdc_boot.py` frozen as a CI-only differential oracle,
  kernel floor renegotiated to `python-files == 0` with a by-name
  exemption rendered in every report, `bootloader minimal` still
  pinning the raw enumeration, Gate 5 recorded closed on the runtime
  dependency. Housekeeping from the approval note: the stale
  same-process wording in `runtime/cdc_store.h` now describes the D29
  shared-coordination design, and CDC Studio's generated `.build/` is
  ignored. PR #5 subsequently froze the Studio boundary and made the UI gate
  repeatable; merge commit `614696d`. PC6 stays hard-paused.

- **RFTC classical logical-cell crucible (2026-07-28, D34).**
  A C99 executable tests the missing layer between continuous phase flow
  and guarded commit through seven separately gated witnesses:
  synchronization onset, oriented winding-sector preservation, hidden
  microstate granularity, bidirectional macro/micro recovery, typed packet
  admissibility, redundant record closure, and an explicit causal cut.
  Identical smoke runs byte-match in JSON and CSV;
  an alternate seed produces a distinct passing record; the same lane
  passes ASan/UBSan. The 256-seed rapid and 4,096-seed stress profiles
  pass all seven witnesses on the operator host. The evidence surface is
  self-contained, byte-bound to the tracked smoke verdict, accepts fresh
  runtime verdicts, rejects malformed evidence, and passes narrow-screen
  validation. The full six-form architecture, authority/transport
  boundaries, claim ladder, and production stress matrix are frozen under
  `docs/rftc/`; integrated implementation is the next gated lane, not a
  completed claim.

- **Second review round repaired (2026-07-28, D30).** Four findings at
  `810f1f6`, each confirmed against the reviewer's own probes and gated:
  (1) BSD `wc` padding failed the suite on a real Mac while both CI lanes
  stayed green — counts are portable now and the macOS lane runs the FULL
  native suite (formal proofs stay Linux-only); (2) `coord_release`
  closed its descriptor after releasing the registry lock — a new opener
  could register in the gap and the stale close dropped its fresh lock;
  closed under the lock now, with a deterministic 1->0->1 lifecycle check
  and a permanent release-window probe build that only that check
  catches; (3) manifest headers were "format only" — one strict shared
  parser now reads both formats everywhere, counts are re-derived, and a
  permanent sweep mutates/removes every header field; (4) concurrent
  installs duplicated the journal (29/30 trials) — capture-once immutable
  bytes, a package-scoped lock, attempt-unique staging, a CHECKED final
  directory sync before `durable=1`, an `after-latch` kill boundary, and
  deterministic fifo-rendezvous concurrency gates (identical, divergent,
  and mid-install mutation).

- **Same-application store coordination repaired (2026-07-28, D29).**
  The repair requested after `ca26608` was confirmed absent (five checks,
  5/5 failing at `fe3d61d`) and landed: all handles in one process that
  name the same store share a reference-counted coordination object — one
  pthread mutex plus ONE fcntl descriptor, keyed by lock-file
  device+inode and pid, alive until the last close — so same-process
  handles are mutually excluded, closing one handle can no longer drop
  the lock another is holding (the POSIX close-drops-locks hazard), and
  a forked child never adopts an inherited object. Open recovery and
  reset joined commit/snapshot/compact inside the critical section. The
  `store-samep` suite proves blocking deterministically (polled
  completion pipes while the section is held; premature completion IS
  the failure) across open, commit, snapshot+compact, reset, and the
  close hazard; verify.sh requires it to fail exactly 5/5 against the
  permanent per-handle probe build, and it runs under ASan/UBSan plus a
  guarded ThreadSanitizer lane (clean on first contact). A handle whose
  store was compacted or reset under it is refused typed
  (ESTATE/ECORRUPT), never allowed a corrupt append.

- **Phase I COMPLETE — `cdc build` / `cdc install` / `cdc x`
  (2026-07-28, D28).** The last three toolchain commands are live and
  hard-gated:
  - `cdc build` emits the canonical bundle plus a manifest binding
    grammar/ABI versions, per-source digests, the corpus identity, the
    artifact digest, and the contract verdict. A red corpus is refused; a
    bundle that does not re-verify with the sources' own verdicts is never
    emitted; outputs are temp-then-rename, deterministic (built twice,
    byte-compared), and cross-checked byte-for-byte against
    `cdc_frontend_check canon`. `--check` types source-drift by file,
    manifest-malformed by line, and bundle/manifest disagreement as
    `artifact-mismatch` (which side moved is not guessable from inside).
  - `cdc install` journals every member through a sealed `cdc_store`
    transaction (the journal IS the install record) and latches the
    directory by staging + fsync + rename. Kill matrix at three named
    boundaries (process genuinely SIGKILLed, rc=137): the package
    directory is never present after a kill, the journal always opens and
    verifies, a plain re-run heals. A held package writes NOTHING (journal
    byte-identity gated); zero expectations refuse (`zero-evidence`);
    reinstalls are idempotent when identical and refused typed when
    divergent. The install effect carries a D17 receipt.
  - `cdc x` re-digests every member against the install manifest, requires
    the directory to be exactly the manifest's member set (unmanifested
    code does not run), recomputes the corpus identity, and rejects path
    separators in the request itself. TRUSTED-LOCAL-ONLY until CT5; the
    boundary is stated in code, docs, and matrix, not sandboxed around.
  - The sanitized sweep runs Phase I too, and caught a real bug on first
    contact (unterminated `read_all` buffers walked into the heap).
  - `deletion gates status`: scanner deleted, `attr-parity` retired,
    `attr-boundary` repointed, `--dump` kept as the last independent
    oracle (D22–D27); `cdc_boot.py` deletion awaits Edward's A/B/C choice
    (`docs/build/BOOTLOADER_DELETION_PROPOSAL.md`); kernel floor
    untouched.

- **Independent review of `1ea1ddd` repaired (2026-07-28).** Two mechanism
  defects and three overclaims. Recorded as D16; the withdrawn claims are
  listed there rather than softened here.

  - **Snapshot/compaction is now ONE atomic generation transition.** The
    base moved into the log's own HEAD record (store uuid, monotonic
    generation, base sealed/events/state, anchor over the replaced HEAD).
    `snapshot` prepares `base.pending`, which `open` never reads; `compact`
    rebuilds the log and activates it with fsync -> rename -> directory
    fsync. Killing a child at each of the 8 boundaries of that transition
    leaves the old generation or the new one, never a mixture, and the
    store always opens and verifies.
  - **The fence is backed by real mutual exclusion.** An fcntl write lock
    on `lock.cdcstore` is held across re-scan + append + fsync(file) +
    fsync(dir), and across compaction. The armed token is the whole
    (generation, sealed, replay-state) triple. Two forked writers released
    from a barrier after both fenced now yield exactly one commit and one
    refusal, three rounds running; the previous test was sequential and
    could not enter that window.
  - **Attestation covers the base**, so two different histories compacted
    to the same sealed count attest differently, and two stores with
    identical histories attest differently.
  - Both new suites (`store-generation`, `store-race`) were verified to
    FAIL against deliberately re-broken builds before being accepted —
    removing the lock reproduces the reviewer's `writer-a=ok writer-b=ok
    reopen=corrupt-tail`; removing the identity binding activates a foreign
    base; skipping the HEAD in attest collapses distinct histories.
  - **Provenance is head-bound.** `scripts/regen_provenance.sh` regenerates
    the manifest from `git ls-files`; the gate checks the path set and the
    bytes, with a counterexample that modifies a tracked file and restores
    it. The old manifest claimed "all tracked files" at 166 entries against
    186 tracked.
  - **CDC Studio has a required macOS CI lane** (`macos-14`, `swift build`
    then `swift test`). The Linux structural gate was correct as a platform
    guard but was the only required UI check, so a surface that did not
    compile reached a green PR. The compile error is fixed (`padded` was a
    `String` method applied to `ReversedCollection<String>`), and the
    subprocess drain now reads stdout and stderr concurrently with a
    SIGTERM/SIGKILL timeout — draining one to EOF before the other
    deadlocks deterministically once the child fills the undrained pipe.

- **Phase D COMPLETE — persistence became a language form (2026-07-28).**
  `store` and `persist` are source directives (capability `H6`,
  `framework_persistence.cdc`), so durable state is exercised through
  `cdc run` / `cdc test` rather than as a C library called out of band.
  The gate is the calculus itself: `op=append` invokes the identical
  `execute_commit` barrier that governs in-memory latching, and only an
  accepted decision reaches `cdc_store_stage`/`cdc_store_commit` — a
  violated prefix balance stages nothing, so no code path exists on which
  a held decision could write.

  What the gate checks (all in `scripts/verify.sh`):
  - the barrier gate itself — admissible → `durable=yes replay=changed`;
    violated → `held reason=balance-violation durable=no replay=stable`;
  - **byte-identity measured outside the runtime that claims it**: seed one
    accepted append, copy the 176-byte sealed log, replay three violating
    appends over the same store, `cmp` — identical;
  - the compaction divergence `durable=yes replay=stable` (D14's resumable
    chain restated at the language level), and `compact-uncovered` holding
    when no base covers the sealed prefix;
  - a real compare-and-set counterexample written in `.cdc` — two declared
    handles onto one directory, the rival moves the log, the fenced writer's
    perfectly admissible append is refused before a byte is written;
  - four permanent counterexamples: overclaimed durability, overclaimed
    replay identity, undeclared durable hold (A7 applies to persistence
    with no exception — `cdc test --gate` fails at runtime exit 0), and
    uncovered compaction;
  - an ASan/UBSan pass over the whole persistence path (three store handles,
    two onto one directory).

  `durable` and `replay` are **observed** from the store on every job, never
  taken from the declaration; the two overclaim fixtures exist to keep that
  true. Recorded as D15.

- **Phase D substrate work (2026-07-28).** Store protocol completed
  (snapshot / compact / compare-and-set fence) on a resumable replay chain:
  compaction preserves the replay identity byte-for-byte while the attest
  digest over raw bytes changes, and a stale writer's commit is refused
  without writing. Snapshots are authenticated and swept byte-by-byte
  (85/85 fail closed). Out-of-process crash matrix added: 7/7 children
  genuinely SIGKILLed at every commit boundary, each recovering to exactly
  the old or new sealed state — a distinct loss mode from the in-process
  torn-write matrix (unflushed stdio buffers are lost), which is why the
  old/new split differs and only the invariants are gated.
  The `.cdc`-declared persistence surface that this substrate was built for
  landed in the same session (entry above). Typed effect receipts through
  the ABI moved to CT2/CT3 closure.

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
  ASan). Every open item listed here at the time — BLAKE3 swap,
  snapshot/compact/fence, kill-based injection, `.cdc`-declared persistence
  jobs, BiDi-decision wiring — has since landed; see the Phase D COMPLETE
  entry above.

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
  runs=24 commit=19 hold=8 (all expected) nest=10 fail=0, exact-gated in
  verify.sh — the persistence framework was absorbed by adding one mode
  rule and no policy change, because the ternary vocabulary already
  covered durable holds. CT3 remaining: cancellation/budgets/deterministic-mode
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

None in this repository. Every component executable here is complete and
gated (Phases A–D, CT0–CT3 closure, the deletion gates through the
scanner, Phase I). Remaining items by category:

- **Queued with recorded reasons**: Ed25519 keyed authentication +
  external anchor; CT5 sealed capability environment + hostile-package
  counterexamples (until then `cdc x` is trusted-local-only);
  `package.cdc` manifest layer / versioning / lockfile; `cycles=N`
  per-cycle expectation families; fuzzing beyond the deterministic corpus
  (CT1 full closure).
- **Awaiting Edward**: the `cdc_boot.py` deletion choice
  (`docs/build/BOOTLOADER_DELETION_PROPOSAL.md`, options A/B/C). Status
  quo is option A; the kernel floor stays untouched until he chooses.
- **Blocked external**: Memory Manifold repository (Phases E–H); GIST /
  Superposition bundle (Phase K, Track M); macOS host (CI lane is the
  Swift compiler of record). PC6 stays hard-paused.

## External blockers

- Memory Manifold repository access (blocks Phases E–H in this lane).
- Superposition tree access or supplied document digests (blocks Phase K
  conformance work; does not block A–D).
- Operator push of `codex/mobius-u-identity-system` (blocks reconciliation
  against the amendment's recorded local HEAD; does not block A–D).

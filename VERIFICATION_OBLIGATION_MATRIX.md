# Verification Obligation Matrix

This matrix binds four fields that must not be collapsed:

```text
scope -> evidence maturity -> execution verdict -> receipt or open obligation
```

The project does not resolve critique by weakening its target. It preserves the
target while refusing to promote a declaration, numerical fixture, or finite
lemma beyond the mechanism and scope it actually establishes.

## Two independent claim axes

| Maturity | Meaning | Representative surface |
|---|---|---|
| specified | source or mathematical contract exists; current execution is not implied | richer flow/event/delay semantics and future source-bound polarity |
| executed | native implementation consumes the selected source/state and emits a bound receipt | grammar, reducer, U1, U2 tangent, recurrence, persistence |
| adversarially verified | positive/negative/oracle/permanent-mutant gates distinguish the implementation from shortcuts | commit holds, U1 negative fixtures, U2 recurrence/restoration/product-order/polarity mutants |
| mechanized finite | Lean or Rocq/Coq proves the exact finite statement named | finite carrier/algebra, event order, aperture, cone, sheet, and barrier counterexample |

| Verdict | Meaning |
|---|---|
| accepted | every prerequisite for the scoped result passed |
| held | a typed dynamical or semantic prerequisite did not pass; no stronger result is emitted |
| violated | an asserted contract or invariant failed |
| not-emitted | a gated downstream artifact was not authorized |

`held` is not an evidence-maturity rung. The canonical U2 recurrence hold is
executed and adversarially verified precisely because the gate refuses a false
return-map claim.

## Current U1/U2 decision surface

| Scope | Maturity | Verdict | Binding evidence |
|---|---|---|---|
| `framework_loop.cdc` U1 lifted-cover closure | executed + adversarially verified | accepted | reciprocal cone checks, one/two-turn sheet checks, holonomy, record/decision equality, guarded enactment |
| canonical 13-coordinate U2 path tangent | executed + adversarially verified | accepted | source-ordered manifest, exact ordered operation Jacobians, `pathTangentDigest` |
| canonical complete-state recurrence | executed + adversarially verified | held: `recurrence-mode-mismatch` | complete initial/final state and discrete-mode comparison |
| projected cover-only return | executed + adversarially verified | held: `undeclared-quotient` | projection cannot authorize full-system monodromy |
| canonical monodromy and characteristic multipliers | recurrence-gated | not-emitted until recurrence verifies; downstream spectral holds retain monodromy but not multipliers | recurrence-stage holds contain null monodromy/multiplier fields; backend holds keep the return operator and null multiplier fields |
| isolated explicit `4 pi` relative return | executed + adversarially verified | accepted; `marginal` | complete restoration action and `D rho`, `M_rel = I_13`, validated real-Schur spectrum |
| generic polarity-covariance mechanism | adversarially verified; finite components mechanized | accepted for analytic fixture | involution, fixed aperture, primal conjugacy, and tangent conjugacy residuals |
| `loop-u720` polarity covariance or physical polarity law | specified only / exploratory | not-emitted | no source-bound conjugate U1/U2 pair or empirical specialization |

## Detailed obligation inventory

| Claim | Current evidence and verdict | Remaining obligation |
|---|---|---|
| balanced-ternary carrier | `laws.cdc` declares carrier witnesses | finite codomain proof for `commit` |
| dyadic/triadic bridge closure | `laws.cdc` declares the invariant; `bridge64.cdc` declares all 64 rows; `runtime/cdc_bridge_runtime.c` consumes and validates them | finite uniqueness/bijection proof for bootstrap codebook |
| existence viability | `laws.cdc`, `relations.cdc`, and `trace_windows.cdc` declare viability witnesses | viability invariant over bounded continuity, permeability, and transition capacity |
| trace-order locality | `laws.cdc`, `relations.cdc`, and `trace_windows.cdc` declare local trace, local-counter, detuning, overlap, and recursive-policy witnesses | partial-order theorem for causal trace windows, local event counters, and projection policy |
| gate is abelian | `laws.cdc` declares associativity, commutativity, identity, inverse witnesses; Lean/Coq finite mirrors check the finite carrier laws | extend finite algebra law to algebraic group proof over torus carrier |
| interference is monoidal | `laws.cdc` declares associativity, commutativity, void-unit witnesses; Lean/Coq finite mirrors check the finite carrier laws | extend finite monoid law to commutative monoid proof over the continuous carrier |
| rotation is linear | `laws.cdc` declares rotation-linearity witness; Lean/Coq finite mirrors check finite rotation linearity | carrier action proof over the continuous carrier |
| core-fold is a morphism | `laws.cdc` declares linearity/equivariance and non-idempotence witnesses | projection morphism proof |
| commit preservation | `laws.cdc` declares preservation witness; `native_reducer.cdc` includes accepted and held commit fixtures | accepted/held case split over cell order; no negative-balance repair mutation |
| commit soundness | `laws.cdc` declares commit and flow-subset soundness witnesses | guard accept/hold case split |
| local confluence | `laws.cdc` declares disjoint-commit witness | footprint-disjoint diamond lemma |
| flow additivity | `laws.cdc` declares split-duration witness | monoid-action proof for the flow relation |
| normal forms | `laws.cdc` declares localized normal-form witnesses; `native_reducer.cdc` declares `proof trit-walk-n6`; `runtime/cdc_native_runtime.c` checks `729 / 267 / 51 / 20 / 5`; Lean and Coq mirrors live in `formal/` | extend finite normalization from checked counts into commit-barrier preservation theorem |
| angular/path relation | `relations.cdc` declares angle, lines, path endpoints, and nesting-cone witnesses | path-indexed relation algebra |
| trace/window layer | `trace_windows.cdc` declares passive/committing separation, role-relative observer, incidence boundary, coupled-observer, shared-state commit, and causal-window witnesses; `native_surface.cdc` executes trace, measure, and policy jobs in the C native runtime | derived-observer theorem over flow/commit/nest |
| generated higher-arity codebooks | `bridge512.cdc` and `bridge4096.cdc` contain the full `n=9` and `n=12` generated rows; `runtime/cdc_bridge_runtime.c` verifies and regenerates them; `scripts/verify.sh` compares tracked rows to runtime output | native `.cdc` generator rule or extracted proof for arbitrary `n=3k` |
| interactive bridge grid | `assets/bridge64-grid.svg` is regenerated by `runtime/cdc_bridge_runtime.c grid-svg`; `scripts/verify.sh` checks interactive class/script presence and exact asset drift | native UI artifact generation remains an engineering witness, not a theorem obligation |
| shared native parser/expectation core | `runtime/cdc_source.c` / `runtime/cdc_source.h` provide `.cdc` line parsing, attribute extraction, typed attrs, and primitive expectation assertions shared by the bridge and reducer runtimes; `scripts/verify.sh` links it into both C runtimes and compile-checks it beside the WASM export path | full source AST, declaration registry, and repo-wide `cdc_boot.py` parity in native/WASM form |
| bridge coordinate runtime | `system.cdc` declares `G1`; `bridge_jobs.cdc` declares coordinate jobs; `scripts/verify.sh` compiles the C runtime and checks lookup, source-declared jobs, higher arity, and grid generation | native `.cdc` reducer or extracted kernel replacing the C bridge pilot |
| native reducer runtime | `native_reducer.cdc` declares field/module/cell/channel state and flow/commit/nest jobs; `native_surface.cdc` declares guard/trace/measure/policy/bridge/counter jobs; `runtime/cdc_native_runtime.c` executes them and emits checked replay JSON | express the native reducer itself in `.cdc` |
| WASM replay export | `runtime/cdc_wasm_exports.c` exposes the native replay JSON path through a C ABI; `scripts/verify.sh` compile-checks the export and links with `emcc` when available | live browser WASM runtime and repo-wide `cdc_boot.py` parity |
| executable reducer IR | `native_reducer.cdc` declares compile and interpret jobs; `runtime/cdc_native_runtime.c` emits reducer IR and executes it through an interpreter path | self-host the compiler/interpreter in `.cdc` |
| native compile/proof path | `native_reducer.cdc` declares compile/proof jobs; `runtime/cdc_native_runtime.c` emits reducer IR and exhaustively checks finite carrier counts | self-host the compiler/reducer in `.cdc`; extract or port proof obligations to Lean/Coq/Kani |
| council deliberation | `council_bridge.cdc` declares a source-level council and deliberation; `runtime/cdc_native_runtime.c` projects member trits into dyadic/triadic bridge coordinates and checks the expected decision | generalize council semantics into the main reducer and trace/window policy layer |
| bridge-coordinate source evolution | `council_bridge.cdc` declares an `evolve` job; `runtime/cdc_native_runtime.c` writes a copied `.cdc` source with the requested bridge-coordinate witness and verification checks containment | replace copy/append pilot with typed native source-transform semantics |
| transition framework | `framework_transition.cdc` declares capability `H1` with guard/flow/commit/nest/trace/measure/policy/bridge/counter binding witnesses; `scripts/verify.sh` executes the exemplar through run and surface modes | typed transition-system semantics over the commit relation |
| procedural framework | `framework_procedural.cdc` declares capability `H2` with cue/step/retry/consolidate witnesses plus self-referential compile and interpret jobs; the gate executes run, compile, and interpret modes | procedure IR beyond flow/commit/nest op kinds; self-hosted compiler path |
| episodic framework | `framework_episodic.cdc` declares capability `H3` with live/record/consolidate/aperture/content/recall/key/ordinal witnesses; the gate executes run and surface modes and cross-checks bidirectional bridge64 recall | binding episodic record/recall onto the declared `H6` durable store; multi-episode window algebra |
| deliberative framework | `framework_deliberative.cdc` declares capability `H4` with quorum and enactment witnesses; the gate checks the adopted coordinate and requires the appended decision-memory witness in the evolved copy | generalize council semantics into the main reducer and trace/window policy layer |
| framework role contracts | `framework` declarations in the six `framework_*.cdc` files carry `requires=`/`permits=`; `cdc_boot.py` enforces role completeness, uniqueness, orphan closure, exactly-one executable link, link existence, and role-primitive compatibility through `expect framework <key> complete` and `expect frameworks closed` | native/C parity for the framework-contract checker under the bootloader deletion gate |
| task-loop composition | `framework_loop.cdc` executes two sense/act/integrate cycles over one shared state object in a single `run`/`interpret` invocation, with second-cycle expectations (carried phase 0.492228, cumulative belief 1.333333) reachable only through first-cycle mutations; surface/council/evolve modes exercise gate, record, recall, refine, key, decide, and enact over the same source and the adopted coordinate equals the recorded key | unbounded cycle iteration beyond the two-cycle fixture |
| U1 lifted-cover closure | the `universal` runtime mode holds one live runtime object through reducer execution, trace/bridge projection, council decision, closure validation, and enactment; acceptance requires reciprocal angularly biased receptive/radiant cones, half-turn projection return with sheet inversion, full 720-degree projection return with sheet restoration at winding two, holonomy 0.125 agreement, accepted local commits, and runtime-computed record/decision coordinate equality before the computed coordinate is enacted with a `universal-closure` witness; three negative fixtures hold with `full-sheet-mismatch`, `cone-not-reciprocal`, and `coordinate-mismatch` and create no evolved output; this accepted verdict does not claim every native coordinate and latch returned | continuous frame transport and holonomy preservation theorem; scale-equivariance; any physical light-cone or gravity specialization; full-state recurrence must remain a separate U2 receipt |
| U2 path tangent | `framework_loop.cdc` declares `orbit -> variational -> spectrum` and a U2 witness; `stability` closes a source-ordered 13-coordinate manifest over the admitted U1 path, differentiates current native `flow`/fixed-mode `commit`/`nest` maps in execution order, and emits initial/final state, selected-cell discrete mode identities, itinerary, matrix, and source/manifest/path-tangent digests; analytic component maps are compared to adaptive finite differences and product/nest shortcuts are killed by permanent mutants; verdict for the canonical tangent is accepted even though complete recurrence does not hold | an optional whole-path finite-difference receipt for each arbitrary source execution; automatic differentiation or a proof that generated operation Jacobians refine the primal executor |
| U2 complete recurrence | the canonical full-state analysis compares continuous state plus latch/mode and correctly holds `recurrence-mode-mismatch`; the projected cover fixture separately holds `undeclared-quotient`; neither emits monodromy or multipliers; false-recurrence acceptance is a permanent mutant | a genuinely recurrent full-state canonical task loop, or an explicit restoration action for every changed coordinate with equivariance and section witnesses |
| U2 relative restoration | the isolated positive fixture declares a cover-phase translation by `-4 pi`, binds the full row-major `D rho = I`, witnesses equivariance and U1 section compatibility, applies `D rho` as the terminal factor, and accepts relative recurrence; omission of the restoration derivative is a permanent mutant | source-language restoration forms beyond the current narrow phase quotient; non-identity restoration fixtures over a source-bound U1 path |
| U2 monodromy and spectrum | only verified full or relative recurrence authorizes `M = D U` or `M_rel = D rho D U`; the positive fixture emits `I_13`, thirteen `physical`-labeled `+1` multipliers, and `marginal`; the deterministic real-Schur backend is residual-checked and the no-backend binary holds without fabricating eigenvalues | broader conditioned and defective-matrix coverage; domain-specific interpretation only after state, units, calibration, and alternatives are supplied |
| hybrid-event sensitivity | `runtime/cdc_variational.c` and `tests/u2/` adversarially verify reset Jacobian versus saltation, transversality holds, ordered noncommuting events, and itinerary checks; the current source `guard` is not bound to an off-grid transition, so canonical receipts honestly report `unbound-no-saltation` | source syntax and primal execution for localized guard surfaces, crossing direction, reset, priority/simultaneous composition, Zeno budget, and matching primal/tangent event times |
| neutral/gauge modes | U2 retains near-unit modes as `physical` unless a declared generator and section/restoration alignment verify a removable neutral mode; false automatic deletion is gated | source-level symmetry generators and Poincare-section binding for real periodic-orbit specializations |
| scoped polarity covariance | the generic runtime gate independently checks source/target involution, aperture fixation, primal conjugacy, and tangent conjugacy; three polarity mutants are rejected; Lean/Rocq keep carrier, cone, and sheet involutions separate and prove the `+-`/`-+` barrier counterexample | bind an explicit conjugate Universal Operator in `.cdc`, require independent recurrence on both sides before spectral comparison, and keep physical polarity an empirical specialization |
| U2 finite formal layer | `formal/{lean,coq}/U2VariationalFinite.*` mechanizes finite identity/associative composition, observable event order, carrier/cone/sheet involutions, fixed aperture, and the barrier counterexample | runtime Jacobian correctness, recurrence, numerical spectrum, source-bound covariance, and continuous/physical theorems remain open |
| BiDi-gated durable persistence | `framework_persistence.cdc` declares capability `H6` with declare/recover/gate/hold/replay/attest/snapshot/uncovered/compact/fence/contend witnesses over the `store` and `persist` source forms; `runtime/cdc_native_runtime.c` routes `op=append` through the same `execute_commit` barrier that governs in-memory latching, so only an accepted decision reaches `runtime/cdc_store.c`; `durable` and `replay` are observed from the store, never taken from the declaration; `scripts/verify.sh` checks the barrier gate, byte-identity of the sealed log across three held appends measured OUTSIDE the runtime, the `durable=yes replay=stable` divergence under compaction, a two-handle compare-and-set refusal, four permanent counterexamples (overclaimed durability, overclaimed replay identity, undeclared durable hold, uncovered compaction), and an ASan/UBSan pass over the whole persistence path | typed effect receipts and closure witnesses through the ABI; keyed authentication (Ed25519 over the HEAD) so integrity survives a motivated forger rather than only corruption; an external anchor so whole-file generation rollback is detectable; a durability theorem rather than per-run runtime checks; distributed or networked stores |
| store generation transitions and writer serialization | `runtime/cdc_store.c` carries the compaction base in the log's own HEAD record (store uuid, monotonic generation, base sealed/events/state, anchor over the replaced HEAD); `snapshot` prepares `base.pending` that `open` never reads and `compact` activates it with fsync/rename/directory-fsync under an fcntl write lock also held across re-scan+append+both fsyncs; the fence token is the (generation, sealed, replay-state) triple; attest digests from byte zero so it covers the base; `scripts/verify.sh` runs `store-generation` (snapshot-only reopen, kill at all 8 transition boundaries with old-or-new-never-mixed, twin-history base substitution refused on store identity alone, stale-generation base refused, distinct stores and distinct histories attest differently, FIFO/directory bases typed EIO) and `store-race` (two forked writers released from a barrier after both fenced: exactly one commits, one is refused, the store verifies and holds only the winner, three rounds; plus commit-vs-compact preserving the commit) — both suites were verified to FAIL against deliberately re-broken builds before acceptance; same-application coordination (D29): all handles in one process share a reference-counted coordination object (one mutex + ONE fcntl descriptor kept until the last close, keyed by lock-file device+inode and pid), open recovery and reset joined the critical section, and the five-check `store-samep` suite (open, commit, snapshot+compact, reset, close-hazard; blocking proven by polled completion pipes, never sleeps) must fail exactly 5/5 against the permanent per-handle probe build (`CDC_STORE_TEST_PER_HANDLE_LOCK`), runs under ASan/UBSan, and has a guarded ThreadSanitizer lane | keyed authentication over the HEAD; an externally retained anchor to detect whole-file rollback; multi-writer semantics beyond one-lock serialization |
| toolchain build/install/x | `runtime/toolchain/cmd_build.c` emits the canonical bundle plus a BLAKE3 manifest binding grammar/ABI versions, per-source digests, the corpus identity, the artifact digest, and the contract verdict; a red corpus and a non-re-verifying bundle are refused, outputs are temp-then-rename, deterministic (built twice, byte-compared), and cross-checked against `cdc_frontend_check canon`; `cmd_install.c` captures every member once into immutable attempt-owned bytes (D30), journals through a sealed `cdc_store` transaction under a package-scoped fcntl lock, latches via attempt-unique staging+fsync+rename with the final directory sync CHECKED before `durable=1`, with a four-boundary kill matrix (no partial directory, journal intact, re-run heals, durable never claimed early), deterministic concurrent-install gates (identical -> one install + one idempotent observation; divergent -> one winner + one typed refusal, no byte mixing; ONE journal transaction either way), a mid-install source-mutation gate, held-writes-nothing by journal byte-identity, zero-evidence refusal, and a typed install receipt; both manifest formats are read ONLY by the strict shared parser (`runtime/toolchain/cdc_manifest.{h,c}` — exactly one pinned header, closed vocabulary, re-derived counts) with a permanent field-by-field mutation sweep; `cmd_x.c` re-digests every member, refuses tampered/unmanifested/traversal/non-member entries by name, and runs the entry through the fused executor | CT5: sealed capability environment and hostile-package counterexamples (`cdc x` is trusted-local-only until then — no network, no registries, no archives, and verification is drift detection, not a sandbox); package versioning and the `package.cdc` manifest layer; Ed25519 signing over bundle and package manifests |
| RFTC C3 authenticated control plane | `runtime/cdc_transport.{h,c}` implements the canonical `RF3W` envelope with action, horizon, payload, and causal state covered by a domain-separated keyed BLAKE3 MAC; `runtime/cdc_authority.{h,c}` checks and consumes bounded versioned leases; `runtime/cdc_supervisor.{h,c}` serializes authenticate/causal/authority/capacity/commit/consume; `scripts/verify_rftc.sh` gates official keyed vectors, wrong key/recipient/subject/frame/horizon/action/quorum/expiry, zero digests, replay, forged authority tickets, partition/gap/parent/logical-clock holds, process wire roundtrip, malformed/trailing/tampered input, precommit resource exhaustion, retryable commit failure, concurrent exactly-once admission, ASan/UBSan, and TSan | Shared-key authenticity only: Ed25519/mTLS identity, key rotation, deployed network sessions, quorum certificates, recursive cells, and cross-host reconciliation remain open; no Q claim |
| double-cover sheet parity | `formal/lean/CDCFinite.lean` and `formal/coq/CDCFinite.v` mechanize `one_turn_inverts_sheet` and `two_turns_restore_sheet` over the Z2 sheet | extend finite parity to continuous lifted-path holonomy |
| native language center | `kernel.cdc` declares terms, rules, capabilities, witness counts, and a one-file Python language/runtime/semantic-execution boundary (`cdc_boot.py`); Blender asset-generation utilities under `tools/` are out of scope | native reducer and proof checker expressed in `.cdc` |

## Mechanized finite layer

The first theorem-prover port is now seeded by `formal/lean/CDCFinite.lean` and
`formal/coq/CDCFinite.v`, and mirrored by the native C proof checker. It remains
finite, but includes carrier counts and finite algebraic laws:

1. balanced-ternary carrier;
2. `3^6` committed-walk codomain;
3. prefix-walk admissibility;
4. commit-barrier preservation;
5. localized normal forms;
6. `2^6 = 4^3 = 64` bridge codebook uniqueness and totality;
7. finite gate associativity/commutativity/identity/inverse;
8. finite interference associativity/commutativity/unit;
9. finite rotation linearity.

The separate `U2VariationalFinite` mirrors add finite tangent identity and
associativity, observable order for noncommuting maps, fixed-aperture carrier
inversion, distinct cone/sheet involutions, and the `+-` accepted / `-+` held
barrier counterexample. They do not prove runtime Jacobians, recurrence,
saltation, a numerical spectrum, or a physical polarity law.

The next proof increment is a refinement result connecting the native primal
operation implementations to their emitted Jacobians, followed by
continuous-carrier algebra and flow/event obligations under explicit numeric
and regularity assumptions.

## Gate Discipline

`scripts/verify.sh` is the local and CI authority for the release gate. The
current source registry reports `262/262` expectations, 20 terms, 22 rules, 16
invariants, 46 capabilities, six frameworks, and 4831 native witnesses. Those
are registry counts, not proof totals.

The verifier rebuilds the grammar-1 frontend, bridge, native reducer, unified
driver, persistence, and U2 real-Schur paths; checks generated codebooks and
replay/product artifacts; runs positive, negative, sanitizer, concurrency,
receipt, deterministic, and permanent-mutant cases; invokes
`scripts/verify_u2.sh --skip-formal`; then checks both finite formal families and
compiles the paper when the tools are available. U2 specifically gates source
identity, path tangent, false recurrence, projected-return refusal, explicit
relative restoration, spectrum-after-hold suppression, backend unavailability,
analysis purity, and eight permanent mutation families.

The GitHub Actions workflow
`.github/workflows/ci.yml` installs Lean 4.31.0, Rocq/Coq 9.1.1, and Tectonic
0.16.9 before running `./scripts/verify.sh --require-formal`, so pushes and
pull requests cannot drift from the native witness surface, executable
receipts, finite formal mirrors, generated artifacts, or warning-clean paper.

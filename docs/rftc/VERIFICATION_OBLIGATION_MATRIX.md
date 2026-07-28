# RFTC / C3 Verification Obligation Matrix

Status: D37-D38 implementation complete on the active branch; rapid release,
sanitizer, concurrency, independent epoch-receipt review, and maximal formal
gate pass. PR promotion and exact-head CI remain pending.

This matrix is the claim compiler for the RFTC execution lane. A row is earned
only by the named implementation and permanent counterexample. Passing a
lower row cannot be cited for a higher claim.

## Classification boundary

| Label | Meaning | Current authority |
|---|---|---|
| C0 | deterministic classical simulation | allowed where exact replay is shown |
| C1 | collective classical field computation | earned by the seven-witness RFTC crucible |
| C2 | topologically classified classical logical cells | earned by oriented-boundary and witnessed-transition tests |
| C3-capable local kernel | local runtime contains the mechanisms needed by a distributed C3 deployment | earned by ABI 1.5 D36-D37 tests |
| C3 | distributed BiDi logical-cell runtime | **not earned** until a real multi-host session and reconciliation receipt pass |
| C4 | useful classical computational advantage | **not tested** |
| QS0/QS1 | quantum-simulator semantics, with increasing validation | future quantum ABI lane; never physical evidence |
| QH0 | real hardware execution receipt without a nonclassical witness | future provider/hardware lane |
| R0 | Born/closure measure uniquely derived from declared axioms | open proof program; not earned |
| Q0 | loophole-aware nonclassical physical witness | **not tested** |
| Q1 | quantum computational advantage on an accepted task | **not tested** |
| H0 | executable boundary record-sufficiency mechanism | open Model C/D program; not earned |
| B0 | black-hole foundry reproduces declared information behavior | open physical/model program; not earned |
| G0 | geometry/gravity prediction beyond accepted baselines | open physical/model program; not earned |

## ABI 1.5 mechanism obligations

| ID | Obligation | Implementation | Permanent counterexample | Current evidence |
|---|---|---|---|---|
| F1 | Observation arrival order cannot define topology | `cdc_frame_seal` follows successor links from the lowest member identifier | permuted input and cyclic rotation byte-match | `cdc_cell_test` |
| F2 | A frame is immutable, bounded, fresh, and versioned | `cdc_frame_snapshot` owns a sealed canonical copy | duplicate member, broken ring, undersized, stale, invalid window | `cdc_cell_test` |
| F3 | One frame reduction cannot combine incompatible logical clocks | `cdc_frame_seal` requires one clock/window/frame across the snapshot | one observation carries a mismatched logical clock | `cdc_cell_test` |
| T1 | Winding is orientation-dependent and branch-cut ambiguity is refused | `cdc_topology_classify` | reversed ring and exact `+/-pi` edge controls | `cdc_cell_test` |
| T2 | Sector changes require explicit evidence | `cdc_topology_verify_transition` | changed winding without phase-slip witness | `cdc_cell_test`, `cdc_scheduler_test` |
| T3 | Membership/topology replacement is a frame transition, not local deformation | `cdc_cell_reduce` | skipped/regressed frame version and wrong witness kind | `cdc_cell_test` |
| C1 | Macrostate preserves hidden-state provenance | `cdc_cell_state` digests frame, boundary, sector, hidden class, prior state, and evidence | equal macro values from changed sources retain distinct hidden/state identity | `cdc_cell_test` |
| C2 | Logical clocks, windows, reducer versions, and causal horizons are monotonic | `cdc_cell_reduce` | old clock, window regression, reducer/topology regression | `cdc_cell_test` |
| S1 | Recursive topology is a bounded forest | bottom-up `cdc_scheduler_add_cell` plus one-parent rule | missing child, duplicate parent, depth/cell limit, late graph mutation | `cdc_scheduler_test` |
| S2 | Scheduler configuration is canonical | `cdc_scheduler_seal` hashes canonical cell/spec order | reversed sibling construction yields same seal | `cdc_scheduler_test` |
| S3 | Execution order is deterministic | canonical event key `(clock, kind, cell, member, local sequence)` | different publication orders produce the same state and execution digest | `cdc_scheduler_test` |
| S4 | Composite state cannot outrun incomplete children | recursive same-clock barrier | one child missing/old holds the parent | `cdc_scheduler_test` |
| S5 | Duplicate delivery is idempotent and conflicting duplicate is terminal | committed-observation memory plus event identity | identical replay holds duplicate; changed same-key event conflicts | `cdc_scheduler_test`, `cdc_scheduler_journal_test` |
| S6 | Bounded queues do not mutate on overflow | scheduler capacity reservation | event-limit exhaustion then drain/retry | `cdc_scheduler_test`, `cdc_scheduler_wire_test` |
| S7 | A new configuration epoch has one coherent predecessor and imports atomically | every exported epoch receipt binds its source configuration digest; `cdc_scheduler_import_previous_states` requires every item to match the declared predecessor before mutation | valid per-cell receipts spliced from two predecessor schedulers reject atomically; an invalid mixed changed/unchanged batch leaves every cell unimported; corrected exact retry succeeds | `cdc_scheduler_test`, independent release review |
| S8 | Same-frame carry-forward preserves exact recursive meaning | scheduler-exported epoch receipts bind canonical structure, composite child IDs, and a canonically revalidated cell state | identical exterior root ring with swapped children, tampered structure digest, and tampered amplitude/lineage/transition/state digest reject pre-mutation | `cdc_scheduler_test`, independent D38 scheduler review |
| S9 | Publication, witness delivery, and drain are serialized correctly under real overlap | one scheduler mutex protects queue, witness, reduction, and execution trace | publisher, witness, and drain threads release from one start barrier under ThreadSanitizer | `cdc_scheduler_test` |
| W1 | Command bytes have one canonical representation | bounded `CDCP` v1 encoding | every-byte mutation, truncation, trailing byte, reserved field, phase alias | `cdc_scheduler_wire_test` |
| A1 | Remote commands authenticate and bind authority scope | `RF3W` envelope plus lease-derived supervisor request | wrong key/recipient/frame/action/horizon/quorum/nonce/causal parent | `cdc_control_plane_test`, `cdc_supervisor_test` |
| A2 | Retryable application failure consumes no authority or causal state | typed supervisor `HOLD` | resource/store failure followed by exact same-envelope acceptance | `cdc_supervisor_test`, `cdc_scheduler_journal_test` |
| A2a | Application staleness is retryable, not signed poison | `CDC_SCHEDULER_HOLD_STALE` maps to supervisor `HOLD` | exact stale authenticated command later accepts without changing envelope identity | `cdc_supervisor_test`, `cdc_scheduler_wire_test` |
| A3 | Signed semantic poison cannot block the stream | typed supervisor `REJECT` | malformed authenticated command mutates no app state, consumes its position, next command accepts | `cdc_supervisor_test`, `cdc_scheduler_wire_test`, `cdc_scheduler_journal_test` |
| A4 | Concurrent duplicate admission commits exactly once | supervisor-owned serialization | simultaneous identical envelope | supervisor and journal ThreadSanitizer lanes |
| J1 | Acceptance means the complete authenticated envelope is durable | `cdc_scheduler_journal_admit` stages and seals before supervisor commit | every store-commit failure boundary holds; after recovery exact retry accepts whether the original transaction latched or not | `cdc_scheduler_journal_test` |
| J2 | Restart does not trust the unkeyed store digest as identity | replay decodes and verifies the envelope MAC/identity with the supplied key | wrong key returns `EAUTH` before scheduler mutation | `cdc_scheduler_journal_test` |
| J2a | Restart enforces the configured peer policy, not MAC validity alone | replay reconstructs `cdc_transport_peer` and verifies schema, recipient, key ID, sequence, clock, and causal parent | wrong recipient, wrong key ID, invalid first sequence, and forged first parent return `EPOLICY` with scheduler pristine | `cdc_scheduler_journal_test` |
| J2b | Terminal semantic poison is durable causal history without application mutation | canonical `CDJR` v1 outcome wraps the complete envelope and `REJECT` outcome | replay counts the terminal reject, advances transport causality, enqueues no application event, and accepts the next valid command | `cdc_scheduler_journal_test` |
| J3 | Corrupt committed history is never partially replayed | store full-scan boundary precedes callbacks | flipped sealed byte returns `ECORRUPT`, callbacks/scheduler mutation zero | store replay and scheduler journal tests |
| J3a | A late malformed authenticated command cannot partially replay an earlier valid command | collect and decode the complete exact stream before publishing | valid first envelope plus malformed second envelope returns `EWIRE` with scheduler pristine | `cdc_scheduler_journal_test` |
| J4 | Restart is deterministic across fresh schedulers | full decode then canonical replay order | two fresh schedulers reconstruct byte-identical state/configuration/execution digests | `cdc_scheduler_journal_test` |
| J5 | At-least-once durable delivery is safe | scheduler exact-command deduplication | repeated authenticated commands reconstruct one logical update | `cdc_scheduler_journal_test` |
| J6 | A commitment is not misrepresented as executable history | exact visitor rejects a compacted base | snapshot/compact then journal replay returns `EUNSUPPORTED` while pristine | `cdc_scheduler_journal_test` |

## Gate obligations

| Gate | Required command/evidence | Status |
|---|---|---|
| Rapid release | `./scripts/verify_rftc.sh` | passing on active D38 worktree |
| Memory/UB safety | journal, cell, scheduler, wire, supervisor under ASan/UBSan | passing on active D38 worktree |
| Concurrency | supervisor, scheduler, journal under ThreadSanitizer where supported | passing on active D38 worktree |
| Independent epoch review | permission-to-block reattack of child/state/lineage receipt boundaries | PASS after three found-and-fixed high-severity receipt counterexamples plus one null-output API defect |
| Shipped ABI | strict unified driver contains ABI 1.5 and D37-D38 symbols | passing local strict build and maximal gate |
| Reproducible binary | byte-compare unified driver rounds | PASS on active staged source |
| Formal/native/paper | `./scripts/verify.sh --require-formal` | PASS on active staged source |
| Provenance | regenerated BLAKE3 tracked-file manifest | PASS after staging every new source/evidence file |
| Independent exact-head CI | Linux formal lane plus macOS CDC Studio lane | pending PR |
| Exact-head merge | reviewed PR SHA equals merged source | pending |

## Promotion refusals

The following are categorical failures, regardless of other green tests:

1. labeling a simulator, classical cluster, circuit-cut result, or learned
   controller as Q0/Q1;
2. labeling the local ABI 1.5 kernel as C3 without a multi-host execution
   receipt and deterministic reconciliation replay;
3. treating a keyed shared-secret MAC as public-key identity,
   non-repudiation, or mTLS;
4. replaying a compacted commitment as though it contained discarded events;
5. averaging mechanism witnesses so one failed obligation is hidden;
6. allowing a UI or product surface to promote a claim beyond the evidence
   record it renders.

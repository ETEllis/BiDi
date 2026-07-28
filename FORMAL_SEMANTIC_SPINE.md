# Formal Semantic Spine

> **Current projection (2026-07-28):** the typed Grammar-1 AST, collecting
> diagnostics, canonical serialization, fused runtime, durable store, receipts,
> and ABI 1.4 now exist. This document remains the proof/semantic spine; older
> future-tense frontend language below should be read as the design that led to
> those accepted surfaces.

This document pins the next formalization pass to one canonical object. CDC is
the language; this spine is the semantic kernel that keeps `.cdc`, the paper,
the bootloader, and the witnesses from drifting into parallel descriptions.

## Principle

Every artifact should become a projection of the same semantic spine:

```text
.cdc source
  -> typed AST
  -> runtime state tuple
  -> small-step relation
  -> invariant table
  -> native witnesses
  -> finite proof artifacts
  -> theorem-prover obligations
```

The current repository has a native `.cdc` contract/witness suite, canonical C
frontend, stable ABI, and fused runtime. The one Python file is a frozen
differential oracle. The refinement is now to make additional reducer code and
proofs generated from, or audited against, the same native source.

Trace/window semantics are derived from this spine. They do not add a fourth
foundational step kind. They name causal observer windows, trace spans, and
measurement records over `flow`, `commit`, and `nest`.

## AST

The AST should represent only the canonical calculus primitives:

- `CellTerm`: phase, amplitude, intrinsic frequency, plasticity, optional latch.
- `ModuleTerm`: named cell vector plus belief, prior, precision, and action gain.
- `ChannelTerm`: source path, destination path, weight, delay, angular phase bias,
  optional line projection, and plasticity flag.
- `FieldTerm`: modules, channels, child fields, timestep, gain, deadband, gating.
- `CounterTerm`: register-machine witness term for universality.
- `WindowSpec`, `TraceSpan`, `ObserverSpec`, `MeasurementRecord`,
  `WindowPolicy`, `AgencySummary`, and `IncidenceSpec`: derived observer,
  measurement, local-counter, and policy records over the same field state.

`kernel.cdc` now declares the canonical term surface. The native target is for
the same spine to continue growing as `.cdc` terms and transition rules while
`cdc_boot.py` remains only mechanical bootstrap code. See
`NATIVE_SELF_HOSTING_MANDATE.md`.

## Runtime State Tuple

A runtime state is:

```text
S = (t, modules, channels, child_fields, inbound_relations, deadband, dt, gain, gated)
```

Each module state carries:

```text
M = (name, cells, belief, prior, precision, action_gain, child)
```

Each cell state carries:

```text
x = (theta, amplitude, plasticity, omega, sigma, memory)
```

This tuple is the intended bridge between mathematical notation and future
native reducer code. The semantic spine names the immutable shape the native
runtime must instantiate.

## Small-Step Relation

The canonical reduction relation has three step kinds:

| step | source form | meaning |
|---|---|---|
| `flow(d)` | `F ->_d F'` | evolve phase, amplitude, belief, and plasticity for duration `d` |
| `commit(m)` | `F ->_beta F'` | guarded quantize/barrier/belief/latch update for module `m` |
| `nest(m)` | `m[[F]]` | exchange parent context downward and child coherence upward |

Operationally, nesting is represented as two automatically installed
path-aware relations at angular bias `alpha=0`: one aggregate child-to-parent
up-cone and one aggregate parent-to-child down-cone for each child module.
General channels may use nonzero `alpha` and projected `lines`, so the original
parent/child cone is the neutral case of the same relation operator.

A future reducer may integrate flow numerically, but the semantic relation is
the source of truth. Numerical choices should be recorded as realization
parameters, not confused with the calculus definition.

## Derived Trace/Window Layer

The trace/window layer records how a bounded observer window sees a field:

```text
phase-time      continuous flow and rotation
event-time      ordered guarded commits
trace-time      phase/event history through a window
```

Trace-time is local. Smooth phase motion can accumulate with zero commit events,
and event density can differ by window. A global tick is a realization detail,
not part of the semantic spine.

The discrete outcome space is balanced ternary: `-1`, `0`, `+1`. The middle
value is resting equilibrium and a real crossing/aperture state, not binary
false. A committing measurement is a guarded balanced-ternary commit plus a
`MeasurementRecord`; passive observation produces a `TraceSpan` and leaves field
dynamics unchanged.

Window policy is recursive but not foundational. It can update the sampling,
commit, adaptation, or projected-state policy for a bounded window while still
reducing through `flow`, `commit`, and `nest`.

## Derived Universal Closure

`universal-close` is a derived rule, not a fourth step kind. Its state record
is the **lifted frame**:

```text
lifted-frame       (projected-phase mod 2*pi, winding count, Z2 sheet, holonomy)
universal-record   the bridge coordinate computed from the closed frame's trace
```

The rule composes the three foundational steps with trace projection, council
decision, and source evolution under one acceptance guard: reciprocal
receptive/radiant cones active in one flow evaluation, half-turn projection
return with sheet inversion, full 720-degree return with sheet restoration and
winding two, holonomy agreement, accepted local commits, and record/decision
coordinate equality — enacting only the runtime-computed record. The finite
sheet-parity lemmas (one turn inverts the Z2 sheet, two turns restore it) are
mechanized in `formal/lean/CDCFinite.lean` and `formal/coq/CDCFinite.v`;
continuous frame transport and holonomy preservation remain queued.

## Typed Invariant Table

Each invariant should have:

- a stable key;
- a mathematical statement;
- the witness file and witness name;
- a finite proof artifact where one exists and a theorem-prover target where it
  remains open.

`laws.cdc` now declares invariant keys and witness links for:

- `gate-abelian`;
- `interfere-monoid`;
- `rotation-linear`;
- `corefold-morphism`;
- `balanced-ternary-carrier`;
- `dyadic-triadic-closure`;
- `existence-viability`;
- `trace-order-locality`;
- `preservation`;
- `soundness`;
- `local-confluence`;
- `flow-additivity`;
- `normalforms`;
- `universal-closure`.

## Projection Targets

### `.cdc`

The native reducer pilot now parses `native_reducer.cdc` into an executable
runtime state and emits a small reducer IR for source-declared `flow`, `commit`,
and `nest` jobs. The broader parser should eventually produce the AST directly
rather than executing line by line. The current `cdc_boot.py` can then become:

```text
parse .cdc -> ProgramTerm -> initialize RuntimeState -> reduce -> check expects
```

The stronger self-hosting target is:

```text
parse .cdc -> native kernel terms -> native reducer transitions -> native expects
```

At that point a host language is no longer the semantic center; it is only one
replaceable loader for a language that can describe itself.

### `cdc_boot.py`

The bootloader must remain a loader/checker only: read `.cdc`, collect
declarations, verify expectations, and report. It must not grow reducer
semantics back into Python.

### Native Witness Files

`laws.cdc`, `bridge64.cdc`, `bridge_codebooks.cdc`, `bridge512.cdc`,
`bridge4096.cdc`, `bridge_jobs.cdc`, `native_reducer.cdc`,
`native_surface.cdc`, `council_bridge.cdc`, `system.cdc`, `relations.cdc`, and
`trace_windows.cdc` should remain the native
witness surface. Each witness declares the invariant or capability it discharges.

`runtime/cdc_bridge_runtime.c` is the first operational consumer outside Python:
it reads `bridge64.cdc`, validates the finite table, performs lookup, projects
trace occupancy into bridge coordinates, executes source-declared jobs from
`bridge_jobs.cdc`, regenerates/verifies the `bridge512.cdc` and `bridge4096.cdc`
higher-arity codebooks, and emits the visible interactive 64-cell grid.

`runtime/cdc_native_runtime.c` is the first operational reducer consumer outside
Python: it reads `native_reducer.cdc`, executes source-declared flow, commit,
and nest jobs, emits reducer IR, interprets that IR, and checks the finite n=6
balanced-ternary walk spectrum. It also consumes `native_surface.cdc` to
exercise guard, trace, measure, policy, bridge, and counter clauses, and consumes
`council_bridge.cdc` to exercise source-declared council deliberation and
bridge-coordinate source evolution.

### Paper

The paper should present the same AST/state/reduction/invariant table, then
state which parts are currently native witness declarations and which parts are
future formal proof obligations.

### Lean/Coq/Kani

The first formal target is now represented three ways: native C proof checking,
Lean source, and Coq source for the finite n=6 balanced-ternary carrier layer
plus finite algebraic laws. The checked finite layer covers:

1. trit quantization;
2. prefix-walk admissibility;
3. commit barrier preservation;
4. localized normal forms;
5. finite gate associativity/commutativity/identity/inverse;
6. finite interference associativity/commutativity/unit;
7. finite rotation linearity.

The next formal step is to extend those artifacts from finite carrier and
finite-algebra witnesses into commit-barrier preservation, then the flow
relation under explicit Lipschitz/determinism assumptions.

## Acceptance Criteria For The Next Formal Pass

- the accepted Grammar-1 AST remains the unique canonical parse projection;
- `cdc_boot.py` remains a frozen differential oracle and accumulates no reducer
  or product semantics;
- RFTC dynamic state projects through the same runtime tuple and guarded
  relation rather than creating a parallel semantics;
- store receipts and downstream Memory Manifold decisions carry replayable
  closure evidence through the ABI;
- `native_reducer.cdc` and `native_surface.cdc` keep executable flow/commit/nest,
  guard, trace, measure, policy, bridge, and counter clauses synchronized with
  the native runtime.
- `bridge64.cdc` stays as the explicit finite bootstrap codebook and the C
  bridge runtime stays a verified consumer of that source.
- `bridge_codebooks.cdc` records the higher-arity growth rule for `n=9` and
  `n=12`; `bridge512.cdc` and `bridge4096.cdc` contain the full generated rows
  and must match runtime regeneration.
- relation witnesses cover angular phase, dimension projection, path endpoints,
  and `.cdc` nesting auto-cone installation.
- trace/window witnesses cover passive observation, committing measurement,
  trace additivity, causal windows, observer roles, incidence projections,
  local counters, recursive window policy, coupled observation, agency summaries,
  trace-order locality, and projected higher-order boundaries.
- every witness in `.cdc` references an invariant or capability key.
- the paper's invariant table matches `laws.cdc`.
- deleted host files stay deleted; replacements live in `.cdc`.
- `scripts/verify.sh` proves code, `.cdc`, witnesses, native reducer execution,
  finite proof checks, and semantic registry stay synchronized.

This is the path from native contract calculus to theorem-prover-ready calculus
without losing the working system.

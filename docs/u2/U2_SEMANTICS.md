# CDC U2 Semantics: Variational Universal Operator and Hybrid Monodromy

Status: normative implementation contract for U2

Source baseline audited: `a4ce53bdb63ad39c77bd5bfb248239c33d573d9b`

Scope: native CDC state evolution, tangent execution, recurrence qualification,
hybrid-event sensitivity, monodromy, and characteristic multipliers

## 1. Decision

U2 is a **derived analysis layer over the executed CDC reduction path**. It does
not add a fourth foundational reduction. `flow`, `commit`, and `nest` remain the
only primitive state reductions; `universal` remains their guarded, lifted
closure. U2 adds:

1. a deterministic state-coordinate manifest;
2. the derivative of an accepted executed path, `D𝒰`;
3. explicit guard/reset sensitivity, including saltation at state-triggered
   events;
4. an independently verified recurrence or relative-recurrence gate;
5. a monodromy operator only after that gate passes; and
6. characteristic-multiplier diagnostics with typed failure states.

The central fail-closed rule is:

> Tangent execution does not imply recurrence; lifted-cover closure does not
> imply full-state recurrence; and eigenvalues of a non-returning path are not
> Floquet multipliers.

The current `loop-u720` witness earns U1 closure of its lifted cover, accepted
local commits, record/decision agreement, and guarded enactment. It does **not**
currently establish a periodic orbit of the full native state: in particular,
the context belief and child prior accumulate across the two cycles, and cells
that begin unlatched end with populated latches. U2 may
differentiate that accepted path, but it must hold any monodromy/Floquet claim
until full or explicitly justified relative recurrence is verified.

Normative words `MUST`, `MUST NOT`, `SHOULD`, and `MAY` have their usual
requirements meaning.

## 2. Executable authority and current-state partition

The derivative MUST follow the native executor, not a richer prose model. At
the audited baseline, `runtime/cdc_native_runtime.c` has the following exact
partition.

### 2.1 Dynamic continuous coordinates

For U2 v1, the continuous dynamic vector is ordered by source declaration
within the selected Universal's executable closure:

\[
x = (\theta_1,\ldots,\theta_{N_c},
     b_1,p_1,\ldots,b_{N_m},p_{N_m}) \in \mathbb R^n,
\]

where:

- `theta` is each cell phase;
- `belief` is each module belief; and
- `prior` is each module prior.

These are the only real-valued coordinates mutated by the current `flow` and
`nest` executor. The native v1 closure is derived mechanically from the exact
U1 execution and post-step reads:

1. every `flow` selects every cell in its field;
2. every `commit` selects its module and that module's cells;
3. every `nest` selects both modules and their cells;
4. the Universal selects its lifted cover cell, the field read by its selected
   record bridge, and the modules read by its selected council deliberation;
5. selecting a cell closes over its owning module, and selecting a module
   closes over all its cells; and
6. selected cells and selected module `belief`/`prior` pairs are emitted in
   original source declaration order.

Unreferenced declaration islands are not tangent coordinates and cannot add
inert unit multipliers. The manifest records the exact coordinate order,
global source indices, dimensions, topology, and digest. U2 v1 has no syntax
for requesting a larger closure; adding one requires a separately specified
source contract rather than silently widening the state.

### 2.2 Discrete mode

The discrete mode is:

\[
q=(h_1,\sigma_1,\ldots,h_{N_c},\sigma_{N_c},\text{event itinerary}),
\]

where `has_latch` is `h_i` and `latch` is the balanced-ternary symbol
`sigma_i ∈ {-1,0,+1}`. Source topology, field/module membership, and the ordered
reduction program are fixed realization structure, not tangent coordinates.

`projected phase`, `winding`, and `Z2 sheet` are currently derived from a cover
cell's unwrapped phase relative to its initial value. They are not separately
stored continuous coordinates. Holonomy is currently the declared reciprocal
channel-angle sum, not an independently integrated state.

### 2.3 Frozen parameters and non-dynamic fields

At the audited baseline, the following are configuration parameters or inert
runtime fields for purposes of `D𝒰`, not dynamic state:

- cell `amplitude` and `omega` (`omega` drives phase but is not updated);
- module `precision` and `action_gain`;
- field `dt`, `gain`, and `deadband`;
- channel `weight`, `delay`, `angle`, `lines`, endpoints, cone, and pair; and
- source jobs, expectations, witness records, counters, bridges, councils, and
  enactment paths.

This is intentionally narrower than the aspirational semantic prose, which
describes amplitude relaxation, belief flow, plasticity, delay history,
openness gating, line projection, and off-grid event location. Those features
MUST NOT silently enter U2 mathematics until they enter the primal executor.
A state ABI/version change MUST regenerate the coordinate manifest and all U2
fixtures.

Nonzero delay requires a history state (generally an infinite-dimensional or
discretized history space), so `delay != 0` MUST yield
`held:unsupported-delay-state` until the primal runtime implements and declares
that state. Any other source attribute that purports to affect dynamics but is
not consumed by the selected executor MUST be disclosed in the receipt and,
when material to the claimed model, MUST hold as `unsupported-primal-semantic`.

## 3. Hybrid state model

U2 interprets an executable CDC path as a hybrid system

\[
\mathcal H=(Q,\{X_q\}_{q\in Q},f_q,E,\{g_e\},\{R_e\}),
\]

with discrete modes `q`, continuous state spaces `X_q`, smooth within-mode
dynamics or executable maps, directed events `e`, scalar guard surfaces
`g_e(t,x)=0`, and reset maps `R_e(t,x)`.

The first implementation MUST require constant continuous dimension along a
cycle. Rectangular saltation maps MAY be recorded for dimension-changing
transitions later, but a square monodromy spectrum is forbidden unless the
returned tangent space is canonically identified with the initial one.

The analyzed state map is the state component of the Universal Operator:

\[
\mathcal U_X:(q_0,x_0)\mapsto(q_T,x_T).
\]

Trace, bridge, council, and enactment are acceptance/readout/effect layers.
They may accept or hold `𝒰_X`; external file mutation has no derivative and MUST
occur only after primal and U2 acceptance. U2 analysis SHOULD run against an
effect-free snapshot, then allow the existing guarded enactment path to proceed.

## 4. Exact derivatives of the audited native maps

### 4.1 `flow`

The current executor applies one synchronous finite-duration map, not a `dt`
substep integrator. For a cell `i` in the selected field,

\[
\theta_i' = \theta_i + \omega_i d
  + Gd\sum_{e:\,t(e)=i}w_e
     \sin(\theta_{s(e)}+\alpha_e-\theta_i),
\]

where only channels whose source and target cells share that field contribute.
Beliefs and priors are unchanged by this map. Its exact phase Jacobian is

\[
\frac{\partial\theta_i'}{\partial\theta_\ell}
=\delta_{i\ell}+Gd\sum_{e:\,t(e)=i}w_e
\cos(\theta_{s(e)}+\alpha_e-\theta_i)
(\delta_{s(e)\ell}-\delta_{i\ell}).
\]

This formula correctly gives zero coupling derivative for a self-loop's phase
difference. U2 MUST differentiate the synchronous pre-step state, matching the
executor's `next_theta` behavior. It MUST NOT differentiate an imagined exact
ODE solution and call it parity with this map.

When the primal runtime later gains a numerical ODE integrator, the within-mode
state-transition matrix is instead obtained from the variational equation

\[
\dot\Phi(t)=D_x f_q(t,x(t))\Phi(t),\qquad \Phi(t_0)=I,
\]

using the same accepted steps, tolerances, and event localizer as the primal
trajectory. That realization change requires a versioned receipt.

### 4.2 `commit`

At the audited baseline, an accepted commit evaluates

\[
\tau(\theta_i)=
\begin{cases}
+1 & \cos\theta_i>\delta,\\
-1 & \cos\theta_i<-\delta,\\
0  & \text{otherwise},
\end{cases}
\]

checks the nonnegative-prefix barrier, and writes the latches. It does not
change phase, belief, or prior. Therefore, within a fixed accepted trit/mode
itinerary, its continuous reset is the identity and `D_xR=I`. A held commit
terminates the Universal Operator and has no accepted tangent result.

The quantization boundaries are nonsmooth. A tangent is valid only when the
nominal path and all verification probes retain the same trit vector, barrier
outcome, and event order. Otherwise U2 MUST hold with
`nondifferentiable-quantization` or `itinerary-diverged`.

If the primal commit later implements the documented snap, belief update, or
other continuous reset, `R_e` and `D_xR_e` MUST be explicit executable objects;
the identity rule MUST then cease to apply.

### 4.3 `nest`

Let `u(q)` be the mean child trit selected from latches when present and current
quantization otherwise. The audited native map is

\[
b_p' = b_p + G u(q),\qquad p_c'=b_p',
\]

with every other continuous coordinate unchanged. Within a fixed discrete
mode, `u(q)` is constant. Consequently:

- `∂b_p'/∂b_p = 1`;
- `∂p_c'/∂b_p = 1`;
- `∂p_c'/∂p_c = 0`; and
- phase derivatives are zero away from a quantization boundary.

The overwritten child-prior row is therefore not an identity row. This detail
MUST be covered by an analytic-versus-finite-difference fixture.

## 5. Guard, reset, and saltation semantics

The current `guard` declaration reports an open/closed state but is not bound to
a commit as an off-grid event surface. Scheduled commits therefore use `D_xR`
only; applying a saltation correction to them would be false precision.

To earn hybrid-event sensitivity, the primal language/runtime MUST add an
explicit event binding with all of:

- source mode and destination mode;
- a scalar guard `g_e(t,x)`;
- crossing direction (`rising`, `falling`, or a separately justified `either`);
- reset `R_e(t,x)`;
- deterministic priority or an explicit simultaneous-event composition rule;
- localized event time and pre/post states; and
- analytic or verified automatic derivatives of `g_e` and `R_e`.

For a transversal state/time-triggered event, write

\[
n^\top=D_xg_e(t_e,x^-),\quad
d_e=n^\top f^-(t_e,x^-)+\partial_t g_e(t_e,x^-).
\]

The sensitivity jump is the saltation matrix

\[
S_e = D_xR_e +
\frac{\left(f^+ - D_xR_e f^- - \partial_tR_e\right)n^\top}
     {n^\top f^-+\partial_tg_e}.
\]

All quantities are evaluated on the nominal event, with `f+` evaluated at the
reset state in the destination mode. Sign conventions for the guard may change
both numerator-normal and denominator signs, leaving `S_e` unchanged; the
runtime MUST pin one convention in its receipt and fixtures.

For a fixed scheduled event, `D_xg=0`, so the correction vanishes and the jump
is `D_xR`. Substituting a reset Jacobian for `S_e` at a genuine state-triggered
event is forbidden because it drops event-time sensitivity.

The event MUST hold when:

- `|d_e|` is below the declared transversality tolerance (grazing);
- two guards occur within the simultaneous-event tolerance without a declared
  composition order;
- the localizer cannot bracket or converge;
- the reset or guard derivative is missing/nondifferentiable;
- a perturbation changes the mode itinerary;
- event accumulation exceeds the finite-event budget (possible Zeno behavior);
  or
- the nominal and perturbed paths have incompatible dimensions.

This contract follows the standard first-order hybrid sensitivity treatment in
Kong et al. and the earlier trajectory-sensitivity jump treatment of Hiskens and
Pai; references appear in Section 13.

## 6. Tangent execution versus recurrence

### 6.1 Tangent execution

For any accepted path with a stable mode itinerary, U2 may emit

\[
D\mathcal U_X(x_0):T_{x_0}X_{q_0}\rightarrow T_{x_T}X_{q_T}.
\]

This is useful even when the path is not periodic. Its receipt MUST say
`analysis=tangent` and MUST NOT use `monodromy`, `Floquet`, `periodic`, or
`stability-of-orbit` as earned labels.

If recurrence later verifies and binds a return operator, the analysis level
advances to `analysis=monodromy`. A downstream spectral backend or validation
hold after that point does not demote the receipt back to `tangent`; it keeps
the monodromy artifact and withholds only multipliers/classification beyond the
typed held surface.

### 6.2 Absolute recurrence

Absolute recurrence requires:

1. `q_T=q_0`, including latch state and event itinerary class;
2. the same continuous-coordinate manifest at both endpoints; and
3. a weighted residual

\[
r=\|x_T-x_0\|_W
\]

below an explicitly recorded absolute-plus-relative tolerance.

Phase residuals MUST use the declared topology: unwrapped distance for lifted
coordinates and shortest-circle distance only for coordinates declared modulo
`2π`. Discrete sheet restoration cannot substitute for a continuous-state
residual.

### 6.3 Relative recurrence

Relative recurrence requires an explicit, executable endpoint restoration map

\[
\rho_\gamma:X_{q_T}\rightarrow X_{q_0}
\]

representing a declared symmetry, gauge action, phase return, or drift action:

\[
r_\gamma=\|\rho_\gamma(x_T)-x_0\|_W\le\varepsilon.
\]

The runtime MUST verify that `rho_gamma` is a symmetry/equivariance of every
operation in the selected path. Merely omitting changed coordinates is a
projection, not relative recurrence. A projected return may be reported as
`analysis=projected-tangent`, but it cannot authorize Floquet language for the
full system.

For the current `cdc.u2.stability.v1` machine-consumer contract, every
nonprojected recurrence that can carry an earned recurrence claim is recomputed
directly from the emitted initial/final executable states, the coordinate
periods recorded in the manifest (shortest-circle distance only for declared
periodic coordinates and unwrapped difference for lifted coordinates), current
v1 unit weights, the bound absolute-plus-relative tolerances, and any admitted
relative endpoint restoration. The emitted `residual`, `normalizedResidual`,
`verified`, `discreteStateVerified`, and `authorizesMonodromy` fields are part
of that recomputed contract and must match the runtime's measurement rather
than being treated as narrative metadata.

Projected receipts do not expose the v1 include mask that produced the
projection. The core may mark its masked recurrence diagnostic `verified` when
the included coordinates and discrete state pass, but a consumer cannot
independently recompute that bit from the receipt. CDC Studio therefore admits
only projected receipts whose producer diagnostic is internally consistent and
false. Those consumer-admitted receipts remain held, consumer-unverified,
`authorizesMonodromy=false`, and unable to expose monodromy or spectrum.
Likewise, a would-be relative receipt with no admitted executable restoration,
no verified restoration equivariance, or no bound restoration derivative is a
typed non-authoritative hold rather than an earned relative recurrence claim.

For `loop-u720`, the cover's `4π` phase return is a candidate declared phase
restoration. The accumulating context belief and child prior still require
absolute return or a separately proved drift symmetry. No such quotient is
earned merely by U1's current closure receipt.

### 6.4 Poincare sections and neutral modes

A true first-return Poincare map may restrict the tangent to a transversal
section and thereby remove the flow-direction multiplier. A fixed-duration or
externally forced map does not automatically carry a neutral multiplier at
`1`. U2 MUST NOT discard a unit multiplier merely because Floquet analyses often
contain one. Every removed phase, gauge, or symmetry mode must be declared and
verified by tangent alignment with the corresponding generator.

## 7. Ordered tangent and monodromy products

For an executed sequence

```text
flow Phi_1 -> event S_1 -> flow Phi_2 -> ... -> event S_N -> flow Phi_(N+1)
```

column perturbations propagate by left multiplication:

\[
D\mathcal U_X =
\Phi_{N+1}S_N\Phi_N\cdots S_2\Phi_2S_1\Phi_1.
\]

Every scheduled `commit`/`nest` map appears in the same source order using its
own Jacobian. The implementation SHOULD internally treat all operations as an
ordered sequence `J_k` and update `A <- J_k A`, initialized by `A=I`; this
prevents a special-case reversal bug.

For verified absolute recurrence, the monodromy is

\[
M=D\mathcal U_X(x_0).
\]

For verified relative recurrence,

\[
M_\gamma=D\rho_\gamma(x_T)\,D\mathcal U_X(x_0).
\]

A two-event fixture with noncommuting analytic matrices MUST permanently gate
the multiplication order.

## 8. Multipliers and classification

Characteristic multipliers are the eigenvalues `mu_i` of a verified square
`M` or `M_gamma`. U2 MUST retain complex-conjugate pairs, deterministic ordering,
matrix/eigenvalue residuals, the coordinate-manifest digest, and the recurrence
receipt.

With classification band `tau > 0`, after separately verified neutral modes
are removed:

- `stable`: every remaining `|mu_i| < 1 - tau`;
- `unstable`: at least one remaining `|mu_i| > 1 + tau`;
- `marginal`: none is outside and at least one is in
  `[1-tau,1+tau]`; and
- `held`: recurrence, numerical, conditioning, or semantic prerequisites fail.

If recurrence has already authorized `M` or `M_gamma`, a later spectral hold
still lives at `analysis=monodromy`: the receipt may retain the return
operator while emitting no multipliers/backend classification.

For an autonomous periodic orbit, `stable` means **local orbital asymptotic
stability** only when the expected flow-direction neutral mode is verified and
all transverse multipliers are inside the unit circle. It never means global
stability. For a discrete/stroboscopic map, the classification applies to its
verified fixed point without inventing a neutral mode.

Floquet exponents `lambda_i = log(mu_i)/T` MAY be emitted as secondary data only
with period `T`, branch convention, and `mu_i=0` handling declared. Multipliers
are the canonical U2 output.

### 8.1 Scoped polarity covariance

Polarity is a derived analysis witness, not a fourth reduction and not an
automatic consequence of naming channels receptive/radiant. The implemented
`cdc_u2_polarity_covariance_check` accepts a selected realization only when an
explicit source/target involution fixes its declared aperture and satisfies

\[
\rho_1\mathcal U_X(x)=\mathcal U_X^\rho(\rho_0x),
\qquad
D\rho_1 D\mathcal U_X=D\mathcal U_X^\rho D\rho_0.
\]

The gate emits separate scale-normalized residuals for both involutions,
aperture fixation, primal closure covariance, and tangent covariance. A
symmetry-breaking realization remains valid CDC but receives a typed held
polarity witness. The current analytic fixture proves the generic mechanism;
it does not establish covariance of `loop-u720`, spectrum equivalence, or a
physical polarity law. A future source-level binding MUST identify the actual
conjugate Universal Operator and may compare spectra only after both sides pass
their independent recurrence gates.

## 9. Typed holds

U2 uses accepted/held analysis outcomes. It MUST NOT emit a multiplier spectrum
after any hold.

The exact current `cdc.u2.stability.v1` reason strings that the native runtime
can emit are:

| current v1 reason | emitted when |
|---|---|
| `primal-held` | the selected U1 execution or a local commit held before snapshot admission |
| `unsupported-primal-semantic` | the selected executable path requests semantics the current primal runtime does not admit as a differentiable U2 step |
| `unsupported-delay-state` | a selected `flow` depends on nonzero channel delay without a declared history state |
| `state-manifest-mismatch` | the executable closure or coordinate identity cannot be built coherently for the selected path |
| `nondifferentiable-quantization` | a selected scheduled commit sits on or too near a trit boundary |
| `invalid-argument` | a variational helper received an invalid executable argument in the admitted path |
| `dimension-mismatch` | the emitted executable state or Jacobian dimensions do not match the manifest |
| `allocation-failed` | a required tangent or monodromy artifact could not be allocated |
| `nonfinite-variational-state` | the admitted executable state or local variational data is non-finite |
| `recurrence-mode-mismatch` | terminal selected-cell latch state does not restore |
| `recurrence-residual` | full or declared-relative recurrence residual exceeds tolerance |
| `undeclared-quotient` | changed coordinates were omitted without an admitted executable restoration |
| `spectral-backend-unavailable` | recurrence authorized monodromy but no Schur backend is available |
| `spectral-backend-failed` | recurrence authorized monodromy but the Schur backend failed |
| `variational-validation-failed` | recurrence authorized monodromy but Schur residual validation failed |

The following strings belong to the broader U2 status vocabulary but are not
currently emitted by the source-bound `cdc.u2.stability.v1` runtime on Friday,
August 7, 2026. They remain reserved target obligations until the primal
runtime and source language execute those features:

| reserved target reason | not currently emitted because |
|---|---|
| `guard-event-not-localized` | current CDC commits are scheduled; no source-bound localized guard-event transition exists |
| `event-not-transverse` | the source runtime does not yet drive an off-grid guard/reset event through `stability` |
| `itinerary-diverged` | no whole-path perturbation-oracle receipt currently runs on source executions |
| `event-budget-exhausted` | no localized guard-event executor or event-budget accounting is emitted in v1 |
| `neutral-mode-unverified` | no declared neutral-mode removal request is currently admitted in source-bound receipts, so the runtime never reaches this hold today |

Parser/configuration errors remain hard errors rather than typed receipt holds.
Dynamical conditions that invalidate the requested analysis remain typed holds.

## 10. Deterministic receipt contract

Every current `cdc.u2.stability.v1` artifact MUST bind the fields authorized by
its analysis stage; v1 does not guarantee that every receipt reaches endpoint,
tangent, recurrence, or spectral artifacts.

Stage-by-stage, the current source-bound receipt shapes are:

- **U1 pre-snapshot hold**: `analysis=held`. The receipt preserves the U1
  result and its causal reason, but manifest, discrete-state, dimension,
  coordinates, endpoint state, tangent, recurrence, monodromy, and spectral
  fields are null or empty.
- **Layout-admission early hold**: `analysis=held`. U1 admitted the source
  snapshot, but the runtime could not build a coherent executable closure or
  state manifest. In this shape the source digest, versioned runtime identity,
  and U1 result remain bound, but manifest, dimension, coordinates, endpoint
  state, discrete-state artifact, tangent, recurrence, monodromy, and spectral
  fields are null or empty. The exact current emitted reason in this shape is
  `state-manifest-mismatch`.
- **Accepted U1 / pre-tangent source-bound hold**: `analysis=held`. The
  executable closure was admitted strongly enough to bind the source digest,
  versioned runtime identity, U1 result digest, manifest digest, dimension, and
  coordinates, but endpoint state, discrete-state artifact, tangent,
  recurrence, monodromy, and spectral fields remain null because no tangent was
  completed. Current emitted reasons in this shape include
  `unsupported-primal-semantic`, `unsupported-delay-state`,
  `nondifferentiable-quantization`, `invalid-argument`, `dimension-mismatch`,
  `allocation-failed`, and `nonfinite-variational-state` when they occur after
  layout admission but before a full tangent receipt binds.
- **Tangent-stage hold**: `analysis=tangent`. Endpoint state, discrete-state
  artifact, path-tangent digest, and tangent matrix are present; recurrence,
  monodromy, and spectral fields remain null.
- **Recurrence-authorized / downstream spectral hold**: `analysis=monodromy`.
  Recurrence and monodromy are bound; multipliers, backend, spectral
  diagnostics, and accepted stability classification remain null/held.
- **Accepted spectral receipt**: `analysis=monodromy` with full recurrence,
  monodromy, multiplier, and spectral diagnostics.

For every nonprojected recurrence-bearing receipt in the current v1 runtime,
the machine consumer can and should independently recompute the earned
recurrence claim from:

- `initialState` and `finalState`;
- the per-coordinate manifest `period` topology;
- implicit unit weights (current v1 emits no weight vector and uses `1.0` for
  every included coordinate);
- `tolerances.recurrenceAbsolute` and `tolerances.recurrenceRelative`;
- the relative restoration artifact when `recurrence.kind=relative`; and
- the selected-cell discrete-state verification bound into
  `recurrence.discreteStateVerified`.

The recomputed raw and normalized residuals must agree with the emitted
`recurrence.residual` and `recurrence.normalizedResidual`, and the resulting
verification/authorization decision must agree with
`recurrence.verified` and `recurrence.authorizesMonodromy`.

Projected receipts are intentionally weaker in current v1: they expose
`recurrence.scope=projected` but do not expose the include-mask that produced
that projection. The core diagnostic may be verified over its hidden mask;
CDC Studio cannot independently reproduce that result and therefore admits
only internally consistent false diagnostics. Consumer-admitted projected
receipts remain held, consumer-unverified, non-authorizing, and unable to
expose monodromy or spectrum. The same held, unverified, non-authorizing rule
applies to non-executable relative receipts with no admitted restoration
action, no verified restoration equivariance, or no bound restoration
derivative.

The executable restoration derivative array, not its label, participates in
authorization. Current CDC Studio validates the exact v1 identity matrix and
requires a canonical BLAKE3 spelling for `derivativeDigest`, but does not
independently recompute that digest. The native digest is therefore receipt
provenance metadata at this boundary rather than a second authorization input.
The generated web console additionally pins the known current 13-by-13 identity
digest used by its bounded accepted fixture.

Within those stage limits, every current `cdc.u2.stability.v1` artifact MUST bind:

- the digest of the exact grammar-1 canonical statement stream captured from
  the parsed unit before its AST is released, a versioned runtime-algorithm
  identity, and the canonical U1 result digest (a binary/build provenance
  digest MAY additionally bind a release artifact at packaging time); the
  runtime MUST NOT re-read the source path later and call those bytes the
  executed source;
- state-manifest digest and complete coordinate list;
- ordered operation/event itinerary and event times;
- primal endpoint states and selected-cell discrete mode identities, with
  explicit per-cell entries, deterministic initial/final discrete-state
  digests, and a recurrence-bound `discreteStateVerified` flag;
- analytic method per operation at usable granularity; the current v1 receipt
  records the executable flow/commit/nest method family, the unbound guard
  boundary, and a more detailed method subobject rather than claiming source
  features it did not execute;
- tolerances, determinism settings, and explicit null/not-applicable fields for
  whole-path finite-difference receipts, localized event budgets, and
  neutral-mode removal until the primal runtime executes those features;
- tangent/monodromy matrix digest and full matrix artifact;
- tangent artifact whenever `analysis=tangent`; monodromy artifact whenever
  recurrence has authorized the return operator, even if a downstream spectral
  stage later holds;
- recurrence kind, restoration map, residual components, and verdict; a
  relative receipt additionally carries the restoration action and coordinate,
  displacement, explicit equivariance and U1 section witnesses, the complete
  row-major restoration derivative, and its digest;
- eigenvalues, spectral radius, validated Schur residuals/tolerance, declared
  neutral modes, ordering rule, and classification when the spectral stage
  succeeds; backend-unavailable/backend-failed/validation holds keep
  `analysis=monodromy`, the monodromy artifact, and null multiplier fields; and
- accepted/held status with exactly one primary typed reason.

The command may present human-readable lines, but the machine receipt is the
authority. Re-running the same source/runtime/configuration MUST produce the
same manifest, itinerary, matrix digest, and deterministically ordered
multipliers within the declared numeric serialization contract.

If U1 holds before snapshot admission, there is no U2 state artifact. The
receipt preserves the U1 result and its causal reason, but the manifest,
recurrence, discrete-state artifact, dimension, coordinates, endpoint state,
tangent, events, monodromy, and spectrum fields MUST be null or empty rather
than populated by zero-dimensional placeholder hashes.

If U1 accepted but the source-bound path holds before any tangent artifact is
completed, the current runtime still binds the admitted manifest surface:
manifest digest, dimension, and coordinates may remain present while endpoint
state, discrete-state artifact, tangent, recurrence, monodromy, and spectrum
stay null. This partial-hold shape is distinct from both the pure U1
pre-snapshot hold and later tangent/recurrence/spectral holds.

## 11. Required verification

The current release-blocking verification is the exact executed gate cluster in
`tests/u2/`, `runtime/cdc_variational.c`, and `scripts/verify_u2.sh`:

1. deterministic linear algebra and validated real-Schur decomposition:
   no-backend typed hold, backend-available success, reconstruction,
   orthogonality, and triangular residual checks;
2. executable variational primitives:
   source-ordered manifest packing, explicit-Euler `flow` Jacobian including a
   self-loop, scheduled fixed-mode `commit` identity, executable `nest`
   overwrite Jacobian, saltation versus pure reset, grazing/non-transverse
   hold, ordered noncommuting event products, recurrence gating, relative
   restoration application, polarity covariance, and multiplier classification
   micro-fixtures;
3. permanent mutant rejection:
   reversed product order, omitted saltation, omitted
   endpoint-restoration derivative, invented theoretical nest derivative,
   false recurrence/projected acceptance, ignored polarity aperture, skipped
   polarity tangent conjugacy, and false polarity acceptance;
4. native canonical held receipts:
   deterministic replay, parser-canonical source identity, tangent artifact
   presence, discrete-state receipts, and no monodromy/multipliers for
   recurrence-stage holds;
5. native accepted relative receipts:
   exact 13-coordinate closure, explicit `4π` restoration witness and
   row-major `D rho`, bound monodromy, physical multiplier retention, spectral
   radius, and Schur diagnostics;
6. native downstream spectral-backend hold:
   recurrence still verified, `analysis=monodromy`, monodromy retained, and no
   emitted multipliers/backend classification; and
7. native pre-snapshot primal hold:
   preserved U1 cause and null downstream U2 artifacts.

Permanent mutant gates MUST kill exactly these eight current families:
reversed product order, omitted saltation, omitted endpoint-restoration
derivative, invented theoretical nest derivative, acceptance of a false
recurrence/projected-only path, ignored polarity aperture, skipped polarity
tangent conjugacy, and automatic false polarity acceptance.

The following remain future obligations rather than current emitted v1 gate
clusters: source-localized simultaneous-event ambiguity, source event-budget
accounting, whole-path finite-difference receipts for arbitrary source
executions, and declared neutral-mode removal requests.

## 12. Claim ceiling

An accepted tangent receipt establishes that U2 computed the first-order
derivative of the **selected executable CDC path** in a fixed local event
itinerary, using operation Jacobians independently checked by the release
fixtures. It does not claim that a whole-path finite-difference oracle ran on
that particular source execution unless the receipt explicitly says so.

An accepted monodromy receipt additionally establishes verified full or declared
relative recurrence and the ordered first-order return operator.

An accepted multiplier classification establishes local linear/orbital
stability information for that recurrence, realization, parameters, and
tolerances.

It does not establish:

- global nonlinear stability or robustness beyond the tested neighborhood;
- correctness of prose dynamics absent from the primal executor;
- a physical constitutive law, physical units, or empirical fit;
- quantum behavior, spinorial matter, nonlocality, gravity, plasma identity,
  biological identity, or cosmological ontology; or
- that a mathematical specialization describes any observed object.

Those stronger claims require a domain-specific state map, dimensional and
constitutive grounding, calibration data, competing models, and discriminating
experiments. U2 supplies an unusually strong engineering instrument for such a
specialization; it is not the specialization's empirical proof.

## 13. Mathematical sources

- Nathan J. Kong, J. Joe Payne, James Zhu, and Aaron M. Johnson,
  [“Saltation Matrices: The Essential Tool for Linearizing Hybrid Dynamical
  Systems”](https://doi.org/10.1109/JPROC.2024.3440211), *Proceedings of the
  IEEE* (2024). Primary derivation and engineering treatment of the sensitivity
  jump at hybrid events.
- Ian A. Hiskens and M. A. Pai,
  [“Trajectory Sensitivity Analysis of Hybrid
  Systems”](https://doi.org/10.1109/81.828574), *IEEE Transactions on Circuits
  and Systems I* 47(2), 204–220 (2000). Primary treatment of continuous
  trajectory sensitivities and event jump conditions.
- Jeffrey J. DaCunha and John M. Davis,
  [“A Unified Floquet Theory for Discrete, Continuous, and Hybrid Periodic
  Linear Systems”](https://arxiv.org/abs/0901.3841) (2009). Primary unified
  treatment connecting periodic hybrid linear systems, monodromy operators,
  and Floquet multipliers.

These sources justify the analysis form. CDC's exact state maps, recurrence
contract, typed holds, and claim boundaries remain defined by this repository.

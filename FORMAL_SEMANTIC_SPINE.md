# Formal Semantic Spine

This document pins the language, executable runtime, U1/U2 analysis, paper, and
formal artifacts to one claim-preserving object. CDC is the language. The spine
is the typed path from source to state mutation to receipt to exactly scoped
proof obligation.

## Authority and claim shape

```text
.cdc canonical source
  -> grammar-1 AST
  -> selected executable state and ordered program
  -> flow / commit / nest reductions
  -> U1 acceptance and effect receipt
  -> U2 path tangent and recurrence receipt
  -> monodromy and multipliers only when authorized
  -> finite formal facts and open obligations
```

Every public claim is a tuple:

```text
(scope, maturity, verdict, receipt-or-obligation)
```

Maturity and verdict are separate.

| Maturity | Meaning |
|---|---|
| `specified` | defined in source or mathematics but not necessarily consumed by the current executor |
| `executed` | consumed by native code and bound to an execution receipt |
| `adversarially-verified` | positive, negative, oracle, or permanent-mutant gates distinguish the mechanism from shortcuts |
| `mechanized-finite` | Lean or Rocq/Coq proves the exact finite statement named |

| Verdict | Meaning |
|---|---|
| `accepted` | all prerequisites for that scoped result passed |
| `held` | a typed dynamical or semantic prerequisite did not pass; no stronger result is emitted |
| `violated` | an asserted contract or invariant failed |
| `not-emitted` | a downstream artifact was not authorized |

A held recurrence can be adversarially verified. A mechanized finite involution
does not establish a continuous symmetry. A source declaration is not an
execution receipt.

## One language, two semantic layers

CDC preserves both a calculus-level target and an audited native realization.
They are related, but they are not interchangeable.

### Specified state, `S_spec`

The calculus-level state may include:

```text
S_spec = (
  time,
  nested fields and modules,
  cell phase / amplitude / frequency / plasticity / latch / memory,
  module belief / prior / precision / action gain,
  weighted delayed angular and projected channels,
  event surfaces and resets,
  trace windows, local counters, policies, and durable effects
)
```

This is the intended formal vocabulary. A field appearing in `S_spec` does not
mean native realization v1 updates it.

### Audited native state, `S_native-v1`

For the current executor, the real-valued dynamic coordinates selected by a U1
path are exactly:

```text
x = (
  theta for each selected cell in source order,
  belief and prior for each selected module in source order
)
```

The discrete mode is:

```text
q = (has_latch, latch for each selected cell, ordered event itinerary)
```

The frozen parameters and fixed structure are:

```text
xi = (
  cell amplitude and omega,
  module precision and action_gain,
  field dt, gain, and deadband,
  channel weight, delay, angle, lines, endpoints, cone, and pair,
  source topology, jobs, expectations, council/readout/effect declarations
)
```

`omega` drives phase but is not itself updated. Projected phase, winding, and
sheet are derived from the unwrapped cover-cell phase. Holonomy is currently the
declared reciprocal channel-angle sum. None is silently appended to the tangent
vector.

The coordinate manifest closes mechanically over the exact accepted U1 path.
Unreferenced declaration islands do not add inert unit multipliers.

## The three primitive reductions

The only primitive state reductions are `flow`, `commit`, and `nest`.

### `flow(d)`

The calculus may specify continuous dynamics. Native v1 executes one
synchronous finite-duration phase map. For cell `i` in the selected field:

```text
theta_i' = theta_i + omega_i d
           + G d sum_(e: target(e)=i)
             w_e sin(theta_source(e) + alpha_e - theta_i)
```

Only channels whose source and target share the field contribute. Every right
side reads the same pre-step state. Belief and prior are unchanged. The exact
native phase Jacobian is therefore:

```text
d theta_i' / d theta_l
  = delta_il
    + G d sum_e w_e cos(theta_source(e) + alpha_e - theta_i)
      (delta_source(e),l - delta_i,l)
```

This is not an exact ODE solution. Amplitude relaxation, belief flow,
plasticity, projected-line dynamics, and delay history remain specified
obligations until the primal executor consumes them.

### `commit(module)`

Native v1 quantizes each selected phase:

```text
tau(theta) = +1  if cos(theta) > deadband
             -1  if cos(theta) < -deadband
              0  otherwise
```

The ordered trit walk is accepted only when every prefix sum is nonnegative. An
accepted commit populates latches but leaves continuous phase, belief, and prior
unchanged; inside one fixed trit/mode itinerary its continuous reset derivative
is identity. A held commit terminates the accepted path and has no accepted
tangent result.

Quantization boundaries are nonsmooth. A perturbation that changes trits,
barrier verdict, or event order must hold U2 rather than report a derivative
across incompatible modes.

### `nest(parent, child)`

Let `u(q)` be the child mean trit from latches when present and current
quantization otherwise. Native v1 executes:

```text
parent.belief' = parent.belief + G u(q)
child.prior'   = parent.belief'
```

All other continuous coordinates are unchanged. Inside a fixed mode,
`u(q)` is constant, so:

```text
d parent.belief' / d parent.belief = 1
d child.prior'   / d parent.belief = 1
d child.prior'   / d child.prior   = 0
```

The last row is an overwrite, not identity. Analytic-versus-finite-difference
and permanent-mutant tests protect this exact implementation detail.

## Derived observation, frameworks, and persistence

Trace, window, measurement, bridge, council, and framework forms observe or
compose the same primitive reductions; they do not add state-reduction kinds.

Trace time is local to a bounded observer window:

```text
phase time   continuous motion
event time   ordered guarded commits
trace time   selected phase/event history through a window
```

Passive observation does not mutate primal state. A committing measurement
still routes through the guarded balanced-ternary commit semantics.

Persistence is a typed effect layer. `persist op=append` traverses the same
barrier as an in-memory commit, and a held decision has no path to durable
mutation. Durable-byte identity and replay-state identity are distinct, so
compaction can change representation while preserving semantic history.

## Universal Operator U1

U1 is a derived guarded composition over one live native state. Acceptance
requires:

1. reversed-endpoint receptive and radiant channels with a shared pair identity,
   both active in the selected frame evaluation;
2. one-turn projected return with sheet inversion;
3. two-turn projected return with sheet restoration at winding two;
4. declared and observed holonomy agreement;
5. accepted local commits;
6. runtime-computed record/decision coordinate equality; and
7. enactment of only that computed coordinate after acceptance.

Write the accepted state map as:

```text
U_X : (q_0, x_0) -> (q_T, x_T)
```

Trace, bridge, council, and enactment are readout, acceptance, and effect layers
around `U_X`. U1 earns **lifted-cover closure**. It does not assert
`(q_T, x_T) = (q_0, x_0)`.

The canonical `loop-u720` U1 accepts. Its full native state does not recur:
latches populate, context belief and child prior accumulate, and the unwrapped
cover phase advances by `4 pi`.

## Variational Universal Operator U2

U2 differentiates the exact selected U1 path. It does not add a primitive
reduction.

For an accepted path with a stable event itinerary:

```text
D U_X(x_0) : T_(x_0) X_(q_0) -> T_(x_T) X_(q_T)
```

This path tangent is meaningful even without recurrence. Ordered Jacobians act
on column perturbations by left multiplication:

```text
A_0 = I
A_(k+1) = J_k A_k
```

For a hybrid itinerary of smooth segments and events:

```text
D U_X = Phi_(N+1) S_N Phi_N ... S_1 Phi_1
```

Scheduled native commits are fixed-time resets and use `D R`, not a saltation
correction. A true state-triggered event requires an explicit guard `g`, reset
`R`, crossing direction, event localizer, and transversality receipt. Only then
is the saltation matrix authorized:

```text
S = D R + ((f+ - D R f- - d_t R) n^T) / (n^T f- + d_t g)
```

Grazing, ambiguous simultaneous events, divergent itineraries, missing
derivatives, and exhausted event budgets are typed holds.

### Recurrence gate

Absolute recurrence requires the same continuous manifest, restored discrete
mode and itinerary class, and a weighted endpoint residual inside explicit
absolute-plus-relative tolerances:

```text
q_T = q_0
norm_W(x_T - x_0) <= epsilon
```

Topology is coordinate-specific: a lifted cover phase uses unwrapped distance;
ordinary declared circle coordinates may use shortest-circle distance.

Relative recurrence additionally requires an explicit executable endpoint
restoration:

```text
rho_gamma : X_(q_T) -> X_(q_0)
norm_W(rho_gamma(x_T) - x_0) <= epsilon
```

The receipt must bind `rho_gamma`, its complete derivative, and executable
equivariance/section witnesses. Hiding changed coordinates is projection, not
relative recurrence.

Only after recurrence may U2 construct:

```text
M     = D U_X(x_0)                        absolute return
M_rel = D rho_gamma(x_T) D U_X(x_0)      relative return
```

Characteristic multipliers are eigenvalues of this verified square return
operator. A real-Schur backend must validate the decomposition and residuals.
Absent recurrence or backend, no multipliers are emitted. A near-`+1`
multiplier remains physical unless a declared symmetry generator and section
prove it is a removable neutral/gauge mode.

The canonical source-bound U2 result is:

| Scope | Maturity | Verdict |
|---|---|---|
| U1 lifted-cover closure | executed + adversarially verified | accepted |
| 13-coordinate U2 path tangent | executed + adversarially verified | accepted |
| complete native-state recurrence | executed + adversarially verified | held: `recurrence-mode-mismatch` |
| canonical monodromy and multipliers | gated downstream result | not emitted |

The isolated relative-return fixture explicitly restores the `4 pi` cover
translation, binds complete `D rho = I`, and earns `M_rel = I_13`, thirteen
`physical`-labeled unit multipliers, and a `marginal` classification. It
validates the machinery, not recurrence of `loop-u720` or a physical system.

## Apertured oriented reciprocity

The strongest non-forced polarity structure currently earned at U level is
**apertured oriented reciprocity**:

- balanced-ternary sign reversal exchanges `-1` and `+1` while fixing `0`;
- receptive/radiant channels are reversed relation roles;
- path orientation may reverse angular direction; and
- double-cover sheet parity flips after one turn and restores after two.

These live in different types. They may be related by an explicitly declared
specialization but cannot be identified by rhetoric. In particular, `+-` passes
the oriented prefix barrier while the naive pointwise sign inverse `-+` fails.
Thus carrier inversion is not a global safety symmetry.

The generic executable polarity-covariance checker accepts only when a supplied
source/target involution fixes its aperture and satisfies both primal and
tangent conjugacy:

```text
rho_1 U_X(x) = U_X^rho(rho_0 x)
D rho_1 D U_X = D U_X^rho D rho_0
```

This mechanism is adversarially verified and its finite component facts are
mechanized. It is not yet source-bound to `loop-u720`, does not establish
recurrent conjugate spectra, and is not a universal physical law.

## Invariant registry

`laws.cdc` declares 16 invariant keys. Their presence and witness linkage are
checked by the registry; their mathematical maturity differs by statement.

| Cluster | Stable keys |
|---|---|
| carrier and bridge | `balanced-ternary-carrier`, `dyadic-triadic-closure` |
| viability and locality | `existence-viability`, `trace-order-locality` |
| algebra | `gate-abelian`, `interfere-monoid`, `rotation-linear`, `corefold-morphism` |
| reduction | `preservation`, `soundness`, `local-confluence`, `flow-additivity`, `normalforms` |
| lifted closure | `universal-closure` |
| durable state | `durable-latch-or-hold`, `replay-identity` |

Registry membership means “declared and linked,” not “universally proved.” The
verification matrix records the maturity, verdict, receipt, and remaining
obligation for each cluster.

## Formal artifacts and exact ceilings

The native finite checker plus `formal/lean/CDCFinite.lean` and
`formal/coq/CDCFinite.v` cover the named finite balanced-ternary counts and
algebraic facts. `formal/lean/U2VariationalFinite.lean` and
`formal/coq/U2VariationalFinite.v` cover only:

1. identity and associative composition of finite tangent maps;
2. observable order for two noncommuting finite event maps;
3. carrier inversion with fixed aperture;
4. distinct cone-role and sheet involutions; and
5. the concrete `+-` accepted / `-+` held barrier counterexample.

They do **not** prove the native runtime Jacobian, full-state recurrence,
saltation implementation, a numerical eigensystem, source-bound polarity
covariance, a continuous holonomy theorem, or any empirical interpretation.

## Synchronization obligations

The spine stays authoritative only while all of the following remain gated:

- the grammar-1 frontend and canonical serialization match native/Python
  declaration behavior;
- source forms and witness links remain closed and typed;
- current native mutations are documented separately from richer specified
  semantics;
- U1 acceptance never aliases full-state recurrence;
- U2 never emits monodromy or multipliers after a recurrence hold;
- relative return always applies the declared restoration derivative;
- product order, nest overwrite, saltation, false recurrence, and polarity
  shortcuts are killed by permanent mutants;
- formal prose names only the exact finite statements in the proof files; and
- paper, README, language reference, product surfaces, and receipts agree on
  versions, counts, ABI, and claim ceilings.

`./scripts/verify.sh` is the repository-wide authority. `./scripts/verify_u2.sh`
is the focused analytic, numerical, receipt, and mutant authority for U2.

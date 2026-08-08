# The `.cdc` Language

`.cdc` is the source language for BiDi Coherence-Delta Calculus. Version 0.3.0
uses one grammar-1 statement stream for the declaration checker, native parser,
runtime, toolchain, Universal Operator, and U2 stability analysis. Within that
language/runtime/semantic execution boundary, Python is confined to
`cdc_boot.py`, a small declaration/expectation oracle; executable semantics live
in the native C implementation and are checked against the same source.
Asset-generation utilities under `tools/` are outside that boundary.

## Semantic center

CDC has exactly three primitive state reductions:

```text
flow(d)               continuous phase evolution
commit(module)        guarded balanced-ternary latch-or-hold
nest(parent, child)   cross-scale belief/prior exchange
```

Trace/windows, measurement, task frameworks, durable effects, U1, and U2 are
derived layers. `universal`, `orbit`, `variational`, and `spectrum` do not add
foundational reductions.

The calculus-level specification is deliberately richer than audited native
realization v1. Public claims therefore carry two independent axes:

- maturity: `specified`, `executed`, `adversarially-verified`, or
  `mechanized-finite`; and
- verdict: `accepted`, `held`, `violated`, or `not-emitted`.

`held` is an execution verdict, not a low evidence grade. A thoroughly tested
gate can be adversarially verified because it correctly holds and suppresses an
unauthorized result.

## Grammar 1

```ebnf
program      = { directive } ;
directive    = kernel | term | rule | provides | bootloader
             | invariant | law | capability | framework | witness
             | field | module | cell | channel | guard | counter
             | flow | commit | nest | trace | measure | policy | bridge
             | compile | interpret | proof | council | deliberate | evolve
             | universal | orbit | variational | spectrum
             | store | persist
             | frame | reduce | complex | topology | authority | transport
             | expect | "end" ;

kernel       = "kernel" name { kwarg } ;
term         = "term" name { name } ;
rule         = "rule" name { name } ;
provides     = "provides" name { name } ;
bootloader   = "bootloader" name { name } ;

invariant    = "invariant" key { kwarg } ;
law          = "law" key { kwarg } ;
capability   = "capability" key { kwarg } ;
framework    = "framework" key { kwarg } ;
witness      = "witness" key { kwarg } ;

field        = "field" key { kwarg } ;
module       = "module" key { kwarg } ;
cell         = "cell" path { kwarg } ;
channel      = "channel" path "->" path { kwarg } ;
guard        = "guard" key { kwarg } ;
counter      = "counter" key { kwarg } ;
flow         = "flow" key { kwarg } ;
commit       = "commit" key { kwarg } ;
nest         = "nest" key { kwarg } ;
trace        = "trace" key { kwarg } ;
measure      = "measure" key { kwarg } ;
policy       = "policy" key { kwarg } ;
bridge       = "bridge" key { kwarg } ;
compile      = "compile" key { kwarg } ;
interpret    = "interpret" key { kwarg } ;
proof        = "proof" key { kwarg } ;
council      = "council" key { kwarg } ;
deliberate   = "deliberate" key { kwarg } ;
evolve       = "evolve" key { kwarg } ;
universal    = "universal" key { kwarg } ;
orbit        = "orbit" key { kwarg } ;
variational  = "variational" key { kwarg } ;
spectrum     = "spectrum" key { kwarg } ;
store        = "store" key { kwarg } ;
persist      = "persist" key { kwarg } ;
frame        = "frame" key { kwarg } ;
reduce       = "reduce" key { kwarg } ;
complex      = "complex" key { kwarg } ;
topology     = "topology" key { kwarg } ;
authority    = "authority" key { kwarg } ;
transport    = "transport" key { kwarg } ;

expect       = "expect" predicate ;
kwarg        = key "=" value ;
```

`#` begins a comment. Values may use shell-style quoting. Canonical grammar-1
serialization drops comments and blank lines, normalizes token spacing and
quoting, and preserves statement order. Its bytes are the U2 source-identity
surface; the stability runtime does not re-read the file later and call those
new bytes the executed source.

## Current executable reductions

The derivative and receipts follow what the native executor mutates, not a
richer prose model.

| Form | Audited native v1 behavior | Current boundary |
|---|---|---|
| `flow` | one synchronous phase map using `omega`, duration, field gain, channel weight, and angle | amplitude, weight, delay history, belief flow, plasticity, and line projection are not dynamic |
| `commit` | quantize cell phases, enforce the nonnegative-prefix barrier, then populate latches | accepted commit leaves phase, belief, and prior unchanged; quantization boundaries are nonsmooth |
| `nest` | add the fixed child mean trit to parent belief and overwrite child prior with the new belief | the overwritten child-prior derivative row is not identity |
| `guard` | report an open/closed source state | it is not yet an off-grid event surface bound to a transition |

Scheduled commits therefore use reset Jacobians. Saltation is reserved for a
future explicit state-triggered event binding with guard, direction, reset,
localization, transversality, and deterministic event ordering. Nonzero channel
delay has no declared history state and must hold U2 as
`unsupported-delay-state` when selected as claimed dynamics.

## Universal Operator U1

`universal` binds an executed reducer path to a guarded lifted-cover acceptance:

```cdc
universal loop-u720 \
  frame=agent cover-cell=loop-cover.phase cover=double \
  half-step=loop-turn-half full-step=loop-turn-full \
  receptive=loop-receptive radiant=loop-radiant \
  record=loop-key decision=loop-quorum enact=loop-enact \
  tolerance=0.000001 expect-status=accepted expect-reason=none
```

Acceptance requires reciprocal receptive/radiant channels, one-turn projection
return with sheet inversion, two-turn projection return with sheet restoration,
declared holonomy agreement, accepted local commits, record/decision equality,
and guarded enactment of the computed coordinate.

That is **lifted-cover closure**. It is not automatically recurrence of every
continuous coordinate, latch, or event mode in the complete runtime state.

## Variational Universal Operator U2

U2 is declared as a typed chain:

```cdc
orbit loop-u720-full universal=loop-u720 coordinates=all quotient=none \
  absolute-tolerance=0.000001 relative-tolerance=0.0
variational loop-u720-tangent orbit=loop-u720-full
spectrum loop-u720-spectrum variational=loop-u720-tangent \
  neutral-tolerance=0.000000001 schur-tolerance=0.0000000001
```

`tolerance=` remains the legacy alias for `absolute-tolerance=`. An `orbit` may
select `coordinates=all` or an explicit comma-separated list, may remove an
explicit `exclude=` list, and declares `quotient=none|phase`. Coordinate
projection alone never authorizes monodromy.

For a selected accepted U1 path, native U2:

1. closes a source-ordered coordinate manifest over exactly the executed path;
2. differentiates the executed `flow`, `commit`, and `nest` maps in order;
3. binds initial/final continuous states, latches, and itinerary;
4. independently checks full or explicit relative recurrence;
5. constructs `M = D U` or `M_rel = D rho D U` only after that check; and
6. computes deterministically ordered characteristic multipliers through a
   validated real-Schur backend.

The continuous coordinate vector is:

```text
(cell theta values in source order,
 module belief/prior pairs in source order)
```

Cell latches and the event itinerary are discrete mode. Amplitude, omega,
precision, action gain, field constants, channel parameters, source jobs, and
effect paths are frozen parameters or non-dynamic structure. Derived projected
phase, winding, sheet, and declared holonomy are not extra tangent coordinates.

Full recurrence requires the same manifest, restored discrete mode, compatible
itinerary class, and a weighted continuous residual within recorded absolute
and relative tolerances. Relative recurrence additionally requires an explicit
endpoint restoration `rho`, a complete `D rho`, and executable equivariance and
section witnesses. Omitting changed coordinates is only projected tangent
analysis.

The command:

```bash
./build/cdc stability framework_loop.cdc
```

emits one authoritative `u2-json=` record for the declared `spectrum` job. The
canonical loop has a 13-coordinate path tangent and U1 acceptance, then holds as
`recurrence-mode-mismatch`; monodromy and multipliers are null. The isolated
relative-return fixture explicitly restores the lifted `4 pi` cover phase,
applies complete `D rho = I`, emits `M_rel = I_13`, retains thirteen
`physical`-labeled `+1` multipliers, and classifies the fixture `marginal`.

The minimum typed-hold vocabulary and the complete receipt schema are normative
in [`docs/u2/U2_SEMANTICS.md`](docs/u2/U2_SEMANTICS.md). Human-readable lines are
diagnostics; the `cdc.u2.stability.v1` record is acceptance authority.

## Polarity is scoped and typed

CDC preserves four distinct structures: balanced-ternary carrier polarity,
receptive/radiant relation roles, path orientation, and double-cover sheet
parity. The finite formal mirrors prove each named involution separately and
prove the counterexample that `+-` is prefix-admissible while `-+` is held.

The implemented generic polarity-covariance checker independently gates
involution, fixed aperture, primal conjugacy, and tangent conjugacy. It is not
yet bound to a `.cdc` polarity form or to `loop-u720`; it does not establish a
global sign symmetry or a physical polarity law. See
[`docs/u2/POLARITY_AUDIT.md`](docs/u2/POLARITY_AUDIT.md).

## Expectations and framework contracts

The bootloader recognizes these public predicate families:

```text
expect native substrate == cdc
expect host-debt <= N
expect python-files == N
expect bootloader minimal == true
expect terms|rules|invariants|witnesses|capabilities|frameworks >= N
expect provides <capability...>
expect law <invariant-key>
expect capability <capability-key>
expect witness <witness-id>
expect reducer|guard|trace|measure|policy|bridge|counter <witness-id>
expect compile|interpret|proof|council|evolution|universal <witness-id>
expect spectrum <witness-id>
expect store|persistence <witness-id>
expect framework <framework-key> complete
expect frameworks closed
```

`expect law K` requires an `invariant K` plus a linked witness. `expect
capability C` likewise requires the declaration and a linked witness. `expect
spectrum W` requires witness `W` to name a declared `spectrum` job; it verifies
the source binding, not the numerical verdict.

A framework declaration is a typed role contract:

```cdc
framework H3 label=episodic requires=live,record,... \
  permits=flow,commit,nest,...
```

`expect framework K complete` enforces exactly one witness for each required
role, exactly one executable link per role, link existence, and primitive
compatibility. `expect frameworks closed` rejects orphan framework labels.

The six current frameworks are transition (`H1`), procedural (`H2`), episodic
(`H3`), deliberative (`H4`), task-loop/U1/U2 composition (`H5`), and durable
persistence (`H6`).

## Durable forms

```cdc
store <id> dir=<path> mode=fresh|open
persist <id> store=<id> op=<verb> [module=<id>] [seal=<n>]
```

Persistence verbs are `append`, `replay`, `attest`, `verify`, `snapshot`,
`compact`, and `fence`. `append` traverses the same balanced-ternary barrier as
an in-memory commit. A held decision cannot reach the log. Every persistence
job reports observed durable-byte identity separately from replay-state
identity; compaction may honestly report `durable=yes replay=stable`.

## Source and implementation map

| Surface | Authority |
|---|---|
| `kernel.cdc` | terms, rules, provided capabilities, bootloader boundary, aggregate minimums |
| `laws.cdc` | 16 invariant declarations and linked semantic witnesses |
| `system.cdc` | 32 file-local A1-G8 capability declarations; not the repository total |
| `framework_*.cdc` | six role contracts and their executable bindings |
| `framework_loop.cdc` | H5, U1, and source-bound canonical U2 chain |
| `native_reducer.cdc` / `native_surface.cdc` | primal reducer and derived surface jobs |
| `bridge64.cdc`, `bridge512.cdc`, `bridge4096.cdc` | explicit and generated bridge codebooks |
| `runtime/cdc_parser.c` / `runtime/cdc_ast.c` | grammar-1 frontend and canonical source identity |
| `runtime/cdc_native_runtime.c` | primal, U1, persistence, and U2 source consumer |
| `runtime/cdc_variational.c` | tangent, recurrence, restoration, saltation, and polarity mechanisms |
| `runtime/cdc_linalg.c` | deterministic real-Schur spectrum and validation |
| `tests/u2/` / `tests/fixtures/u2/` | analytic, numerical, recurrence, receipt, and hold contracts |
| `formal/lean/` / `formal/coq/` | exact finite statements only |

The current checked root corpus reports:

```text
262/262 expectations
20 terms · 22 rules · 16 invariants
46 capabilities · 6 frameworks · 4831 native witnesses
```

These are registry facts, not theorem counts. `./scripts/verify.sh` is the local
release authority; `./scripts/verify.sh --require-formal` additionally requires
Lean, Rocq/Coq, and Tectonic. `./scripts/verify_u2.sh` runs the focused U2
analytic, spectral, receipt, purity, positive/negative, and eight-family
permanent-mutant gate.

## Claim boundary

CDC 0.3.0 executes a native guarded hybrid substrate and a recurrence-gated
first-order analysis of selected paths. It does not turn source declarations
into runtime behavior, path tangents into Floquet multipliers, finite algebra
lemmas into continuous proofs, or a mathematical specialization into empirical
physics. Stronger claims require the exact executable state, receipt, formal
obligation, or discriminating experiment appropriate to their scope.

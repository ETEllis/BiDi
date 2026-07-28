# The `.cdc` Language
### Native source format for the Coherence-Delta Calculus

`.cdc` is the native language surface for the calculus kernel. In v0.2.4 the
checked surface includes native declarations, witnesses, and the first
source-declared reducer jobs: terms, reducer rules, field/module/cell/channel
state, flow/commit/nest jobs, guard/trace/measure/policy/bridge/counter jobs,
IR interpretation, council deliberation, bridge-coordinate source evolution,
native replay JSON, invariants, capabilities, witnesses, and expectations are expressed in `.cdc`;
Python is limited to the small `cdc_boot.py` loader/checker.

The semantic target remains the full calculus:

```text
flow(d)       continuous reduction
commit(m)     guarded balanced-ternary event reduction
nest(m,F)     nested bidirectional coherence-delta coupling
trace/window  derived observer projection
```

The bootloader does not implement those reductions. It verifies that the native
source tree declares them and that every claim has a native witness handle. The
non-Python runtime `runtime/cdc_native_runtime.c` consumes `native_reducer.cdc`,
`native_surface.cdc`, and `council_bridge.cdc`; executes the checked `flow`,
`commit`, `nest`, `guard`, `trace`, `measure`, `policy`, `bridge`, `counter`,
`interpret`, `council`, `evolve`, and `replay` jobs; and verifies their expectations.
Both C runtimes share `runtime/cdc_source.c` / `runtime/cdc_source.h` for
native `.cdc` line parsing, attribute extraction, typed attributes, and
primitive expectation assertions.
`runtime/cdc_wasm_exports.c` exposes the replay path through a compile-checked
C ABI for an eventual Emscripten build.

## Current Checked Grammar

```ebnf
program      = { directive } ;
directive    = kernel | term | rule | provides | bootloader
             | invariant | law | capability | framework | witness
             | field | module | cell | channel | guard | counter
             | flow | commit | nest | trace | measure | policy | bridge
             | compile | interpret | proof | council | deliberate | evolve
             | universal | store | persist
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
cell         = "cell" key { kwarg } ;
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
store        = "store" key { kwarg } ;
persist      = "persist" key { kwarg } ;

expect       = "expect" predicate ;
kwarg        = key "=" value ;
```

`#` begins a comment. Values may be quoted with shell-style quoting.

## Native Expectation Predicates

The minimal bootloader currently verifies:

```text
expect native substrate == cdc
expect host-debt <= 1
expect python-files == 1
expect bootloader minimal == true
expect terms >= N
expect rules >= N
expect invariants >= N
expect witnesses >= N
expect capabilities >= N
expect provides <capability...>
expect law <invariant-key>
expect capability <capability-key>
expect witness <witness-id>
expect reducer <witness-id>
expect guard <witness-id>
expect trace <witness-id>
expect measure <witness-id>
expect policy <witness-id>
expect bridge <witness-id>
expect counter <witness-id>
expect compile <witness-id>
expect interpret <witness-id>
expect proof <witness-id>
expect council <witness-id>
expect evolution <witness-id>
expect universal <witness-id>
expect frameworks >= N
expect frameworks closed
expect framework <framework-key> complete
```

`expect law K` requires both an `invariant K` declaration and at least one native
`witness ... invariant=K`. `expect capability C` requires both a
`capability C` declaration and at least one native `witness ... capability=C`.

A `framework` declaration is a typed role contract:

```text
framework H3 label=episodic requires=live,record,... permits=flow,commit,...
```

`expect framework K complete` requires that every role in `requires=` is bound
by exactly one witness carrying `framework=<label> role=<role>`, that each such
witness links to exactly one declared executable job, that the linked job's
primitive kind is in `permits=` (`reducer=` links resolve to their declared
`flow`/`commit`/`nest` kind; `council=` and `evolution=` links resolve through
`deliberate` and `evolve` jobs), and that no witness carries an unknown or
duplicate role. `expect frameworks closed` additionally rejects any witness in
the whole tree whose `framework=` label has no `framework` declaration.
`expect reducer W` requires witness `W` to link to a declared native reducer
step through `witness ... reducer=<flow-or-commit-or-nest-id>`.
`expect guard W`, `expect trace W`, `expect measure W`, `expect policy W`,
`expect bridge W`, `expect counter W`, `expect compile W`, `expect interpret W`,
`expect proof W`, `expect council W`, `expect evolution W`, `expect store W`,
and `expect persistence W` use the same linkage pattern for source-declared
full-surface, compile, IR interpretation, finite-proof, council,
source-evolution, and durable-persistence jobs.

### Persistence forms

`store <id> dir=<path> [mode=fresh|open]` declares a durable log the way
`field` declares a continuum. `mode=fresh` removes exactly that store's own
two artifacts before opening — nothing else in the directory is touched —
so a source file's declared expectations are absolute rather than relative
to whatever a previous run left behind.

`persist <id> store=<id> op=<verb> [module=<id>] [seal=<n>]` is a job over a
declared store. `op` is one of `append`, `replay`, `attest`, `verify`,
`snapshot`, `compact`, `fence`.

`op=append` is the load-bearing one: it runs the **identical**
balanced-ternary barrier that `commit` runs over `module`, and only an
accepted decision may reach the log. A violated prefix balance holds —
nothing is staged, nothing is committed, and the sealed bytes are
unchanged. Durable mutation is therefore not reachable from source except
through an accepted commit decision.

Every job records two independent identities, both **observed** rather than
declared: `durable=yes|no` (did the sealed log bytes change) and
`replay=stable|changed` (did the semantic state folded from sealed history
change). They agree everywhere except under `op=compact`, which rewrites
layout without rewriting history — `durable=yes replay=stable`. Inline
`expect-status`, `expect-reason`, `expect-durable`, `expect-replay`,
`expect-sealed`, `expect-events`, `expect-trits`, and `expect-balance`
attributes check those observations; over-claiming any of them fails
closed.

## Native Files

| file | content |
|---|---|
| `kernel.cdc` | language terms, reducer rules, provided capabilities, bootloader boundary, and global expectations |
| `laws.cdc` | invariant registry and 25 law/metatheorem witness declarations |
| `bridge64.cdc` | explicit 64-row `2^6 = 4^3` dyadic/triadic bootstrap codebook |
| `bridge_codebooks.cdc` | higher-arity bridge declarations for `n=9` and `n=12` |
| `bridge512.cdc` | full generated `n=9`, `2^9 = 8^3 = 512` bridge codebook rows |
| `bridge4096.cdc` | full generated `n=12`, `2^12 = 16^3 = 4096` bridge codebook rows |
| `bridge_jobs.cdc` | source-declared bridge-coordinate jobs consumed by the C runtime |
| `native_reducer.cdc` | source-declared field/module/cell/channel state plus reducer, compile, interpret, and finite-proof jobs consumed by the C native runtime |
| `native_surface.cdc` | source-declared guard, trace, measure, policy, bridge, and counter jobs consumed by the C native runtime |
| `council_bridge.cdc` | source-declared council deliberation and bridge-coordinate source-evolution jobs |
| `framework_transition.cdc` | transition framework (`H1`): guard precondition, flow action, fired/blocked commits, nest hierarchy, trace log, bridge state key, counter tally |
| `framework_procedural.cdc` | procedural framework (`H2`): cue, executed and retried steps, nest consolidation, self-referential compile/interpret proceduralization |
| `framework_episodic.cdc` | episodic framework (`H3`): lived flow, committed record, archive consolidation, trace content, measured recall, bridge memory key, ordinal counter |
| `framework_deliberative.cdc` | deliberative framework (`H4`): option modules, council quorum decision, bridge-coordinate enactment into source memory |
| `framework_persistence.cdc` | persistence framework (`H6`): declared durable stores, barrier-gated append, held append that writes nothing, replay/attest/verify reads, snapshot/compaction, and a two-handle compare-and-set counterexample |
| `framework_loop.cdc` | task-loop composition (`H5`) and Universal Operator instance (`U1`): two executed sense/act/integrate cycles plus lifted-cover turns over one shared state object, gate/record/recall/refine/key/decide/enact jobs, self-referential compile/interpret, and the `universal` 720° closure job |
| `system.cdc` | 32 capability declarations and native witness handles |
| `relations.cdc` | angular, projected, cross-scale, detuning, and overlap relation witness handles |
| `trace_windows.cdc` | balanced-ternary trace/window, local-counter, coupled-observer, and recursive-policy witness handles |
| `cdc_boot.py` | minimal loader/checker; not the reducer or language semantics |
| `runtime/cdc_source.c` / `runtime/cdc_source.h` | shared native `.cdc` line parser, attribute reader, typed attribute accessors, and primitive expectation checks for C consumers |
| `runtime/cdc_bridge_runtime.c` | non-Python bridge consumer for lookup, trace projection, generated codebook verification, interactive grid/SVG output, and finite validation |
| `runtime/cdc_native_runtime.c` | non-Python reducer, full-surface, compile-IR, IR interpreter, finite-proof, council, source-evolution, persistence, and replay consumer for source-declared jobs |
| `runtime/cdc_store.c` / `runtime/cdc_store.h` | durable, replayable event-log substrate behind the `store`/`persist` forms: authenticated record framing, typed recovery, snapshot/compaction, and the compare-and-set fence |
| `runtime/cdc_blake3.c` / `runtime/cdc_digest.c` | vendored BLAKE3 and the canonical content/event digest surface |
| `runtime/cdc_wasm_exports.c` | compile-checked C ABI wrapper for the native replay JSON path |
| `formal/lean/CDCFinite.lean` | Lean mirror of the finite n=6 balanced-ternary carrier and algebraic law proofs |
| `formal/coq/CDCFinite.v` | Coq mirror of the finite n=6 balanced-ternary carrier and algebraic law proofs |

## Balanced Ternary Carrier

The committed discrete carrier is balanced ternary, not binary:

```text
-1  inward localization / contraction
 0  resting equilibrium / open crossing
+1  outward expansion / dissipation
```

The middle value is a real equilibrium/crossing state. It is not false, null, or
absence.

## Full Semantic Syntax Target

The following forms are now part of the checked reducer surface:

```text
field <name> dt=<real> gain=<real> deadband=<real>
module <name> field=<field> belief=<real> prior=<real>
cell <path> module=<module> theta=<real> amplitude=<real> omega=<real>
channel <path-a> -> <path-b> delay=<real> weight=<real> angle=<real> lines=<i,j>
channel <path-a> -> <path-b> id=<name> cone=<receptive|radiant> pair=<name> ...
guard <name> cell=<path> expect-state=<open|closed>
flow <name> field=<field> duration=<real>
commit <name> module=<module>
nest <name> parent=<module> child=<module>
counter <name> value=<int> increment=<int> decrement=<int>
trace <window> field=<field> expect-trits=<trits> expect-events=<int>
measure <name> observer=<module> target=<module> mode=<mode>
policy <name> window=<trace> sampling=<mode> commit=<mode> adapt=<mode>
bridge <name> trace=<trace> via=<codebook>
compile <name> source=<path> expect-ops=<int>
interpret <name> source=<path> expect-ops=<int>
proof <name> carrier=balanced-ternary arity=<int>
council <name> field=<field> members=<module-list> quorum=<int>
deliberate <name> council=<council>
evolve <name> source=<path> output=<path> coordinate=<dyadic> append-witness=<id>
universal <name> frame=<module> cover-cell=<path> cover=double half-step=<flow> full-step=<flow> receptive=<channel-id> radiant=<channel-id> record=<bridge> decision=<deliberate> enact=<evolve> expect-holonomy=<real> expect-coordinate=<dyadic> expect-status=<accepted|held> expect-reason=<reason>
```

Those forms must continue to grow as native reducer clauses in `.cdc`, not by
growing Python back into a runtime.

## Universal Operator Form

`universal` is a derived, higher-order job over `flow`, `commit`, and `nest`
plus `trace`, `bridge`, `council`, and `evolve` — it is not a fourth
foundational reduction. The C runtime's `universal` mode executes it over one
live runtime object: it verifies that the named `receptive`/`radiant` channels
form a reciprocal, angularly biased endpoint pair in the frame's field (both
active in the same flow evaluation), runs the reducer steps, tracks a lifted
double-cover frame on the cover cell (projected phase modulo 2π, winding
count, Z2 sheet, accumulated holonomy), and accepts only when the half turn
returns the projection with an inverted sheet, the full 720° turn restores
both, measured holonomy matches the declaration, all local commits are
accepted, and the runtime-computed record coordinate equals the runtime-
computed council decision coordinate — which is then the coordinate enacted.
Any failure returns `status=held` with one of: `cone-not-reciprocal`,
`half-projection-mismatch`, `half-sheet-mismatch`, `full-projection-mismatch`,
`full-sheet-mismatch`, `holonomy-mismatch`, `coordinate-mismatch`,
`local-commit-held` — and no evolved output is created.

The generic calculus vocabulary is **causal cone**: a channel's cone declares
the direction of causal influence relative to its frame. A physical
*light-cone* reading is a specialization that additionally requires a metric
and null-propagation constraints; the language does not assume it.

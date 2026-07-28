# Native Task Frameworks

The kernel gives the calculus its verbs: `flow`, `commit`, `nest`, `guard`,
`trace`, `measure`, `policy`, `bridge`, `counter`, `compile`, `interpret`,
`council`, `evolve`. This layer gives those verbs task-shaped names. Each
framework is a **binding plus an executed exemplar plus a typed contract**,
not new machinery:

- the exemplar layer required no new grammar and no host growth — frameworks
  use only pre-existing checked directives;
- the contract layer adds exactly one declarative registry directive
  (`framework <key> label=... requires=... permits=...`) whose role contract
  is enforced by the frozen differential oracle and checked in the required
  repository gate; the specialized reducer runtimes skip the declaration while
  the canonical frontend retains it, and any future oracle deletion follows
  the explicit Option-A replacement decision in
  `NATIVE_SELF_HOSTING_MANDATE.md`;
- every binding witness links to a job the C native runtime actually executes;
- every exemplar ships with deterministic expectations checked by
  `./scripts/verify.sh`.

A framework file therefore has four sections: a capability registry entry
(`H1`–`H5`), a `framework` role contract, a runnable exemplar with `expect-*`
values, and binding witnesses whose `framework=`/`role=` attributes name the
pattern while their job links (`guard=`, `reducer=`, `trace=`, `measure=`,
`policy=`, `bridge=`, `counter=`, `compile=`, `interpret=`, `council=`,
`evolution=`) keep every claim attached to executed behavior.

## The Framework Contract

`framework=` and `role=` are not free-floating metadata: each framework
declares its taxonomy as native semantics. The frozen `cdc_boot.py` oracle
currently supplies the independent completeness check inside the required gate;
it is not a product execution dependency.

```text
framework H3 label=episodic requires=live,record,consolidate,aperture,content,recall,key,ordinal,policy permits=flow,commit,nest,guard,trace,measure,policy,bridge,counter
expect framework H3 complete
```

`expect framework <key> complete` fails unless all of the following hold:

1. **completeness** — every role in `requires=` is bound by a witness;
2. **uniqueness** — no role is bound twice;
3. **no orphan roles** — every `framework=<label>` witness carries a role from
   `requires=`;
4. **exactly one executable link** per binding witness;
5. **link existence** — the linked job is declared in the tree (`reducer=`
   links resolve to their declared `flow`/`commit`/`nest` kind, `council=`
   through `deliberate`, `evolution=` through `evolve`);
6. **role-primitive compatibility** — the resolved primitive kind is in
   `permits=`.

`expect frameworks closed` (in `kernel.cdc`) additionally rejects any witness
anywhere in the tree whose `framework=` label has no `framework` declaration,
and `expect frameworks >= 5` pins the registry floor. The taxonomy is
therefore contract-checked, not documentation.

## The Generic Task Loop

The four frameworks are named slices of one loop that the kernel already
executes end to end:

```text
sense        flow        continuous phase motion
gate         guard       open/closed precondition on a crossing cell
act          commit      guarded balanced-ternary event (accepted/held)
integrate    nest        child coherence up, parent context down
record       trace       window-local ternary content, no global tick
consolidate  bridge      occupancy -> dyadic/triadic coordinate (a key)
decide       council     member trits -> quorum -> one coordinate
enact        evolve      decision written into durable source memory
refine       policy      recursive window adaptation
```

The balanced-ternary carrier is the shared alphabet throughout:
`+1` advance/outward, `0` hold/open crossing, `-1` withdraw/inward.

## Transition Framework (`framework_transition.cdc`, capability `H1`)

The catch-all for action, state change, and transitional dynamics.

| role | primitive | exemplar job |
|---|---|---|
| precondition | `guard` open state | `transition-guard` |
| action | `flow` phase motion | `transition-action` |
| fired transition | accepted `commit` | `transition-fire` |
| blocked transition | held `commit` + reason | `transition-blocked` |
| hierarchy | `nest` | `transition-lift` |
| transition log | `trace` | `transition-trace` |
| observation | `measure` | `transition-measure` |
| transition policy | `policy` | `transition-policy` |
| state key | `bridge` coordinate | `transition-bridge` |
| tally | `counter` | `transition-counter` |

A machine configuration is a committed module; its six-trit field trace
projects to one `bridge64` coordinate, so every machine state has a canonical
64-state key. Illegal transitions are held with `balance-violation` rather
than mutating state — retry semantics are native, not bolted on.

```bash
build/cdc_native_runtime run framework_transition.cdc
build/cdc_native_runtime surface framework_transition.cdc
```

## Procedural Framework (`framework_procedural.cdc`, capability `H2`)

Procedural memory: skills as ordered, compilable, refinable step sequences.

| role | primitive | exemplar job |
|---|---|---|
| cue | `flow` | `procedural-cue` |
| executed step | accepted `commit` | `procedural-execute` |
| retry | held `commit` | `procedural-retry` |
| consolidation | `nest` belief integration | `procedural-consolidate` |
| proceduralization | `compile` source -> reducer IR | `procedural-ir` |
| skilled execution | `interpret` IR path | `procedural-ir-exec` |

The classic declarative-to-procedural conversion is literal here: the same
declarative `.cdc` source compiles into reducer IR and then executes through
the interpreter path, with identical checked outcomes on both paths.

```bash
build/cdc_native_runtime run framework_procedural.cdc
build/cdc_native_runtime compile framework_procedural.cdc
build/cdc_native_runtime interpret framework_procedural.cdc
```

## Episodic Framework (`framework_episodic.cdc`, capability `H3`)

Episodic memory: record, consolidate, recall, replay.

| role | primitive | exemplar job |
|---|---|---|
| lived episode | `flow` | `episodic-live` |
| record | accepted `commit` | `episodic-record` |
| consolidation | `nest` into the archive | `episodic-consolidate` |
| recording aperture | `guard` open state | `episodic-guard` |
| episode content | `trace` window | `episodic-trace` |
| recall | committing `measure` | `episodic-recall` |
| recording policy | `policy` | `episodic-policy` |
| memory key | `bridge` coordinate | `episodic-key` |
| episode order | local `counter` | `episodic-ordinal` |

An episode is a window-local ternary trace — episode order needs no global
clock because trace time is window-relative (`trace-order-locality`). The
committed trace consolidates through `bridge64` into a content-addressable
coordinate, and recall runs both directions through the codebook: content
(`110011`) to key (`303`) and key back to content. The `replay` runtime verb
is the re-experience path for recorded reducer output.

```bash
build/cdc_native_runtime run framework_episodic.cdc
build/cdc_native_runtime surface framework_episodic.cdc
build/cdc_bridge_runtime lookup-dyadic bridge64.cdc 110011   # recall by content
build/cdc_bridge_runtime lookup-triadic bridge64.cdc 303     # recall by key
```

## Deliberative Framework (`framework_deliberative.cdc`, capability `H4`)

Decision and agency: deliberate, decide, enact, remember.

| role | primitive | exemplar job |
|---|---|---|
| options | modules | `option-a`, `option-b`, `option-c` |
| deliberation | `council` + `deliberate` quorum | `deliberative-quorum` |
| decision | one bridge coordinate | `111101` -> `331` |
| enactment | `evolve` source write | `deliberative-enactment` |

Option trits project into one occupancy word; the decision is adopted only at
quorum. The adopted coordinate is then enacted by writing a decision-memory
witness into a source copy — deliberation ends in durable source memory, not
a transient outcome. The gate requires the appended witness line itself, so
enactment cannot be satisfied by the copy alone.

```bash
build/cdc_native_runtime council framework_deliberative.cdc
build/cdc_native_runtime evolve framework_deliberative.cdc
```

## The Loop Composition (`framework_loop.cdc`, capability `H5`)

The generic task loop is not only a valid composition of checked slices — it
is executed as one continuous composition over one shared state object.
`framework_loop.cdc` declares a single organism (an `agent` module inside a
`context` module's field) and runs **two full sense → act → integrate cycles
in one runtime invocation**:

| cycle | sense | act | integrate |
|---|---|---|---|
| 1 | `agent.b` phase moves 0 → 0.25 | commit `++0` accepted | context belief 0 → 0.666667 |
| 2 | phase moves 0.25 → 0.492228 (`0.25·cos(0.25)` from the *carried* phase) | commit `++0` accepted | belief accumulates to 1.333333 |

The second cycle's expectations are unreachable without the first cycle's
mutations, so carried state is what the gate checks — the loop runs, it is
not merely diagrammed. The same organism then proceduralizes (`compile` /
`interpret` re-execute both cycles as IR), gates on its open crossing cell,
records its configuration (`++0+0-`), recalls its agent state, refines by
recursive policy, projects its record to a key (`110101` → `311`), deliberates
over its own modules (occupancy 4 at quorum 4 → adopt), and enacts the
decision into source memory. The adopted coordinate **equals** the recorded
key: what the organism records is what it decides.

```bash
build/cdc_native_runtime run framework_loop.cdc        # two cycles, one state object
build/cdc_native_runtime compile framework_loop.cdc    # the loop as reducer IR
build/cdc_native_runtime interpret framework_loop.cdc  # the loop re-executed as IR
build/cdc_native_runtime surface framework_loop.cdc    # gate, record, recall, refine, key, count
build/cdc_native_runtime council framework_loop.cdc    # decide on the recorded coordinate
build/cdc_native_runtime evolve framework_loop.cdc     # enact into build/enacted_loop.cdc
```

The loop is contract-checked as framework `H5` with seventeen required roles
spanning every permitted primitive, including the recurrence roles
(`recur-sense`, `recur-act`, `recur-integrate`) that pin the carried-state
cycle and the `close` role bound to the Universal Operator.

### The Universal Operator (`universal`, capability `U1`)

`bidiγΔ` is the calculus's **open** bidirectional transport between reference
frames. The Universal Operator `𝒰` is its **closed, self-referential
completion** — a higher-order guarded return map, derived over
`flow`/`commit`/`nest` plus `trace`, `bridge`, `council`, and `evolve`, never
a fourth foundational reduction. The loop file is its first executable
instance:

```bash
build/cdc_native_runtime universal framework_loop.cdc
```

One `universal` mode invocation holds **one live runtime object** through
reducer execution, surface trace and bridge projection, council decision,
closure validation, and enactment. Acceptance requires all of: the
`receptive`/`radiant` channels form a reciprocal, angularly biased endpoint
pair active in the same flow evaluation (holonomy `0.375 − 0.25 = 0.125`);
the lifted double-cover frame (projected phase mod 2π, winding count, Z2
sheet) returns its projection with an **inverted** sheet after 360° and
restores both only after **720°**; all local commits are accepted; and the
runtime-computed record coordinate equals the runtime-computed decision
coordinate — independently computed from live state, never compared as
repeated literals. Only then is that computed coordinate enacted, with a
`universal-closure` witness carrying coordinate, winding, sheet, and holonomy.
Interpreter parity runs against a copy of the initial runtime snapshot, never
the live object.

The gate also proves the guard guards: a 360°-only fixture holds with
`full-sheet-mismatch`, a nonreciprocal cone fixture holds with
`cone-not-reciprocal`, and a record/decision mismatch fixture holds with
`coordinate-mismatch` and creates no evolved output. The finite sheet-parity
claims — one turn inverts the Z2 sheet, two turns restore it — are mechanized
in the Lean and Coq mirrors. The Möbius/double-cover picture is a topological
realization of the return path, not a claim that carrier states are physical
spinors, and the continuous holonomy theorem remains a queued obligation.

## Writing Your Own Instance

1. Declare state with existing forms: `field`, `module`, `cell`, `channel`.
2. Pick the loop slices you need and declare their jobs with `expect-*`
   values computed from the kernel semantics (trit of a cell is `cos(theta)`
   against the field deadband; commits walk cells in declaration order under
   the nonnegative-balance barrier; a traced field needs exactly six cells to
   project through `bridge64`).
3. Declare the `framework` contract — `requires=` your roles, `permits=` the
   primitive kinds they may bind — and add `expect framework <key> complete`.
4. Add one binding witness per role, linked to its job, and `expect` lines so
   `cdc_boot.py` enforces both the linkage and the role contract.
5. Execute the file through the matching runtime modes and wire the checked
   output into `scripts/verify.sh`.

## Honest Boundaries

Framework claims live in the `witnessed` and `runtime-checked` tiers of
`VERIFICATION_OBLIGATION_MATRIX.md`, never in `proved`. Explicitly queued, not
claimed:

- **Persistence is declared, but the episodic framework does not use it yet.**
  `H6` (`framework_persistence.cdc`) makes a cross-run durable store a
  first-class source form, and its append path is gated by the same
  balanced-ternary barrier as any commit. `H3` still keeps episodes within a
  run and through `evolve`-written source copies; wiring episodic record and
  recall onto a declared store is queued, not claimed. There is still no
  similarity index or reconstructive retrieval — recall remains exact,
  finite codebook lookup.
- **No learned parameters.** Consolidation updates belief/prior state through
  declared dynamics; nothing is fitted.
- **Recall is codebook lookup**, exact and finite — not similarity-based or
  reconstructive retrieval.
- **Procedure IR is bounded** to `flow`/`commit`/`nest` op kinds; richer
  procedure control flow is a reducer-semantics obligation, not a framework
  patch.
- **Council/evolve generalization** into the main reducer and trace/window
  policy layer remains queued in the matrix, unchanged by this layer.
- **Persistence claims are per-run, not proved.** The durable guarantees
  `H6` exercises are runtime-checked on each execution: the barrier gate,
  the byte-identity of a held append, and the replay/attest divergence under
  compaction are observed outcomes with permanent counterexamples, not
  theorems.
- **Concurrency scope is exactly one lock.** Writers are serialized across
  PROCESSES by an fcntl write lock held through re-scan, append, both
  fsyncs, and the whole compaction transition. Two handles inside one
  process do not block each other; there the fence token does the work, and
  it is compared against on-disk state under the same lock. A writer that
  loses the compare-and-set has a spent handle and must be reopened.
  Distributed or networked stores are queued.
- **Compaction discards history and keeps a commitment to it.** What
  survives is the replay IDENTITY, not replayable events. After compacting,
  the events are gone; the HEAD carries a digest that any future replay
  resumes from.
- **Integrity tags are unkeyed.** They detect corruption and accidental
  mutation, not a motivated forger. The compaction base is identity-bound
  (store uuid plus monotonic generation), which defeats substituting a base
  from another store and replaying a stale one. It does NOT defeat rewriting
  the whole log: rolling a file back to a previous generation is detectable
  only by an observer who retained that generation externally. Ed25519
  signing over the HEAD plus an external anchor is the queued repair.
- **The loop composition spans four runtime invocations** over one declared
  source: the reducer chain (both cycles), the surface pass, the council pass,
  and the enactment pass. A single-process executor that fuses all modes over
  one live state object — and unbounded cycle iteration — are queued, not
  claimed.
- **The framework-contract checker lives in the bootloader** and shares its
  deletion gate: it must reach native/C parity before the Python host can be
  removed.
- **Universal closure is finite and discrete where it is claimed.** The sheet
  parity is proved; the executed 720° fixture is runtime-checked. Continuous
  frame transport, a continuous holonomy theorem, scale-equivariance, and any
  physical light-cone or gravity reading are queued, not claimed.

## Registry

| capability | framework | roles | file |
|---|---|---|---|
| `H1` | transition | 10 | `framework_transition.cdc` |
| `H2` | procedural | 6 | `framework_procedural.cdc` |
| `H3` | episodic | 9 | `framework_episodic.cdc` |
| `H4` | deliberative | 2 | `framework_deliberative.cdc` |
| `H5` | loop (+ `U1` universal closure) | 17 | `framework_loop.cdc` |
| `H6` | persistence | 11 | `framework_persistence.cdc` |

The kernel contract requires all six through `provides`/`expect provides`
(`transition-framework procedural-framework episodic-framework
deliberative-framework persistence-framework framework-contract
task-loop-composition bidi-gated-durable-mutation`), the
framework registry floor and closure (`expect frameworks >= 6`,
`expect frameworks closed`), and the raised capability and witness floors in
`kernel.cdc`. Every framework's role contract is individually enforced by its
`expect framework <key> complete` line.

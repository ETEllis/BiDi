# Native Self-Hosting Mandate

This repository is not finished while the executable center is a host language.

The intended end state is:

```text
.cdc source
  -> .cdc parser/reducer expressed in .cdc
  -> .cdc witnesses expressed in .cdc
  -> minimal host loader only until the native reducer can run directly
```

Python remains only as the bootstrap loader because it is portable and easy to
inspect. It is not the language, not the calculus, and not the semantic
substrate.

## Current Host Boundary

As of v0.2.4, host code is restricted to one Python file:

- `cdc_boot.py`: minimal loader/checker for native `.cdc` declarations.

The bridge also has a non-Python operational consumer:

- `runtime/cdc_source.c` / `runtime/cdc_source.h`: shared native `.cdc`
  line/attribute parser and primitive expectation core used by the bridge and
  reducer runtimes. This removes parser/checker drift between C consumers but
  does not yet replace the whole declaration bootloader.

- `runtime/cdc_bridge_runtime.c`: reads `bridge64.cdc`, validates the finite
  codebook, performs dyadic/triadic lookup, projects six-trit trace occupancy
  into bridge coordinates, executes `bridge_jobs.cdc`, emits interactive
  grid/SVG output, and regenerates/verifies the tracked `bridge512.cdc` and
  `bridge4096.cdc` higher-arity codebooks.

The first reducer path also has a non-Python operational consumer:

- `runtime/cdc_native_runtime.c`: reads `native_reducer.cdc`, initializes native
  field/module/cell/channel state, and executes source-declared `flow`,
  `commit`, and `nest` jobs with checked expectations. It also emits and
  interprets reducer IR, exhaustively checks the finite n=6 balanced-ternary
  walk spectrum from source-declared compile/proof jobs, consumes
  `native_surface.cdc` for guard, trace, measure, policy, bridge, and counter
  jobs, and consumes `council_bridge.cdc` for council deliberation plus
  bridge-coordinate source evolution. It also emits native replay JSON for the
  Flow -> Commit -> Nest -> Trace -> Bridge demo.
- `runtime/cdc_wasm_exports.c`: exposes the native replay JSON path through a C
  ABI suitable for Emscripten. The gate compile-checks this surface and links it
  when `emcc` is available.

All reducer semantics, invariants, capability claims, and witness obligations
must be expressed as `.cdc` declarations. The bootloader may parse, collect, and
check expectations; it may not become the reducer or witness suite.

The minimal bootloader target for the Python phase is:

```text
read .cdc source -> parse lines -> collect native declarations/obligations
                 -> verify expectations -> report
```

Anything beyond that reintroduces host debt unless it has a named native removal
gate.

## Non-Negotiable Direction

The language must become native in its own source language:

- `.cdc` must express the core term syntax;
- `.cdc` must express flow, commit, nest, relation, trace, window, and measure
  semantics;
- `.cdc` must express its own witness programs;
- `.cdc` must express the reducer transition rules well enough that the host
  loader becomes mechanical and replaceable;
- no foundational behavior may depend on host-object semantics.

The discrete layer is balanced ternary: `-1 / 0 / +1`. The middle value is
resting equilibrium and an open crossing state, not binary false or absence.
Self-hosting must preserve this equilibrium-centered carrier.

## Required Burn-Down Gates

Do not remove host files by breaking the executable artifact. Remove them only
when the native replacement is green.

### Gate 1: Native Core Terms

Add `.cdc` forms for declaring the semantic spine itself:

```text
cell
module
channel
field
commit-rule
flow-rule
nest-rule
relation-rule
window-rule
measure-rule
bridge-rule
```

Acceptance: native `.cdc` can declare and verify the same term, rule, invariant,
capability, and witness objects without a Python semantic registry.

### Gate 2: Native Reducer Kernel

Encode the reducer as `.cdc` transition rules over explicit state records.

Acceptance: native `.cdc` owns `kernel.cdc`, `laws.cdc`, `bridge64.cdc`,
`bridge_codebooks.cdc`, `bridge512.cdc`, `bridge4096.cdc`, `bridge_jobs.cdc`,
`native_reducer.cdc`, `native_surface.cdc`, `council_bridge.cdc`, `system.cdc`,
`relations.cdc`, and trace/window witness scenarios through the native contract.

Status: concrete pilot for v0.2.4. `native_reducer.cdc` now declares executable
field/module/cell/channel state plus flow, commit, nest, compile, interpret,
and finite proof jobs, and `runtime/cdc_native_runtime.c` consumes that source
directly. `native_surface.cdc` adds source-declared guard, trace, measure,
policy, bridge, and counter exercises. `council_bridge.cdc` adds a
source-declared council and source-evolution exercise. The native runtime also
emits checked replay JSON for the demo path. Lean and Coq
finite-carrier and finite-algebra mirrors live under `formal/`. The remaining
work is to express the reducer itself in `.cdc` and remove the neutral host
runtime boundary.

### Gate 3: Native Witness Harness

Move law, relation, acceptance, and trace/window witnesses into `.cdc`.

Status: complete for v0.2.4. Verification now invokes `.cdc` files and checks
their declared expectations.

### Gate 4: Host Loader Collapse

Collapse former reducer, semantic-registry, law, acceptance, relation, and
trace/window host modules into native `.cdc` sources.

Status: complete for v0.2.4. The only remaining Python host artifact is `cdc_boot.py`,
and its behavior is fully specified by `kernel.cdc` expectations.

### Gate 5: Host Removal Or Replacement

Replace the remaining host loader with a smaller neutral bootstrap target or
native executable path.

Candidate directions:

- a tiny C/Zig/Rust loader;
- a WASM runtime;
- a generated standalone reducer from `.cdc`;
- a theorem-prover-extracted kernel;
- a direct native binary once `.cdc` has a compiler path.

Acceptance: the repository can verify the language without expanding beyond the
minimal bootloader.

Status: partial concrete pilot for v0.2.4. The bridge no longer depends on
Python for operational lookup: `runtime/cdc_bridge_runtime.c` consumes
`bridge64.cdc` directly, regenerates/verifies the `bridge512.cdc` and
`bridge4096.cdc` codebooks, and emits the interactive bridge grid. The first
reducer path also no longer depends on Python: `runtime/cdc_native_runtime.c`
consumes `native_reducer.cdc`, `native_surface.cdc`, and `council_bridge.cdc`
and executes flow, commit, nest, guard, trace, measure, policy, bridge, counter,
compile, interpret, proof, council, and source-evolution jobs. This does not yet
replace the full declaration bootloader or make `.cdc` self-compiled, but it
moves reducer execution, full-surface checking, IR interpretation, finite proof
checking, council deliberation, source evolution, and demo replay JSON out of
Python and into source-declared native jobs. `runtime/cdc_source.c` now factors
the shared native `.cdc` line parser, typed attribute reader, and primitive
expectation checks used by the bridge, reducer, replay, and WASM export paths.
`runtime/cdc_wasm_exports.c` compile-checks the C ABI for the replay path and
links a live WASM module when Emscripten is available. Live browser WASM
execution and full `cdc_boot.py` parity remain separate deletion gates.

Durable state joined the language on the same terms. `store` and `persist`
are source forms (`framework_persistence.cdc`, capability `H6`), not a host
service the source reaches around the language to call: `op=append` runs the
identical balanced-ternary barrier that governs in-memory latching, and only
an accepted decision may reach `runtime/cdc_store.c`. The `cdc_boot.py`
additions this required are collect-only — the two directives in the form
set, their two step sets, two witness link forms, and two expectation heads.
They carry no persistence semantics and are covered by the existing
**toolchain-verify-parity** deletion gate: `cdc verify --contract` must
produce a byte-identical report to the bootloader on every run, so these
entries retire with `cdc_boot.py` itself under this gate.

**GATE 5 CLOSED on the runtime dependency (2026-07-28, operator decision
Option A, D33).** The language verifies and runs entirely natively: both
runtimes, the contract evaluator, the test runner, the fused executor,
and the Phase I toolchain are C consuming `.cdc` source, and no Python
executes on any production path. `cdc_boot.py` is NOT deleted: it is
FROZEN as a CI-only differential oracle — the project's last independent
implementation of collect-and-verify — exempt BY NAME from the
`python-files` floor, which now expects 0 counted files. `expect
bootloader minimal == true` still enumerates the raw file set, so a
second Python file remains a contract violation. The mandate's stated
end state ("zero Python") was a proxy for "the language does not need a
host to run"; the property is achieved, the proxy is realigned, and the
independent reference survives until native coverage renders it
genuinely redundant — a judgement reserved to the operator.

The **frontend-differential-dump** gate has been split, and the reason is
recorded in DECISIONS D26. It originally named the legacy line scanner and
`cdc_boot.py --dump` for deletion together. The scanner is now deleted: both
runtimes parse through the grammar-1 frontend, so it had no callers. But
`--dump` stays until `cdc_boot.py` itself goes, because it is the last
INDEPENDENT oracle for the frontend — the scanner was a second implementation
inside the same binary, while the bootloader is a separate implementation in
another language. Retiring both at once would leave the frontend checked only
against itself.

## Immediate Rule

From this point forward, new behavior must be added first as `.cdc` source or
as native-semantics documentation. Host code may only appear inside
`cdc_boot.py`, and only when paired with a named native deletion gate.

The project is allowed to use a bootloader. It is not allowed to confuse the
bootloader with the language.

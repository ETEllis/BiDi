<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/identity/mobius-u-code-sigil-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="assets/identity/mobius-u-code-sigil.svg">
    <img src="assets/identity/mobius-u-code-sigil.svg" alt="BiDi Universal Operator system sigil" width="112">
  </picture>
</p>

<h1 align="center">BiDi</h1>

<p align="center">
  <strong>Universal Operator System</strong>
</p>

BiDi is a Universal Operator System that keeps complex transformations
executable, inspectable, and engineerable from formal source through live state,
guarded action, recurrence, and evidence. Its CDC kernel, U1 acceptance, and U2
variation and return analysis give every result the history and earned scope
needed to reproduce it, challenge it, or build from it.

<p align="center">
  <strong><a href="UNIVERSAL_OPERATOR_SYSTEM.md">Understand</a> · <a href="#run">Run</a> · <a href="#verify">Verify</a> · <a href="paper/arxiv/main.pdf">Paper</a></strong>
</p>

## One system, stable parts

- **BiDi** is the integrated source, execution, and evidence system in this repository.
- **CDC** is its formal language, native kernel, and canonical `.cdc` source contract.
- **U1** guards lifted-cover acceptance and effect enactment.
- **U2** differentiates an accepted path and gates return analysis on independently verified recurrence.
- **Möbius** is the embodied reference instrument that makes receipt-bound cover behavior visible.
- **`𝒰_` / `𝒰`** denote the complete executable operator sigil and its reduced mathematical body.

```mermaid
flowchart TD
    Source[".cdc source"] --> Runtime["native state + reductions"]
    Runtime --> U1["U1 guarded acceptance"]
    U1 --> U2["U2 path tangent"]
    U2 --> Gate{"recurrence verified?"}
    Gate -->|no| Hold["typed hold"]
    Gate -->|yes| Spectrum["monodromy + spectrum"]
    Hold --> ReceiptNode["receipt + earned scope"];
    ReceiptNode --> Paper["formal layer + paper"];
    Spectrum --> ReceiptNode;
```

Each transition is earned independently. A hold preserves the last verified
state and emits its receipt instead of fabricating the next one. The machine
record, source digest, and gate remain inspectable at every step.

## Why BiDi exists

BiDi grew through a reciprocal exchange between Edward Ellis's theories,
products, formal work, and executable systems. Each project exposed something
the others needed: a missing language, state contract, behavior, proof
obligation, or instrument. Those gaps became new operator machinery, and the
machinery returned to the surrounding work as sharper mechanisms, engineering
constraints, and testable alternatives.

That theory-product-formalism-runtime loop is the origin of the system and the
reason its layers remain connected. Read the
[origin and evolution](ORIGIN_AND_EVOLUTION.md) for the repository-grounded
history and human-agent build method.

Version **0.3.0** is the current public release:

**262/262 expectations** · **20 terms · 22 rules · 16 invariants**<br>
**46 capabilities · 6 frameworks · 4831 native witnesses** · **ABI 1.5**

## Run

The native path requires Python 3.10+, a C99 compiler, and BLAS/LAPACK (Apple
Accelerate on macOS is supported). The U2 gate builds the current runtime,
executes the canonical path, and exercises its adversarial fixtures:

```bash
git clone https://github.com/ETEllis/BiDi.git
cd BiDi

./scripts/verify_u2.sh --skip-formal
./build/u2/cdc_native_runtime stability framework_loop.cdc
```

The canonical source currently reports:

```text
U1             accepted
analysis       tangent
recurrence     held: recurrence-mode-mismatch
monodromy      not emitted
multipliers    not emitted
```

That hold is the result, not a failed success screen. The two-turn lifted-cover
criterion passes, while latch mode, belief, prior, and unwrapped phase do not
all return. The complete authoritative record is the output line beginning
`u2-json=`.

The positive relative-return calibration is explicit and separate:

```bash
./build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_true_relative_marginal.cdc
```

It declares the endpoint restoration `rho(theta) = theta - 4 pi` and complete
`D rho = I`. In that bounded fixture, `M_rel = I_13` and earns a marginal
relative spectrum. The canonical loop remains governed by its own held
recurrence receipt.

## Verify

The complete repository gate checks the source contract, native runtime,
generated artifacts, durable store, U1/U2 receipts, counterexamples, sanitizer
and concurrency paths, finite formal mirrors, product-source integrity, and the
paper:

```bash
./scripts/verify.sh

# CI-equivalent strict gate; also requires Lean, Rocq/Coq, and Tectonic
./scripts/verify.sh --require-formal
```

The verification unit is always:

```text
(scope, maturity, verdict, receipt-or-obligation)
```

Implementation, finite proof, numerical calibration, and physical law are four
different levels of evidence. BiDi keeps them distinct and binds each public
statement to a receipt or an explicit open obligation. Screens and status
counters remain views over that evidence.

### Current release boundary · 0.3.0

Within the language/runtime/semantic execution boundary, the native CDC parser,
registry, reducer, receipts, U1/U2 path, and unified `cdc` driver are implemented
and gated together.

- Grammar-1 source and native contract: **accepted; executed and adversarially verified**.
- Canonical `framework_loop.cdc` U1: **accepted** for guarded lifted-cover closure.
- Canonical U2 tangent: **accepted** over a source-bound 13-coordinate manifest.
- Canonical complete-state recurrence: **held** with `recurrence-mode-mismatch`.
- Canonical monodromy and multipliers: **not authorized and not emitted**.
- Isolated `4 pi` relative-return fixture: **accepted, marginal** with explicit restoration.
- Carrier, event-order, cone-role, aperture, sheet, and barrier facts: **mechanized finite** in Lean and Rocq.
- Physical and quantum interpretations: **exploratory and not evidenced by this artifact**.

See the [verification obligation matrix](VERIFICATION_OBLIGATION_MATRIX.md) for
the claim-to-receipt inventory and [U2 semantics](docs/u2/U2_SEMANTICS.md) for
the normative recurrence contract.

## CDC: the formal and executable kernel

The kernel is small on purpose. CDC has exactly three primitive reductions:

- `flow(d)` performs synchronous continuous phase evolution;
- `commit(m)` quantizes to balanced ternary and enforces the nonnegative-prefix barrier;
- `nest(parent, child)` performs the audited cross-scale belief/prior exchange.

Trace windows, frameworks, persistence, U1, and U2 are derived jobs over those
reductions. The language may specify richer amplitude, delay, projection,
plasticity, and localized-event semantics than native realization v1 executes;
unconsumed declarations remain public obligations rather than hidden dynamics.

```cdc
field loop-cover-field dt=0.125 gain=0.0 deadband=0.5
module loop-cover field=loop-cover-field belief=0.0 prior=0.0
cell loop-cover.phase module=loop-cover theta=0.0 amplitude=1.0 omega=6.283185307179586

universal loop-u720 frame=agent cover-cell=loop-cover.phase cover=double ...
orbit loop-u720-full universal=loop-u720 coordinates=all quotient=none tolerance=0.000001
variational loop-u720-tangent orbit=loop-u720-full
spectrum loop-u720-spectrum variational=loop-u720-tangent
```

The unified `cdc` driver exposes verification, execution, stability, build,
installation, and trusted-local package commands. Package installation is
journaled and atomic; `cdc x` re-digests installed members before execution but
is not a hostile-package sandbox.

## U1, U2, and polarity

U1 accepts only when reciprocal channel roles, double-cover restoration,
declared holonomy, local commits, record/decision agreement, and effect gating
agree. It earns lifted-cover closure. Complete runtime recurrence remains a
separate U2 question.

U2 closes a coordinate manifest over the exact accepted path, differentiates
the executed maps in source order, records continuous and discrete endpoints,
and verifies full or explicitly restored recurrence before it constructs a
monodromy operator or emits characteristic multipliers. A missing spectral
backend produces a typed hold rather than invented eigenvalues.

Polarity does not collapse into one binary toggle. The architecture treats it
as **apertured oriented reciprocity** wherever a contract declares it: carrier
inversion exchanges `-1` and `+1` while fixing `0`; receptive and radiant roles
may exchange; path orientation may reverse; and double-cover sheet parity
changes after one turn and restores after two. These transformations touch one
another without becoming interchangeable. The system neither forces them into
one symmetry nor promotes their verified software behavior into a universal
physical polarity law. The exact scope and counterexamples are recorded in the
[polarity audit](docs/u2/POLARITY_AUDIT.md).

## Architecture and evidence

Start with the [Universal Operator System](UNIVERSAL_OPERATOR_SYSTEM.md) for the
whole-to-parts model and compatibility boundary. Then use the route appropriate
to the question:

- [CDC language reference](CDC_LANGUAGE.md): syntax and source contract.
- [Formal semantic spine](FORMAL_SEMANTIC_SPINE.md): specification versus audited realization.
- [Frameworks](FRAMEWORKS.md): H1–H6 source-level task vocabulary.
- [Bridge runtime](BRIDGE_RUNTIME.md): bridge generation and runtime behavior.
- [Verification matrix](VERIFICATION_OBLIGATION_MATRIX.md): claims, receipts, counterexamples, and open obligations.
- [U2 semantics](docs/u2/U2_SEMANTICS.md): tangent, recurrence, restoration, and spectral authority.
- [Paper](paper/arxiv/main.pdf) and [source](paper/arxiv/main.tex): CDC formalization and release argument.

## Instruments and research lanes

[CDC Studio and the web evidence console](ui/README.md) are operator instruments
over runtime receipts. Their controls and visual states do not outrank source,
runtime, or receipt evidence.

The [Reference-Frame Topological Coherence lane](docs/rftc/VERIFICATION_OBLIGATION_MATRIX.md)
is deterministic classical local machinery for synchronization, winding,
provenance, admissibility, and record closure. It is not a deployed multi-host
session, physical qubit, entanglement, nonlocality, or quantum advantage.

Möbius identity construction, renders, motion, and provenance live under
[identity documentation](docs/identity/README.md). The interactive
[one-turn/two-turn motion study](demo/mobius-identity.html) is a reference
instrument. **Möbi𝒰s remains a proposed product-linked identity**, not the name
of the formal kernel or a replacement for the BiDi system.

## Compatibility

A clearer name is not permission to break the working contract. The system
identity does not rename the package, executable, file format, ABI, C symbols,
or formal namespaces. Existing automation continues to use
`bidi-coherence-delta-calculus`, `cdc`, `.cdc`, and the established CDC symbols.
Historical specifications remain versioned provenance, not current release
authority.

## Citation and license

BiDi is released under the [MIT License](LICENSE). Cite the repository through
[CITATION.cff](CITATION.cff) and cite the CDC paper for the formal kernel and
U1/U2 release argument.

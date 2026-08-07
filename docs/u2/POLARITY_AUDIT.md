# Polarity Audit: What BiDi/CDC Already Earns and What It Must Not Force

Status: source-grounded audit plus implemented derived covariance gate

Source baseline audited: `a4ce53bdb63ad39c77bd5bfb248239c33d573d9b`

Question: is polarity already universal in the Universal Operator, and what—if
anything—should be added without imposing a binary ontology on the calculus?

## Verdict

**Polarity is structurally present at the Universal Operator level, but a single
universal polarity law is not yet formalized or earned across all of CDC.**

What the implementation already earns is stronger and less forced than “all
things come in binary opposites”:

> An accepted Universal Operator closure requires frame-relative, reciprocal
> inward/outward causal directions inside a balanced-ternary carrier that keeps
> a real equilibrium/aperture, while preserving signed phase and residual
> holonomy rather than requiring the two directions to cancel.

Call this **apertured oriented reciprocity**. It is the strongest non-forced
invariant common to the polarity cluster:

1. orientation is relative to a declared frame;
2. receptive and radiant are roles of directed relations, not substances;
3. the crossing/equilibrium state is fixed and real, not binary false;
4. two directions may be reciprocal without being identical or canceling; and
5. closure preserves their path-dependent mismatch as holonomy.

That lands unusually well with a polarity-first physical theory because it can
represent opposed orientation, mediation, residual phase, broken symmetry, and
closure. It does **not** yet establish that every CDC term, every physical
interaction, or nature itself is governed by one binary polarity.

The minimal honest addition is a **derived, typed polarity-covariance witness**
over selected realizations. That matrix/map-level gate is now implemented as
`cdc_u2_polarity_covariance_check`; it is not a new foundational reduction.
It accepts only an explicit involution, fixed aperture, primal conjugacy, and
U2 tangent conjugacy. No `.cdc` declaration is yet bound to it, and no global
polarity claim follows from the analytic acceptance fixture.

## 1. Evidence ledger

| Candidate polarity | What the repository actually contains | Earned status |
|---|---|---|
| receptive / radiant | `execute_universal` requires named channels labeled `receptive` and `radiant`, sharing a pair id, with swapped endpoints, nonzero angles, and endpoints in the frame field | Runtime-enforced for accepted `universal`; not required for arbitrary CDC channels |
| inward / outward | The language documents receptive influence into a frame and radiant influence outward; committed `-1` / `+1` are interpreted as inward contraction / outward expansion | Interpretive mapping grounded in direction and trit sign; not one global physical law |
| signed phase | Flow uses `sin(theta_source + angle - theta_target)` and the carrier has phase inverse/rotation structure | Executable continuous orientation; a circle, not a binary set |
| context / evidence | `bidiγΔ` describes context downward and evidence/coherence upward; native `nest` raises child mean trit into parent belief then copies the new parent belief into child prior | Executable bidirectional exchange, but asymmetric and role-typed rather than opposite scalars |
| balanced ternary | `-1`, `0`, `+1`; Lean/Coq define `invCarrier` swapping signs and fixing zero | Mechanized carrier inverse exists; an explicit involution theorem is not currently registered |
| flow / commit / nest | Three foundational reductions: continuous evolution, discrete latch-or-hold, and cross-scale exchange | A triad of different semantics, not a polarity pair |
| U closure | Reciprocal cones, half-turn sheet inversion, two-turn restoration, declared holonomy, accepted commits, record/decision equality | Runtime-enforced guarded closure; not full-state periodicity or a physical polarity theorem |
| Z2 sheet | One turn flips the sheet and two turns restore it; Lean/Coq prove finite parity facts | Genuine binary parity, but it is not the same object as trit sign, channel direction, or physical charge |

This distinction matters: multiple compatible dual structures reinforce the
architecture, but resemblance does not prove that they are projections of one
ontic physical polarity.

## 2. Exact U-level polarity already implemented

For an accepted `universal` job, let the two declared channels be

\[
e_r:a\rightarrow b\quad\text{(receptive)},\qquad
e_o:b\rightarrow a\quad\text{(radiant)}.
\]

The native executor checks:

- the labels are exactly receptive/radiant;
- both channels share a nonempty `pair` identity;
- source and target endpoints are reversed;
- both angles are nonzero;
- both endpoints belong to the Universal Operator frame's field; and
- at least one flow step executes in that frame field.

It then records

\[
\beta=\alpha_r+\alpha_o
\]

as holonomy and accepts nonzero `beta` when it matches the source declaration.
The current example deliberately uses `0.375 + (-0.25) = 0.125`, so reciprocal
does not mean equal-and-opposite cancellation.

This is a meaningful polarity result: both orientations must participate in the
same closure and the unmatched phase is retained. It is also carefully scoped.
The generic `channel` form accepts directed channels without cone labels or a
partner, so reciprocal polarity is an invariant of accepted U closure, not of
every legal field.

## 3. Why a single binary polarity would distort CDC

### 3.1 The aperture is irreducible

Balanced ternary contains a real fixed center:

\[
\rho(-1)=+1,\qquad \rho(0)=0,\qquad \rho(+1)=-1.
\]

The zero state is equilibrium and an open crossing. Collapsing it into either
pole destroys the commit carrier, guard semantics, trace outcomes, and the
system's capacity to represent a boundary that is open without being absent.

### 3.2 Signed phase is circular, not binary

Phase lives on `R / 2pi Z`; `theta` and `theta+pi` can be complementary, but
there are infinitely many orientations and path-dependent sums. A global
positive/negative label would erase angular detuning and holonomy—the very
quantities that make U closure informative.

### 3.3 Reciprocal channels need not be mirror-identical

The U runtime requires reversed endpoints and reciprocal roles but does not
require equal weights, opposite angles, or zero holonomy. Imposing those
conditions globally would reject the existing accepted `loop-u720` witness and
remove the residual phase the operator is designed to preserve.

### 3.4 The commit barrier chooses an admissibility orientation

The nonnegative-prefix barrier is not invariant under naive sign exchange. For
example, `+-` is admissible, while its pointwise sign inverse `-+` violates the
first prefix. Thus `invCarrier` is an algebraic inverse, but accepted commit
paths are not globally sign-symmetric.

For localized walks ending at zero, reverse-order plus sign inversion can
preserve Dyck/Motzkin-style admissibility. The runtime also accepts admissible
nonlocalized walks, for which that construction fails. Claiming global polarity
symmetry would therefore contradict the executable safety orientation.

### 3.5 `flow`, `commit`, and `nest` are not opposites

`flow` is a continuous/synchronous phase map, `commit` is a discrete
quantize-and-latch-or-hold transition, and `nest` is a scale-transfer update.
They compose; none is the inverse or negative of another. Turning them into
positive/negative labels would remove the calculus's hybrid and multiscale
content.

### 3.6 Context and evidence are typed roles

In the native `nest` map, child trit coherence increments parent belief and the
new parent belief overwrites child prior. These directions are complementary,
but their data types and transformations differ. “Up” is not the numeric
negative of “down,” and swapping them requires an explicit role/type map.

### 3.7 Sheet parity is not carrier polarity

The `Z2` sheet records cover parity. Trit sign records a committed pole. Cone
direction records causal orientation. They may be coupled in a specialization,
but identifying them by name alone would collapse three different state spaces.

### 3.8 Commit is generally irreversible

Latch, barrier, bridge, decision, and enactment can be many-to-one or effectful.
Polarity reversal therefore cannot be assumed to mean time reversal or inverse
dynamics. The correct general concept is covariance under a declared
orientation map, not universal reversibility.

## 4. The minimal addition now implemented

The C API now supplies one **derived analysis contract**, provisionally named
`oriented-reciprocity`. It does not add a reduction rule. Its caller supplies
a typed map

\[
\rho_F:X_q\rightarrow X_{\rho(q)}
\]

for a selected frame/realization and must satisfy:

### 4.1 Involution and fixed aperture

\[
\rho_F\circ\rho_F=\operatorname{id},\qquad
\rho_F(A_F)=A_F,
\]

where `A_F` is the declared equilibrium/aperture set. For the trit carrier,
`rho_F` swaps `-1/+1` and fixes `0`. For phase, the source must specify whether
the action is sign reversal, a `pi` rotation, endpoint reversal, frame
orientation reversal, or another typed map; these are not interchangeable.

### 4.2 Relation-role reversal

`rho_F` maps a receptive relation in one oriented realization to the radiant
relation in the conjugate realization and swaps its endpoints. Parameter
transforms—angle sign, weight/constitutive sign, line projection, and delay—must
be declared by type. They MUST NOT be guessed from the words receptive/radiant.

### 4.3 Operational covariance

For every selected smooth segment, event, and reset, the conjugate realization
must satisfy the applicable equations:

\[
\rho_F\,\varphi_q^t(x)=
\varphi_{\rho(q)}^{\rho,t}(\rho_Fx),
\]

\[
g_{\rho(e)}(t,\rho_Fx)=s_e g_e(t,x),\qquad s_e\in\{-1,+1\},
\]

\[
\rho_F R_e(t,x)=R_{\rho(e)}^{\rho}(t,\rho_Fx).
\]

The crossing direction must transform with `s_e`. A barrier or reset that does
not satisfy this relation is an honest symmetry-breaking operation, not a
failure of the calculus.

At Universal Operator scale, the earned equation is conjugacy/covariance,
not inversion:

\[
\rho_F\,\mathcal U_X
=\mathcal U_X^{\rho}\,\rho_F.
\]

This remains meaningful when `𝒰` is lossy and noninvertible.

### 4.4 U2 tangent consequence

Differentiating the covariance relation gives the executable test

\[
D\rho_F|_{x_T}\,D\mathcal U_X|_{x_0}
=D\mathcal U_X^{\rho}|_{\rho_Fx_0}\,D\rho_F|_{x_0}.
\]

For a verified recurrent/conjugate pair with compatible endpoint restoration,
the monodromy operators are similar:

\[
M^{\rho}=D\rho_F\,M\,(D\rho_F)^{-1},
\]

and therefore have the same characteristic multipliers. This is the highest
value addition: U2 makes “polarity is preserved through the whole engineered
closure” a numerical and formal claim that can fail, rather than a metaphor.

### 4.5 Exact executable boundary now

`runtime/cdc_variational.{c,h}` exposes a fail-closed specification containing
the original and conjugate executable maps, source/target polarity maps, their
four exact derivatives, a source point, a fixed-aperture point, and a numeric
tolerance. It reports scale-normalized residuals for:

- source and target involution;
- aperture fixation;
- primal closure covariance; and
- U2 tangent covariance.

A symmetry-breaking realization returns a typed held witness while remaining
valid CDC. Three permanent mutants reject ignoring the aperture, skipping the
tangent consequence, and accepting polarity by name. The finite Lean/Coq layer
proves carrier/cone/sheet involutions separately and permanently exhibits the
`+-` versus `-+` prefix-barrier counterexample.

This implementation earns the mechanism. It does not yet bind a declared
`polarity` form to a native Universal Operator execution, compare recurrent
conjugate spectra, or promote the accepted analytic fixture into a claim about
`loop-u720` or physics.

## 5. Recommended source-language surface

A minimal future declaration binding the implemented gate could be:

```text
polarity loop-orientation
  frame=agent
  map=loop-frame-reversal
  aperture=balanced-zero
  receptive=loop-receptive
  radiant=loop-radiant
  scope=universal
```

The exact grammar may be compressed, but the receipt MUST expose:

```text
polarity=<id> scope=<scope> map=<map-digest>
involution-residual=<real> aperture-fixed=<yes|no>
flow-covariance-residual=<real>
event-covariance=<accepted|held|not-applicable>
closure-covariance-residual=<real>
tangent-covariance-residual=<real>
spectrum-match-residual=<real|not-applicable>
status=<accepted|held> reason=<typed-reason>
```

Minimum hold reasons:

- `polarity-map-not-involutive`;
- `aperture-not-fixed`;
- `relation-not-conjugate`;
- `flow-not-covariant`;
- `event-not-covariant`;
- `closure-not-covariant`;
- `tangent-not-covariant`; and
- `spectrum-not-comparable`.

Do not silently downgrade a failed covariance test to a warning. A
symmetry-breaking model may still be valid CDC; it simply does not earn the
polarity witness.

## 6. Formal and executable gates

### 6.1 Finite formal layer

The finite layer now contains explicit Lean/Coq statements, rather than relying
on inspection of the three-case definition:

1. `invCarrier (invCarrier t) = t`;
2. `invCarrier zero = zero`;
3. `value(invCarrier t) = -value(t)`;
4. cone-pair reversal applied twice is identity; and
5. sheet flip remains a separate involution, with no theorem equating it to
   `invCarrier`.

Do **not** add a theorem that naive trit sign inversion preserves the existing
commit barrier; it is false. Add the `+-` / `-+` counterexample permanently.

### 6.2 Runtime layer

The current API fixtures cover an accepted analytic conjugacy, fixed-aperture
failure, tangent-only failure, and non-involutive-map failure. Binding the gate
to source/runtime still requires:

- accepted reciprocal receptive/radiant U pair;
- missing partner holds;
- same-direction endpoints hold;
- role labels swapped without endpoint reversal hold;
- nonzero residual holonomy remains accepted when declared;
- `rho_F^2` restores the original typed state;
- fixed aperture remains fixed;
- one covariant flow pair passes;
- one explicitly symmetry-breaking flow or barrier pair holds the polarity
  witness while remaining a valid CDC program;
- U2 tangent conjugacy passes for an analytic fixture; and
- recurrent conjugate monodromy spectra agree within tolerance.

Mutants must kill omitted role reversal, forced zero holonomy, conflation of
sheet/trit involutions, and automatic polarity acceptance from naming alone.

## 7. What “universal” may mean after this addition

Three claim levels must remain distinct:

### Level A — already earned

> Every accepted CDC Universal Operator instance contains an executable
> reciprocal receptive/radiant channel pair in one frame and closes with a
> declared residual holonomy, within a carrier that retains a real ternary
> aperture.

This is source- and runtime-backed now.

### Level B — earned after the proposed witness

> A selected CDC realization is covariant under a declared frame-relative
> polarity involution, including its primal closure and U2 tangent dynamics.

This is testable and may be earned per realization. It should initially be
called `polarity-covariant`, not universally polarity-symmetric.

### Level C — physical universality, not earned by the calculus alone

> The same polarity involution is a constitutive invariant of a physical domain
> or of nature across domains.

This additionally requires measured variable bindings, units, constitutive
equations, conservation/continuity laws, calibration data, symmetry-breaking
conditions, competing explanations, and discriminating experiments. CDC/U2 can
execute and falsify such a specialization; mathematical recurrence cannot
promote it to empirical truth by itself.

## 8. Exact recommendation

1. **Keep U2 exactly hybrid and nonbinary.** Do not add “polarity” as a fourth
   primitive beside `flow`/`commit`/`nest`.
2. **Name the existing U-level fact** `apertured-oriented-reciprocity`: two
   reciprocal causal roles, fixed/open center, signed phase, residual holonomy.
3. **Keep the implemented derived `polarity` witness scoped**, and bind it to a
   source declaration only with explicit involution and covariance obligations.
4. **Use U2 as its decisive gate:** primal covariance, tangent conjugacy, and,
   only for verified recurrent cases, monodromy-spectrum equivalence.
5. **Preserve asymmetry when real.** The commit barrier's orientation, lossy
   resets, causal delay, and nonzero holonomy are allowed symmetry breakers.
6. **Promote no physical claim without a specialization packet** defining
   observables, dimensions, constitutive dynamics, and falsifiers.

The answer to the motivating question is therefore: **yes, the architecture
already captures the fundamental polarity pattern at the Universal Operator's
most important structural point; no, it has not yet collapsed that pattern into
one formal universal law—and it should not.** One small covariance witness can
make the stronger claim executable without forcing the calculus or the physics.

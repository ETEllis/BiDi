# Born Closure Derivation Program

Status: **R0 open; this document records the proof obligation, not a
derivation already achieved**

## 1. Target

Derive the probability measure used by record closure from independently
motivated RFTC axioms. The reducer, random source, fixture, and test threshold
may not already contain squared-amplitude probabilities.

For mutually exclusive outcomes represented by orthogonal components
\(\psi_i\), the target is:

\[
p(i\mid\psi)=\frac{|\psi_i|^2}{\sum_j|\psi_j|^2}.
\]

Reproducing this histogram after sampling from the target distribution earns
nothing.

## 2. Candidate closure measure

Let \(\mu_F(\psi,P)\) be the measure that a frame closes into record projector
\(P\). Candidate requirements are:

1. **normalization**
   \[
   \mu(\psi,I)=1;
   \]
2. **exclusive additivity**
   \[
   P_iP_j=0
   \Rightarrow
   \mu(\psi,P_i+P_j)=\mu(\psi,P_i)+\mu(\psi,P_j);
   \]
3. **nested multiplicativity** for independently composed closure channels;
4. **frame-preserving invariance** under allowed basis/gauge transformations;
5. **continuity** under continuous change of state and interaction;
6. **coarse/refine consistency** when a channel is split and recombined;
7. **representation independence** under arbitrary subdivision of an
   unchanged physical outcome;
8. **local/global conditioning compatibility**;
9. **null-channel refusal** when \(P\psi=0\);
10. **noncontextual record identity**: the weight assigned to the same
    projector does not depend on which compatible exclusive partition contains
    it.

The research task is to map every condition to a frozen RFTC axiom or reject
it as an imported assumption.

## 3. Competing family

Begin with:

\[
\mu_\alpha(i\mid\psi)
=
\frac{|\psi_i|^\alpha}
{\sum_j|\psi_j|^\alpha}.
\]

Test \(\alpha>0\), including branch counting and absolute-amplitude weighting.
Do not fit \(\alpha\) after observing quantum target data.

Multiplicativity alone does not necessarily select \(\alpha=2\). The proof
must identify which closure/refinement constraint rejects every surviving
alternative.

Additional competitors:

- equal branch counting;
- basis-dependent weights;
- history-count measures;
- maximum-entropy closure;
- nonlinear contextual weights;
- stochastic hidden-record models;
- signed or quasi-probability measures.

## 4. Comparison theorems

Gleason-type, envariance, decision-theoretic, noncontextuality, and Dutch-book
results are comparison points. They do not constitute an RFTC derivation.

For every imported theorem, the proof packet must contain:

```text
exact theorem statement
all hypotheses
RFTC object corresponding to each hypothesis
proof that the correspondence holds
translated conclusion
countermodel when one hypothesis is removed
```

The two-dimensional case, composite systems, finite precision, and generalized
measurements are separate obligations rather than footnotes.

## 5. RFTC mapping candidates

| Probability condition | Candidate RFTC source | Status |
|---|---|---|
| normalization | one committed record across exhaustive channels | open |
| additivity | exclusive boundary channels compose without overlap | open |
| multiplicativity | `NEST` of independent frames | open |
| invariance | canonical frame-preserving translation | open |
| continuity | continuous interaction before record closure | open |
| refine consistency | renormalize/refine round trip | open |
| representation independence | hidden granularity cannot change macro weight by relabeling | open |
| conditioning | predecessor/causal-record chain | open |
| noncontextual projector weight | same recoverable record across compatible frame descriptions | open |

“Open” means the implementation has not established the mathematical
implication.

## 6. Formal work products

R0 requires:

1. a machine-readable axiom set;
2. a proof that one measure exists;
3. a proof of uniqueness or an explicit surviving family;
4. countermodels for every removed axiom;
5. a finite-dimensional executable checker;
6. symbolic and numerical tests across bases and composite systems;
7. independent proof review;
8. a resource ledger showing the result was not target-sampled.

If several measures survive, the verdict is `R0_HOLD_ALTERNATIVES`, not a
post-hoc choice of the Born measure.

## 7. Executable crucible

The crucible freezes:

- amplitudes and bases before sampling;
- alternate decompositions of the same projector;
- composite and marginal experiments;
- refinement/coarse-graining round trips;
- adversarial relabeling;
- alternate seeds;
- the measure exponent/family before target comparison.

Required negative controls intentionally:

- sample from the Born target;
- hide target probabilities in thresholds;
- branch-count after arbitrary subdivision;
- use basis-dependent lookup tables;
- fit \(\alpha\) after the run;
- discard mismatched outcomes.

The harness must detect and reject every control.

## 8. Promotion

| Verdict | Meaning |
|---|---|
| `R0_REJECTED` | axioms inconsistent or no admissible measure |
| `R0_HOLD_ALTERNATIVES` | more than one measure survives |
| `R0_CANDIDATE` | unique finite-model result, formal/general proof pending |
| `R0_EARNED` | declared RFTC axioms uniquely imply the measure and independent review passes |

R0 does not imply Q0. A classical formal system may derive the same operational
measure without demonstrating a physical nonclassical substrate.

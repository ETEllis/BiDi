# Cosmological Horizon Atlas Specification

Status: **Model D / H0-G0 design contract; executable atlas pending**

## 1. Objective

Construct multiple observer-relative causal frames without a privileged global
ledger, then determine whether lawful record translation and overlap
consistency produce a useful effective geometry.

The verifier may know the full synthetic world for scoring. Executing frames
may not.

## 2. Frame record

```text
AtlasFrame {
  frameId
  localClock
  writableAlgebra
  readableRecordAlgebra
  boundaryChannels[]
  causalHorizon
  topology
  hiddenClassDigest
  predecessorClosure
}
```

## 3. Translation record

```text
FrameTranslation {
  sourceFrame
  destinationFrame
  overlapId
  sourceRecordDigest
  translatedRecord
  retainedInvariants[]
  discardedDistinctions[]
  delay
  cost
  attenuation
  errorBound
  causalReceipt
}
```

Translations are directional and need not be invertible. Claims of equivalence
require both directions and an explicit round-trip bound.

## 4. Horizon

For frame \(F\), the present horizon \(H_F(t)\) partitions distinctions into:

- presently writable/readable;
- committed but awaiting a causal channel;
- compatible hidden distinctions;
- permanently excluded by the tested model.

Crossing \(H_F\) changes current accessibility. It does not delete global
provenance in the verifier.

## 5. Overlap law

For shared overlap \(O_{ij}\):

\[
d\left(
\operatorname{Recover}_{F_i}(O_{ij}),
\mathcal T_{i\leftarrow j}
(\operatorname{Recover}_{F_j}(O_{ij}))
\right)
\le\epsilon_{ij}.
\]

The metric \(d\), tolerance \(\epsilon_{ij}\), permitted age, and causal path
are declared before execution.

An overlap failure produces one typed diagnosis:

```text
NOT_SHARED
BOUNDARY_MISDECLARED
TRANSLATION_INCOMPLETE
CAUSAL_HISTORY_MISSING
TOPOLOGY_MISMATCH
GEOMETRY_INCONSISTENT
```

## 6. Effective geometry

Candidate observables:

- adjacency from available translation channels;
- directed causal order from record ancestry;
- distance from minimum cost/delay/reconstruction loss;
- proper time from local committed-order accumulation;
- curvature from loop holonomy;
- horizon from reachability under present causal bounds.

For path \(\gamma=i\to j\to\cdots\to i\):

\[
\mathcal H_\gamma
=
\mathcal T_{i\leftarrow n}\cdots\mathcal T_{j\leftarrow i}.
\]

The residual:

\[
\delta_\gamma
=
d(z_i,\mathcal H_\gamma z_i)
\]

is a holonomy observable. Calling it curvature requires a derivation connecting
it to an accepted geometric quantity or a distinct predictive result.

## 7. Model D experiment

1. Generate a local-interaction world with a frozen causal graph.
2. Instantiate frames with different locations, clocks, and horizons.
3. Prevent frames from reading global state.
4. Publish local records and causal boundary transactions.
5. Reconstruct shared overlaps through translations.
6. Move or expand horizons by lawful evolution.
7. Measure overlap consistency and loop holonomy.
8. Infer an effective adjacency/distance/causal structure.
9. Compare it to the hidden generating geometry only in the verifier.
10. Replay the full atlas from receipts.

## 8. Required controls

- privileged global ledger;
- hidden all-to-all communication;
- coordinates embedded in record identifiers;
- direct shortest-path oracle;
- arbitrary graph with no geometric generator;
- flat translation system;
- known curved synthetic manifold;
- topology-destroyed translations;
- stale/missing causal history;
- inconsistent overlap injected at one frame;
- coordinate relabeling/gauge transformation.

## 9. Boundary-capacity experiment

Vary:

- interior volume;
- boundary channel count;
- graph cut rank;
- energy/message budget;
- noise/error tolerance;
- topology;
- causal delay.

Measure maximum independently recoverable distinctions. Fit preregistered
candidate laws:

```text
constant
log(volume)
boundary channel count
cut rank
area-like measure
volume
power law with fitted exponent
```

Model selection penalties and held-out sizes are mandatory.

## 10. Relational dissipation experiment

From a newly closed reference record, measure mutual recoverability over
relational distance and causal delay. Test:

\[
D(r)\in
\{r^{-\alpha},e^{-r/\xi},\log^{-1}(r),\text{finite range},
\text{topology dependent}\}.
\]

The shared-zero/power-law hypothesis earns support only if the power law
predicts held-out frames better than alternatives and the exponent remains
stable under coordinate relabeling.

## 11. H0 and G0

H0 may be earned by an executable boundary-sufficiency mechanism without
gravity.

G0 requires all of:

- covariant or explicitly gauge-independent state/translation definitions;
- causal cones;
- proper-time relation;
- redshift or analogous gauge-invariant observable;
- horizon formation;
- conservation ledger;
- a recovered low-energy geometric limit;
- a new preregistered prediction beyond ordinary models;
- independent adversarial review.

Record holonomy in a synthetic graph is not G0.

## 12. Falsifiers

Model D fails if:

- overlap agreement depends on global hidden access;
- inferred geometry is encoded in identifiers;
- holonomy vanishes or appears only through numerical artifact;
- coordinate relabeling changes physical verdicts;
- causal reconstruction uses future records;
- horizons are visual masks rather than access constraints;
- capacity scaling is selected after seeing the data;
- or the same closure operator must be replaced to make the atlas work.

# Horizon Foundry Specification

Status: **H0/B0 design contract; implementation and physical claims pending**

## 1. Definition

A closure foundry is a bounded process that:

1. accepts interacting degrees of freedom and their provenance;
2. declares a causal boundary and admissible local interactions;
3. makes some distinctions inaccessible to an external frame;
4. publishes a smaller recoverable boundary record;
5. preserves commitment to the compatible hidden class;
6. allows the record to become a next-scale primitive;
7. returns effects that change future admissible state.

Detectors, logical cells, phase transitions, black holes, and cosmological
horizons may instantiate this grammar. They are not presumed physically
identical.

## 2. Typed interface

```text
FoundryInput {
  frameId
  predecessorRecord
  microstateCommitments[]
  interactionEdges[]
  boundaryChannels[]
  causalHorizon
  energyLedger
  topology
}

FoundryClosure {
  frameId
  macroRecord
  hiddenClassDigest
  boundaryRecordDigest
  closureCapacity
  recoverableDistinctions[]
  inaccessibleDistinctionCommitment
  decision
  returnEffect
  resourceLedger
}

FoundryEmission {
  sourceClosureDigest
  emissionClock
  accessibleRecord
  correlationCommitment
  energyDelta
  causalParent
}
```

All arrays and ledgers are bounded. All commits are canonical and replayable.

## 3. Invariants

### F1 — exterior sufficiency is tested, never presumed

An exterior consumer receives only boundary channels. Reconstruction code may
not inspect interior state, interior indices, or a central cache.

### F2 — hidden does not mean destroyed

The hidden-class commitment must distinguish:

- information not presently accessible to the exterior;
- information destroyed by a noninvertible model;
- information retained globally but redistributed into correlations.

### F3 — no target curve

The foundry may not receive an area law, thermal curve, Page curve, entropy
curve, or expected turnover time as input.

### F4 — conservation ledger

Every transition balances the declared conserved quantities or returns a typed
failure. Untracked deletion is not evaporation.

### F5 — local authority

No exterior observer gains a read capability that crosses the declared
horizon. No interior component writes an exterior fact without a causal
boundary transaction.

### F6 — replay identity

Same input, interaction order, seed, and boundary contract reproduce the same
receipts. Alternate scheduling order must not change an order-independent
result.

## 4. Black-hole mapping

The provisional mapping is:

| Foundry element | Black-hole candidate |
|---|---|
| interior microstate class | compatible interior relational configurations |
| boundary | event/apparent horizon in the chosen model |
| macro-record | exterior mass, charge, angular momentum, response variables |
| hidden-class digest | commitment to exterior-indistinguishable interior detail |
| emission | Hawking-like outgoing record/correlation channel |
| return | backreaction on geometry, capacity, and future emission |

The mapping earns no physics claim until a declared gravitational model
supplies covariant dynamics and gauge-invariant observables.

## 5. Information metrics

For interior \(I_t\), exterior record \(E_t\), and emitted record \(R_t\):

\[
S_{\mathrm{hidden}}(t)=H(I_t\mid E_t,R_{\le t}),
\]

\[
\mathcal I_{\mathrm{ext}}(t)=I(I_0;E_t,R_{\le t}),
\]

\[
C_t=C(\partial F_t).
\]

A Page-like turnover is detected by a preregistered change in the information
partition, not by fitting the Page curve after the run.

Candidate crossing:

\[
I(I_0;R_{\le t}) > I(I_0;I_t\mid E_t).
\]

The exact estimator, finite-size correction, confidence interval, and null
ensemble are frozen before execution.

## 6. Model C construction

1. Create a locally interacting interior graph with no exterior read path.
2. Declare a bounded boundary channel set.
3. Reduce the exterior-facing state through the frozen closure operator.
4. Evolve the interior and boundary locally.
5. Emit records only through causal boundary transactions.
6. Apply emission backreaction to interior energy and boundary capacity.
7. Track exact global provenance in the verifier, inaccessible to the model's
   exterior agent.
8. Attempt exterior reconstruction from boundary and emission records.
9. Replay from receipts.

The verifier may inspect global state only after the run and must never feed it
to an executing observer.

## 7. Required comparators

- random local emission with matched marginal distribution;
- thermal sampler with no retained correlations;
- hidden central database reconstruction;
- hard-coded Page turnover;
- hard-coded area capacity;
- volume-limited capacity;
- edge-count/cut-rank capacity;
- reversible unitary reference where feasible;
- noninvertible information-destroying reference.

## 8. H0 gate

H0 requires:

- externally actionable reconstruction through boundary channels only;
- measured closure capacity;
- correct failure under capacity saturation;
- topology/translation dependence;
- independent replay;
- superiority to matched non-boundary controls;
- no target-coded area law.

H0 is a boundary-record mechanism result, not a black-hole result.

## 9. B0 gate

B0 additionally requires:

- a declared gravitational black-hole model;
- many-to-one exterior macro-reduction;
- exterior thermal appearance under preregistered metrics;
- globally preserved provenance;
- recoverable emitted correlations;
- emergent Page-like turnover;
- conservation/backreaction;
- comparison with accepted black-hole information behavior;
- independent audit with authority to block.

No simulated B0 result is automatically evidence about an astrophysical black
hole. The verdict must name the substrate and model assumptions.

## 10. Falsifiers

The foundry hypothesis fails for the tested model if:

- exterior recovery needs hidden global access;
- correlation recovery is a seed/lookup-table artifact;
- the turnover disappears under preregistered alternate sizes or seeds;
- the boundary offers no explanatory advantage over a matched arbitrary cut;
- capacity scaling is inserted rather than produced;
- energy/provenance accounting does not close;
- or the same operator cannot serve Models A, B, and C without changing
  semantics.

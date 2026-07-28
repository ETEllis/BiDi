# Quantum-Classical Dissolution Crucible

Status: **QS0/QS1 test contract; Q0/Q1 require separate physical evidence**

## 1. Question

Can the same closure operator reproduce the operational structure associated
with quantum theory without:

- target-coded statistics;
- prohibited communication;
- hidden global state access;
- postselection;
- lookup-table contextuality;
- API-enforced no-cloning;
- or exponential resources disguised as primitives?

A distributed classical mesh is not quantum merely because its topology or
language resembles a qubit.

## 2. Required distinction ladder

```text
topologically classified logical cell
  != topological classical memory
  != operational qubit
  != physical topological qubit
  != fault-tolerant quantum computation
  != quantum computational advantage
```

Every report names exactly one level.

## 3. Model A — microscopic closure

Model A implements:

\[
\text{prepare}
\to
\text{transform}
\to
\text{couple record channel}
\to
\text{close}
\to
\text{condition/recombine where lawful}.
\]

The state, operations, record channels, and resource costs are exposed through
the future quantum ABI. A reference simulator and an independently implemented
oracle validate QS semantics.

## 4. Interference

Sweep a continuous relative phase \(\phi\). Measure:

- fringe phase;
- visibility;
- dependence on which-record overlap;
- recovery after lawful erasure/recombination;
- energy, memory, communication, and synchronization.

Required relationship:

\[
V \leftrightarrow |\langle E_1|E_0\rangle|
\]

within preregistered tolerance for the reference model.

Controls:

- hard-coded sine curve;
- binary wave/particle switch;
- analysis-time record deletion;
- phase labels unavailable to the local detector;
- matched classical oscillator/interferometer.

## 5. Born statistics

Use the measure from
[`BORN_CLOSURE_DERIVATION.md`](BORN_CLOSURE_DERIVATION.md).

The experiment freezes amplitudes, bases, sample count, confidence procedure,
and alternate seeds before outcomes. It rejects any sampler parameterized by
the target distribution.

Until R0 is earned, the quantum ABI may run a standard Born sampler only when
explicitly labeled `REFERENCE_QS`, never as evidence for the new derivation.

## 6. Contextuality

Implement at least one Peres-Mermin or equivalent witness.

Requirements:

- contexts are real operation sequences;
- shared observables have one declared operational identity;
- local workers cannot read the requested context from a global answer table;
- no context-dependent fixture supplies the expected sign;
- the noncontextual bound and confidence procedure are frozen.

A violation in software earns an operational-model result only. It does not
establish physical contextuality.

## 7. Bell-type correlations

Implement CHSH or a stronger declared test with:

- independently generated settings;
- sealed setting commitments;
- no message after setting selection;
- no global state read by local measurement workers;
- no setting-dependent deletion;
- no detection filtering;
- complete trial accounting;
- observable no-signalling checks.

The implementation must state which Bell assumption it modifies or rejects.

Software-network violation remains QS evidence because physical locality is not
established by process boundaries. Q0 requires a loophole-aware physical
experiment.

## 8. No cloning

Given an unknown state supplied through the allowed interface, attempt to
produce two independently usable descendants that preserve all later:

- basis statistics;
- interference relations;
- contextual relations;
- entanglement relations where applicable.

Failure due to permission checks, omitted API methods, secret handles, or an
explicit “clone forbidden” branch is invalid. The impossibility must follow
from the transformation/state structure.

## 9. Model B — distributed topological mesh

Model B adds:

- encoded logical sectors;
- local error operations;
- syndrome or record extraction;
- topology-preserving logical transformations;
- recursive scheduler cells;
- authenticated authority;
- crash/replay;
- partitions;
- cross-host reconciliation;
- explicit classical and simulator backends.

The same workload runs against:

1. conventional classical baseline;
2. topology-destroyed mesh;
3. communication-unrestricted positive control;
4. exact reference simulator;
5. tensor/network simulator where applicable;
6. real QPU backend when available.

## 10. Resource ledger

Every trial records:

```text
logical problem size
state bytes
peak memory
CPU/GPU/QPU time
messages and bytes
synchronization rounds
topology-maintenance cost
verification cost
postprocessing cost
discarded trials
hidden coordinator access
```

An exponentially large state or communication trace is reported as such. A
faithful simulation is valuable but cannot earn Q1.

## 11. Negative-control suite

Permanent controls deliberately implement:

- Born-target sampling;
- context answer tables;
- postselected CHSH violation;
- setting communication;
- shared precomputed schedules;
- global-state local reads;
- hidden central reconstruction;
- exponential amplitude vectors excluded from reported cost;
- API-blocked cloning;
- hard-coded visibility decay;
- relaxed thresholds after failure.

The gate passes only when the evidence compiler identifies each control by its
declared failure code.

## 12. Machine-readable verdict

```json
{
  "schema": "cdc-quantum-claim-v1",
  "model": "A|B",
  "substrate": "classical|simulator|qpu|physical-experiment",
  "claim": "C2|C3|C4|QS0|QS1|QH0|R0|Q0|Q1",
  "witnesses": [],
  "controls": [],
  "resources": {},
  "replay_digest": "",
  "verdict": "PASS|HOLD|REJECT",
  "promotion_refused": []
}
```

Unknown fields, missing controls, or an incompatible substrate/claim pair fail
closed.

## 13. Gates

| Gate | Required result |
|---|---|
| QS0 | ABI and one reference simulator execute typed circuits/measurements |
| QS1 | independent simulators/oracles agree across mutation and fuzz suites |
| QH0 | authenticated real-hardware job/result/calibration receipt |
| R0 | unique closure measure derived from frozen axioms |
| Q0 | physical nonclassical witness with loophole-aware controls |
| Q1 | accepted task advantage including complete resource ledger |

No software-only result may emit Q0 or Q1.

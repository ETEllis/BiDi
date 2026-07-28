# CDC / Memory Manifold Interface Contract

**interface-version:** 1.0.1
**status:** binding cross-repository contract (digest-pinned per release)
**change protocol:** single-writer — the BiDi/CDC lane publishes new versions;
the Memory Manifold and Superposition lanes consume by committed digest.
Neither side edits a published version concurrently. A new version is a new
digest, recorded in both repositories' `docs/build/BUILD_STATE.md`.

This document is the machine-checkable boundary required by Phase A of the
CDC Toolchain + Memory Manifold Full-Build Amendment (2026-07-22). The same
content, at the same digest, must exist in the Memory Manifold repository
before its Phase E work begins.

## 1. Version registry

| surface | current | next planned | owner |
|---|---|---|---|
| interface-version | 1.0.1 | — | BiDi/CDC repo |
| grammar-version | 1 (canonical lexer/parser/AST frontend, `runtime/cdc_lexer.c` + `cdc_parser.c`; acceptance byte-compatible with grammar 0, proven by the CT1 differential; grammar 0 remains the oracle until its recorded removal gate) | extensions only by version bump | BiDi/CDC repo |
| abi-version | 1.2 (`runtime/cdc_abi.h`: parse, diagnostics, canonical bytes, result serialization, registry load, contract verify at bootloader parity, statement introspection; execute declared, fails closed with CDC_ERR_STATE) | 1.3 (fused execution surface, Phase C completion) | BiDi/CDC repo |
| evidence-format-version | 1 (this document §6) | — | BiDi/CDC repo |
| mm-schema-version | 0 (names reserved, §4; no implementation) | 1 (Phase E, in the Memory Manifold repo) | Memory Manifold repo |

## 2. Capability identifier partition

Uniqueness is absolute; identifiers are never silently reused.

| range | assignment | status |
|---|---|---|
| A–F series | calculus properties | live (`system.cdc`) |
| G1–G8 | operational runtimes | live (`system.cdc`) |
| G9 | unified-toolchain-driver | reserved (toolchain plan) |
| H1–H5 | task frameworks: transition, procedural, episodic, deliberative, loop | live |
| H6–H11 | toolchain frameworks: manifest, runner, test, install, build, package-runner | reserved (toolchain plan) |
| H12–H17 | Memory Manifold: substrate, traversal, curvature, re-embedding, consolidation, service boundary | reserved (amendment §4.1) |
| U1 | universal operator | live |

## 3. Semantic contract (normative, implementation-neutral)

The manifold and its operations:

```text
M = (V, E, g, Φ, H, L, Θ, I)

read(M_t, q, frame) -> (readout, path, evidence, tension, proposal)
decide(proposal, evidence, policy, I) -> COMMIT | HOLD | NEST
apply(M_t, decision) -> (M_t+1, closure_witness)
```

Non-weakenable rules (Amendment §5, A7, A10):

1. A read may alter ephemeral activation `H`; it must not silently alter
   durable `V, E, g, Φ, L, Θ`. Durable curvature changes are proposals until
   BiDi adjudicates them.
2. All durable mutation and external effects sit behind the BiDi commit path;
   no helper bypasses the guard because it is "internal".
3. The ledger `L` is append-only; geometry may change access, never history.
   Contradictions remain addressable; resolution adds interpretation, never
   deletes evidence.
4. Balanced-ternary results are typed, never merged: `+1` commit, `0`
   hold/nest/non-projecting, `-1` fail. Gate policy treats an unexpected `0`
   as gate failure while reporting commit/hold/nest/fail counts separately.
5. Relational zero: externally identical outputs may carry different internal
   closure state; conformance tests compare closure state, not only output.

## 4. Reserved core data contracts (typed, versioned at mm-schema-version 1)

```text
MemoryPoint Relation MetricState PotentialState ActivationState
EvidenceRecord ProvenanceRecord PerspectiveScope AuthorityScope
IdentityInvariant QueryFrame TraversalPath Readout TensionSet
CurvatureProposal CommitDecision ClosureWitness EssentialForm
ReembeddingResult ConsolidationPlan ConsolidationResult BackendCapability
```

Every durable record carries: stable identity + schema version;
source/provenance identity; event time and commit order; perspective and
privacy scope; parent/version lineage; content digest; proposing and
committing authority; closure witness or explicit unresolved status.

Identity discipline (Amendment A3, MM0): canonical event/content identity is
computed over canonical event content and identity-bearing metadata only.
Embeddings, heat, amplitude, access counts, and derived indexes are derived
artifacts keyed by `(content_digest, model_id, model_digest, dimensionality)`
and never participate in canonical identity.

## 5. Store and execution protocols

`cdc_store` protocol verbs (Phase D, generic; local reference backend first):

```text
append replay snapshot rebuild transaction compare-and-set/fence
commit rollback recover compact attest verify
```

Backend substitution rule: the local durable native backend is complete and
authoritative alone. A distributed backend (Superposition) may change
placement, redundancy, reconstruction, and execution locality; it may not
change the semantic meaning of a memory, a commit, provenance, or closure.

Adapter contract (consumed in Phase K; conformance harness owned by the
integration lane):

```text
Memory event / snapshot bytes
  -> canonical Memory envelope and BLAKE3 identity
  -> encrypt-before-code durable put (HFS)
  -> durable placement / manifest / reconstruction receipts
  -> Memory transaction records receipt digests

Memory traversal / consolidation job
  -> deterministic CDC continuation identity + declared capabilities
  -> sealed worker execution
  -> typed observations and candidate result
  -> Memory/CDC BiDi decision
  -> consensus fence before any externally visible durable effect
```

Backend identities, leases, placement, and storage receipts may satisfy
transport/backend capabilities; they never replace Memory Manifold's
provenance, perspective, evidence, or semantic closure records.

## 6. Digest and evidence discipline

- Canonical content digest: **BLAKE3** for package sources, lockfiles, built
  artifacts, evidence records, and provenance manifests. **Ed25519** signs
  release/package/replica/authority statements over those digests. Any
  mismatch fails closed.
- Interim (until BLAKE3 is vendored in Phase B/D): provenance records carry
  git object ids plus SHA-256, explicitly labeled interim; see
  `docs/build/DECISIONS.md` D2.
- The 12-bit bridge coordinate remains a semantic coordinate and regression
  witness. It is not integrity.
- Evidence bundles live under `evidence/gates/<gate-id>/` and are referenced
  by digest from `docs/build/BUILD_STATE.md`. Verdicts (test, proof, parity)
  bind: source digest, dependency graph digest, toolchain identity,
  grammar/ABI version, artifact digest, and verdict digests.

## 7. Parity vector format (Amendment A5, gate CT2/CT3)

Runtime/oracle parity compares ordered per-check vectors, never aggregate
counts alone. One record per check:

```text
check identifier
decision: commit | hold | nest | fail
coordinate
effects digest
trace digest
closure witness digest
```

The native runtime and bootstrap/oracle path must agree field-for-field.

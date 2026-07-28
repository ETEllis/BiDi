# Resume here - BiDi/CDC

- **Updated:** 2026-07-28
- **Canonical branch:** `main`
- **Canonical head:** `79a508aa1441877aaeeafe415ef285983b186388`
- **Package / ABI:** `0.2.4` / `1.4`

This is the current continuation pointer. Detailed construction history remains
in [`BUILD_STATE.md`](BUILD_STATE.md) and the append-only
[`DECISIONS.md`](DECISIONS.md).

## Current position

PRs #3-#8 are merged. The repository is green through:

- canonical Grammar-1 frontend and stable ABI;
- native run/test execution and deletion gates;
- durable `cdc_store`, persistence language forms, typed receipts, parity
  vectors, lifecycle contracts, and provenance;
- deterministic `cdc build`, crash-safe `cdc install`, and trusted-local
  `cdc x`;
- frozen bootloader Option A;
- the seven-witness RFTC classical crucible;
- RFTC six-form language ingress;
- ABI 1.4 authenticated local authority/transport supervisor.

The current architecture map is
[`../architecture/CDC_SYSTEM_FLOW.md`](../architecture/CDC_SYSTEM_FLOW.md).

## Verify before changing anything

```bash
git status --short
git rev-parse HEAD
./scripts/verify.sh --require-formal
```

Expected starting head is `79a508a`. A documentation branch may be newer, but
it must name its base and carry a clean full gate before merge.

## Next implementation sequence

1. **RFTC bounded dynamic state**
   - execute `frame`, `reduce`, `complex`, and `topology` as state transitions;
   - retain deterministic replay and typed HOLD behavior;
   - add mutants for aliasing, stale topology, frame substitution, and hidden
     unguarded mutation.
2. **RFTC scheduler and reconciliation**
   - integrate authority/transport with recursive logical cells;
   - add partition, retry, causal-gap, duplicate, and cross-host replay gates.
3. **CT5**
   - sealed capability environment;
   - hostile-package corpus;
   - only then reconsider the trusted-local boundary of `cdc x`.
4. **Identity and anchoring**
   - Ed25519 identities and key rotation;
   - external generation anchor;
   - downgrade and rollback counterexamples.
5. **Package language and proof expansion**
   - `package.cdc`, versions, lockfile;
   - `cycles=N` expectation semantics;
   - broader fuzzing and continuous-flow proofs.

Every phase must add its counterexample before its success path and must leave
`./scripts/verify.sh --require-formal` green.

## Cross-repository work

### Memory Manifold

The current CDC/Memory Manifold interface is version 1.3.0. Memory Manifold may
consume `cdc_store` and the decision semantics, but its integration is not
accepted until ordinary callers cannot mint or bypass commit authority and the
decision/effect/witness chain replays from one sealed transaction.

### Superposition

Superposition pins native BiDi parity for guarded collapse. CDC does not own its
network, placement, or device field. PC6 remains hard-paused and this repository
must not start it.

## Standing boundaries

- `cdc x` is trusted-local, not a sandbox.
- keyed-BLAKE3 RFTC transport is shared-key local authentication, not public-key
  identity or a deployed network.
- finite carrier proofs do not imply the continuous theorem.
- the RFTC lane is classical and does not imply quantum behavior.
- `cdc_boot.py` remains frozen as a CI-only differential oracle until Edward
  makes a new explicit decision.

## Handoff format

Any future baton must return:

- exact base and head SHAs;
- changed contract and new capability IDs;
- permanent counterexample map;
- full native/formal/paper and macOS CI results;
- explicitly unchanged limitations;
- confirmation that Memory Manifold authority work and Superposition PC6 were
  not silently promoted.

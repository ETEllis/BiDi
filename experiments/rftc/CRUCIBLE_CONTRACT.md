# RFTC Crucible Contract

## Purpose

The Reference-Frame Topological Coherence (RFTC) crucible tests the missing
executable layer between BiDi's continuous phase flow and its guarded discrete
commit. The proposed layer reduces many phase-bearing nodes to a logical cell
with a stable boundary observation while retaining evidence that multiple
distinct microstates can occupy the same macroscopic class.

This is a falsification contract, not a promotional claim. The executable is
classified as **classical distributed field computation**. A passing verdict
does not establish quantum superposition, entanglement, Bell nonlocality, or
quantum computational advantage.

## The five independent witnesses

| Witness | Positive condition | Held negative control | Meaning |
|---|---|---|---|
| Synchronization onset | A nine-point coupling sweep crosses `R = 0.80` before maximum coupling, remains below it at the preceding point, and has no regression above 0.03; endpoint separation remains `> 0.50` | Zero-coupling ensemble and the subcritical sweep point | A finite critical-coupling interval separates weak collective order from actionable order |
| Topological sector | Winding-1 sector survives moderate local noise in at least 99% of runs | Destroy adjacency by shuffling node order; force a phase slip | Protection belongs to the oriented boundary, not merely the node values |
| Hidden granularity | Distinct BLAKE3 microstate digests share the same `R`, `Psi`, and winding in 100% of runs | Digest equality would fail the witness | Coarse observation is many-to-one without erasing microstate provenance |
| BiDi recovery | Bottom-up reduction plus top-down constraint improves recovery by more than 30% | Local-neighbor recovery only | Reciprocal macro/micro control is measurably stronger than one-way local flow |
| Causal cut | Local deterministic CHSH witness remains `S <= 2` | A communicating implementation reaches `S = 4` but is invalidated | Ordinary cluster communication cannot be laundered into a nonclassical claim |

All five witnesses must pass. No average score can hide one failed mechanism.

## Reproducibility contract

`scripts/verify_rftc.sh`:

1. builds the C99 executable against BiDi's vendored BLAKE3 implementation;
2. requires byte-identical JSON and CSV from identical runs;
3. repeats the search under an alternate seed and requires at least one
   experiment metric—not merely the seed label—to change;
4. runs the entire smoke profile under AddressSanitizer and
   UndefinedBehaviorSanitizer;
5. validates the evidence schema, five PASS results, and explicit claim
   exclusions.

Profiles are intentionally nested:

- `smoke`: 24 seeds; developer gate;
- `rapid`: 256 seeds; next-hours confidence run;
- `stress`: 4,096 seeds; pre-integration statistical pressure test.

## Decision rule

- **PASS_FOUNDATIONAL_CLASSICAL_MECHANISM** means the logical-cell reducer has
  enough evidence to enter the integrated six-form implementation lane.
- **FAIL_COUNTEREXAMPLE_FOUND** blocks that lane. The failing witness is
  investigated or the mechanism is revised; thresholds are never relaxed
  merely to turn the report green.

The full system remains blocked from a quantum claim unless it is connected to
a genuine nonclassical physical substrate and passes an independently designed,
loophole-aware witness. That work is a separate experimental lane.

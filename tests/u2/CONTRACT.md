# U2 adversarial acceptance contract

This suite accepts U2 only when the derivative follows the **executed CDC
program**, recurrence is independently verified, hybrid events include their
timing sensitivity, and held analyses emit no spectrum.  It deliberately does
not infer physical meaning from a numerical multiplier.

## Callable boundary

The native integration gate invokes:

```text
build/cdc_native_runtime stability tests/fixtures/u2/<case>.cdc
```

Each `spectrum` job must emit one deterministic line beginning `u2-json=`.  The
payload schema is `cdc.u2.stability.v1` and contains stage-authorized fields:

- `orbit`, `status`, and typed `reason`;
- a digest of the exact grammar-1 canonical statement stream captured before
  the parsed AST is released (never a later filesystem re-read);
- recurrence `kind` (`full` or `relative`) plus `scope` (`full`, `projected`,
  or `relative`), weighted
  residual, absolute/relative tolerances, verified flag, and—when relative—the
  executable endpoint restoration action, coordinate and displacement, the
  complete row-major restoration derivative plus digest, and explicit
  equivariance/section witnesses tied to the admitted U1 return; recurrence
  also binds whether selected-cell discrete latch identity was verified;
- selected-cell discrete mode identities for the admitted executable closure,
  with explicit per-cell latch/mode entries plus deterministic initial/final
  digests, once endpoint/tangent state exists;
- tangent dimension and coordinate manifest once the executable closure is
  admitted strongly enough to bind layout;
- event order, event type, localized time, and transversality status once a
  tangent artifact exists;
- granular method detail for executable flow/commit/nest behavior and the
  current guard-localization boundary;
- explicit `null`/not-applicable placeholders for whole-path finite-difference
  receipts, localized event budgets, and neutral-mode removal when v1 did not
  execute them;
- row-major monodromy matrix;
- deterministically ordered multipliers with real, imaginary, modulus, and
  `physical`/`gauge` mode classification, plus spectral radius and validated
  real-Schur residual diagnostics when a spectrum is accepted;
- overall `stable`, `marginal`, `unstable`, or `held` classification.

For a U1 pre-snapshot hold, all U2 state/layout/tangent/recurrence/monodromy
artifacts must be absent or null. For an accepted-U1 pre-tangent source-bound
hold, manifest/dimension/coordinates may already be bound while endpoint,
discrete-state, tangent, recurrence, monodromy, and spectral artifacts remain
absent or null. For recurrence-stage holds, monodromy and multipliers must be
absent or null. For downstream spectral-stage holds after recurrence already
authorized the return operator, monodromy may remain bound while multipliers,
backend, and spectral diagnostics stay absent or null. Human diagnostic lines
may accompany the canonical record, but tests never infer a pass from prose.

## Executable derivative semantics

- `flow` differentiates the runtime's explicit-Euler phase update, not an
  idealized continuous equation.
- A fixed-mode `commit` has the identity derivative on continuous coordinates;
  its latch is a discrete mode transition.
- `nest` differentiates the implemented transfer: `up` is the discrete
  `module_mean_trit(child)`, parent belief receives `gain * up`, and child prior
  is assigned the new parent belief.  It must not invent a differentiable
  child-belief/prior coupling absent from primal execution.
- A fixed-time reset uses its reset Jacobian.  A state-triggered reset uses the
  full saltation matrix, including event-time sensitivity.
- Products act on column perturbations in execution order: the later segment
  left-multiplies the accumulated product.
- An include/exclude mask proves only a projected return.  Relative monodromy
  additionally requires an executable endpoint restoration `rho`, a derivative
  for `rho`, and an equivariance/section witness tying the final state back to
  the initial tangent space.  Without all three, analysis holds before spectrum.
- Relative monodromy is constructed as `M_rel = D(rho) * D(U)`.  Merely binding
  `D(rho)` in metadata is insufficient; the derivative must appear as the final
  ordered event and alter the returned matrix when it is non-identity.
- A lifted `cover-cell` is unwrapped.  Unlike an ordinary phase coordinate it
  cannot pass absolute recurrence merely because its change is `2*pi` or
  `4*pi`; that requires an explicit relative restoration.
- Recurrence uses positive finite per-coordinate weights and explicit absolute
  plus relative tolerance.  Invalid weights or tolerances hold before analysis.
- In the current `cdc.u2.stability.v1` runtime, machine consumers can
  independently recompute every nonprojected earned recurrence claim from the
  emitted `initialState`/`finalState`, coordinate `period` values
  (shortest-circle for declared periodic coordinates, unwrapped difference for
  lifted coordinates), implicit unit weights, absolute-plus-relative
  tolerances, selected-cell discrete-state verification, and the admitted
  relative restoration artifact when present. The recomputed raw/normalized
  residual and the resulting verified/authorization decision must match the
  emitted recurrence fields.
- Projected receipts do not expose the current v1 include-mask that produced
  the projection. The core can verify its hidden-mask diagnostic, but CDC
  Studio cannot independently recompute that bit and admits only projected
  receipts whose producer diagnostic is internally consistent and false.
  Consumer-admitted projected receipts are held, consumer-unverified,
  non-authorizing, and may not expose monodromy or spectrum. A non-executable
  relative receipt with no admitted restoration, no verified restoration
  equivariance, or no bound restoration derivative is held and
  non-authoritative for the same reason.
- Authorization binds to the exact restoration derivative matrix. CDC Studio
  requires the current v1 identity matrix and canonical BLAKE3 digest syntax,
  but does not independently recompute `derivativeDigest`; the digest remains
  receipt provenance metadata rather than an authorization input. The bounded
  web fixture additionally pins its known 13-by-13 identity digest.
- A numerical multiplier near `+1` is not evidence that it is gauge.  Gauge
  removal requires alignment with a declared symmetry generator and the same
  section/restoration semantics used for relative recurrence.

## Required cases

1. Known smooth stable, unstable, and neutral multipliers.
2. Analytic `D U` versus adaptive central finite differences.
3. Two non-commuting events, detecting reversed multiplication order.
4. Reset Jacobian versus saltation matrix, with unequal expected results.
5. Grazing/non-transverse event: typed hold and no spectrum.
6. False full-state recurrence, projection-only drift (which must not authorize
   a spectrum), and relative recurrence with an executable restoration map and
   applied derivative.
7. A neutral multiplier retained as physical unless its declared symmetry
   generator aligns with the section/restoration semantics; an unverified
   `+1` mode must never be silently removed as gauge.
8. Byte-identical canonical output across repeated executions.
9. Runtime derivative of actual nest semantics, rejecting an idealized
   derivative for dynamics the primal runtime does not execute.
10. Nondecreasing event time, allowing explicit equal-time ordering but
    rejecting a backwards itinerary without mutating accumulated state.
11. Ordinary periodic phase versus unwrapped lifted-cover topology.
12. Analysis purity: `stability` cannot perform the declared `evolve` effect.
13. A scoped polarity-covariance witness that separately gates involution,
    fixed aperture, primal conjugacy, and U2 tangent conjugacy; a failed witness
    must not invalidate the underlying CDC realization or become a global law.
14. An unrelated declaration island must not change the selected Universal's
    manifest dimension, coordinate order, recurrence, or multiplier spectrum.
15. A U1 hold that occurs before snapshot admission; U2 must preserve the U1
    reason and emit no fabricated manifest, recurrence, state, tangent,
    monodromy, or spectrum.

## Permanent mutant gate

The verifier recompiles or invokes the identical implementation with eight
test-only mutation families: reversed product order, omitted saltation, omitted
endpoint-restoration derivative, invented theoretical nest derivative,
acceptance of a nonrecurrent/projected-only path, ignored polarity aperture,
skipped polarity tangent conjugacy, and automatic false polarity acceptance. Each
mutant must be rejected by at least one named fixture.  A mutant surviving is
a failing U2 gate, even when the unmutated implementation passes.

## Formal claim ceiling

`formal/{lean,coq}/U2VariationalFinite.*` proves only identity, associative
composition, observable order for two finite non-commuting maps, and distinct
finite carrier/cone/sheet involutions. It also proves the concrete `+-` / `-+`
prefix-barrier counterexample, so naive sign inversion cannot be promoted to a
global safety symmetry. Runtime Jacobians, recurrence, spectra, covariance
residuals, and physical interpretations remain executable/tested claims, not
theorem-proved claims.

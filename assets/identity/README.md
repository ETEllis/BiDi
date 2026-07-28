# Möbi𝒰s Identity Assets

These are non-destructive identity-system candidates derived from the executed
Universal Operator and the existing embodied Möbius product mark. They do not
rename the package, the `.cdc` source format, or the formal paper.

- `mobius-embodied-mark.svg` — closed, present, eye-bearing product body.
- `mobius-u-wordmark-dark.svg` — dark-surface wordmark and phase ground.
- `mobius-u-wordmark-light.svg` — light-surface wordmark.
- `mobius-u-code-sigil.svg` — monochrome canonical Universal Operator `𝒰_` for
  small technical surfaces, with a right-biased live-horizon overhang.
- `mobius-u-code-sigil-dark.svg` — two-ink dark-surface `𝒰_` with the same
  executable horizon.
- `mobius-u-operator.svg` — reduced mathematical body `𝒰` without the live
  horizon. The filename remains a compatibility handle, not a second operator.
- `mobius-ius-relational.svg` — independent `i𝒰s` relational lockup.
- `mobius-bi-seed.svg` — the `bi` source extracted from the name.
- `mobius-bidi-kernel.svg` — portable fallback for the generated `bidi` kernel
  word; it is never imported into the authoritative Blender derivation.
- `mobius-bidi-delta.svg` — portable fallback for the generated BiDi-Delta
  sub-brand lockup.
- `renders/mobius-wordmark-static.png` — canonical embodied still: the connected
  ribbon and literal eyes occupy the `ö` slot inside the type.

The word skeleton begins from the original Möbius landing-page family, Syne,
then converts the visible characters to portable outlines. The `𝒰`, eyes,
phase trajectory, spacing, and return line are engineered identity geometry.
The dark-surface hierarchy is cobalt-indigo `Möb`, phase-indigo `i`, a white
operator core with electric-blue keyline, and relational cyan `s`; white is a
compression event, not the default alphabet color.
The `M`, `b`, and continuous double-curve `s` outlines preserve true open
counters after Blender triangulation. During generation, `𝒰` projects from the
conserved aperture and `s` unfurls from its operator-facing contact pivot;
their endpoint resolves at a controlled tangent as one relational field. The
M-derived Delta edge is seeded on the letter's actual inner diagonal before it
peels outward.
The operator skeleton begins from the STIX Two Math script `𝒰` so it remains
mathematically recognizable, then receives the identity keyline, scale, and
optical placement. No live font is loaded by the assets.

The implementation contract and reconciled handoff package live in
`docs/identity/`; start at `docs/identity/README.md`. The interactive motion
study lives at `demo/mobius-identity.html`.

## Spatial production master

`3d/mobius-identity-master.blend` is generated from the versioned Blender
scripts in `tools/blender/`. It owns one continuous Möbius topology and the
`ö`, `𝒰`, and secondary topological `Δ` projection states. The glyph rig adds
the full wordmark, `i𝒰s`, structural `UI → 의`, literal
`BI + reflected DI → BIDI`, three-stroke `Δ`, reduced body `𝒰`, and the complete
Universal Operator sigil `𝒰_`.
`3d/identity-manifest.json` maps the master and the independently consumable
GLBs for product, web, code, and kernel use. The selected transparent key-state
renders are in `renders/`; the encoded 25-second master and six independent
sequences are in `motion/`.

Build and verify the full package with:

```bash
./scripts/release_identity_3d.sh
```

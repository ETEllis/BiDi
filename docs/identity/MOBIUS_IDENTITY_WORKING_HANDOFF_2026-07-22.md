# Möbi𝒰s Working Identity Handoff — 2026-07-22

Branch: `codex/mobius-u-identity-system`  
Starting commit for this pass: `29cefff`  
Release status: working candidate; topology and current causal animations are
validated; final Pencil-guided optical freeze and reverse-sheet sequence remain
open.

## Start here

1. Read `MOBIUS_U_RECONCILED_SOURCE_OF_TRUTH.md`.
2. Inspect `assets/identity/3d/identity-manifest.json`.
3. Open `demo/mobius-identity.html`.
4. Use `MOBIUS_U_VISUAL_REFINEMENT_WORKING_PAPER.md` when applying Edward's
   markup.
5. Run `./scripts/release_identity_3d.sh` after any geometry or motion change.

## Documentation package

| Document | Consumer | Purpose |
| --- | --- | --- |
| `MOBIUS_U_RECONCILED_SOURCE_OF_TRUTH.md` | everyone | reconciled conceptual and epistemic canon |
| `ETELLIS_XYZ_IDENTITY_INTEGRATION_DRAFT.md` | E.T. Ellis site lane | public-instrument placement and deployment plan |
| `MOBIUS_U_VISUAL_REFINEMENT_WORKING_PAPER.md` | design/Blender lane | remaining optical and motion craft contract |
| `MOBIUS_U_LANGUAGE_AND_REPOSITORY_INTEGRATION.md` | calculus/repository lane | canonical `𝒰_`, reduced `𝒰`, naming, `.cdc`/`.u`, CLI, and migration |
| `MOBIUS_INTERNAL_SYSTEM_FOLD_MAP.md` | internal architecture lane | product/organ/Universal Operator reconciliation |
| existing geometry/motion/integration specs | implementers | exact coordinates, frame ranges, and consumer contract |

## Canonical asset roots

```text
assets/identity/
├── *.svg                 portable static projections
├── 3d/
│   ├── identity-manifest.json
│   ├── mobius-identity-master.blend
│   ├── mobius-identity-master.glb
│   └── independent static/animated GLBs
├── motion/
│   ├── mobius-identity-master.mp4
│   └── six independent semantic clips
└── renders/
    ├── mobius-wordmark-static.png
    └── named semantic witnesses
```

The handoff package does not duplicate binaries. The manifest is the portable
index; its paths and hashes bind the working release together.

## Portable SVG family

- `mobius-embodied-mark.svg` — closed eye-bearing body.
- `mobius-u-wordmark-dark.svg` — dark static wordmark and phase ground.
- `mobius-u-wordmark-light.svg` — light static wordmark.
- `mobius-ius-relational.svg` — terminal relational triad.
- `mobius-bi-seed.svg` — source `BI` extracted from the parent.
- `mobius-bidi-kernel.svg` — portable BIDI fallback.
- `mobius-bidi-delta.svg` — portable BIDIΔ fallback.
- `mobius-u-operator.svg` — reduced mathematical operator body `𝒰`.
- `mobius-u-code-sigil.svg` — small monochrome canonical Universal Operator
  sigil `𝒰_`.
- `mobius-u-code-sigil-dark.svg` — dark-surface keyed canonical `𝒰_`.

## Spatial and motion family

| Semantic role | GLB | MP4 |
| --- | --- | --- |
| full system | `mobius-identity-master.glb` | `mobius-identity-master.mp4` |
| wordmark generation | `mobius-wordmark-animated.glb` | `mobius-wordmark.mp4` |
| `i𝒰s` relation | `mobius-ius-phase-animated.glb` | `mobius-ius.mp4` |
| `UI → 의` | `mobius-ui-hangul-animated.glb` | `mobius-ui-hangul.mp4` |
| `BI → BIDI → Δ` | `mobius-bidi-delta-animated.glb` | `mobius-bidi-delta.mp4` |
| reduced operative body `𝒰` | `mobius-u-animated.glb` | `mobius-operator-u.mp4` |
| complete Universal Operator sigil `𝒰_` | `mobius-u-code-animated.glb` | `mobius-code-sigil.mp4` |

## Implemented in this working pass

- restored a continuous conventional `s` with open upper/lower counters;
- tightened the static `𝒰s` tangent while preserving distinct apertures;
- moved the `s` animation pivot to the operator-facing contact edge;
- made the wordmark `𝒰` project from the conserved `ö` aperture;
- made `s` unfurl only after the operator establishes the hinge;
- tightened the structural Hangul vertical into one compact block;
- retained source-vacating `BI`, reflected `DI`, lineaged Delta strokes, and
  post-closure parent restoration;
- retained the right-biased `𝒰_` cursor overhang standalone and inside Delta;
- restored the accidentally suffixed `mobius-bi-seed.svg` filename after
  confirming the content hash exactly matched the tracked source.

## Working, not yet frozen

- Edward's Pencil adjustments to `M` exterior curves, `𝒰` terminals/weight,
  `s` waist, and the two phase-ground lines;
- final optical choice for static `𝒰s` overlap;
- Korean recognition timing at 200/320/450 ms;
- reverse-sheet `SUI` animation;
- optional respectful `OM` eye-lift state;
- public acceptance of `Möbi𝒰s` as language/runtime name;
- `.u` dual-parser implementation;
- native export of the product's exact RealityKit/Swift body centerline.

## Explicitly preserved user-owned artifact

`assets/identity/motion/mobius-identity-master copy.mp4` is an untracked user
copy. It is not part of the canonical manifest, is not deleted, and must not be
silently staged into the release.

## Build and validation

Use external scratch space on the attached drive to avoid internal-SSD pressure:

```bash
export TMPDIR="/Volumes/ET External/02_BUILD_CACHE/MobiusIdentity/tmp"
./scripts/release_identity_3d.sh
```

The release performs:

1. deterministic Blender scene rebuild;
2. GLB exports and named semantic renders;
3. 600-frame master render and seven MP4 encodes;
4. topology, lineage, counter, palette, frame, duration, and hash validation;
5. full calculus verification;
6. whitespace/diff validation.

Expected high-level witnesses:

```text
MOBIUS_IDENTITY_BUILD_PASS
MOBIUS_IDENTITY_MOTION_PASS
MOBIUS_IDENTITY_VALIDATION_PASS
MOBIUS_IDENTITY_RELEASE_GATE_PASS
MOBIUS_IDENTITY_FULL_RELEASE_PASS
```

## Pencil markup intake

1. Save the markup beside the task, not over a canonical source asset.
2. Record each drawn change as a constraint in the visual working paper.
3. Modify the source SVG/generator, never the committed PNG alone.
4. Rebuild the Blender master from source.
5. compare dark SVG, light SVG, static Blender wordmark, 24-pixel raster, and
   generation frames 63/68/71/75/82;
6. run the full motion release before accepting the mark.

## Next executor instruction

Do not restart the logo. Continue from the current conserved topology and
lineage rig. The next task is optical reconciliation against Edward's markup,
followed by the reverse-sheet sequence as an independent clip. Preserve the
current Korean, BIDI, Delta, connected-`𝒰`, code-sigil, and family-lockup
semantics unless a visual failure is demonstrated in the rebuilt witnesses.

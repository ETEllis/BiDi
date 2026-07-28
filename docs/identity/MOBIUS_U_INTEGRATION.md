# Möbi𝒰s Integration Contract

## One identity package, bounded projections

`assets/identity/3d/identity-manifest.json` is the machine-readable entry point.
All paths inside it are relative to the manifest. Consumers select the smallest
projection that carries the required behavior; they do not reconstruct or
redraw the mark.

The human-facing entry point is
`docs/identity/MOBIUS_U_RECONCILED_SOURCE_OF_TRUTH.md`; the dated pass-off index
is `docs/identity/MOBIUS_IDENTITY_WORKING_HANDOFF_2026-07-22.md`.

| Surface | Primary asset | Fallback | Contract |
|---|---|---|---|
| Favicon, 16–23 px | `mobius-u-code-sigil.svg` | solid monochrome `𝒰_` | preserve the full sigil silhouette; no keyline or motion |
| Terminal, package, file identity | `mobius-u-code-sigil-dark.svg` | monochrome Universal Operator sigil | live horizon is the only horizontal invariant |
| Product/app icon | `mobius-embodied-mark.svg` | `mobius-presence.png` | both literal eyes remain visible |
| Wordmark | `mobius-wordmark-static.png` | dark or light wordmark SVG | the connected collapsed body and literal eyes occupy `ö` |
| Website hero | `mobius-identity-master.mp4` | live SVG study, then presence PNG | honor reduced motion |
| Interactive web 3D | `mobius-identity-master.glb` | MP4 | use manifest frame ranges |
| Native spatial product | `mobius-body.glb` or animated master GLB | embodied SVG | eyes and body share a scene root |
| Reduced operator body | `mobius-u-operator.glb` | reduced-body SVG | notation-minimal `𝒰`; complete identity resolves as `𝒰_` |
| Relational composition | `mobius-ius-phase-animated.glb` | `mobius-ius-relational.svg` | preserve asymmetric i–𝒰–s spacing and operator shear |
| UI / Korean restoration | `mobius-ui-hangul-animated.glb` | named golden frames | `의` is built from body, ground, and i-stem—not pasted type |
| Formal kernel / sub-brand | `mobius-bidi-delta-animated.glb` | `mobius-bidi-delta.svg` | `BI` vacates and projects outward, reflected `DI` crystallizes, then three source strokes close `Δ` and parent `BI` restores |

## State and motion

The 600-frame master runs at 24 fps. The manifest owns the authoritative state
ranges. Product code may jump to a state, scrub continuously, or use the six
published independent sequences:

- frames 1–108: embodied wordmark generation;
- frames 109–156: `i𝒰s` relational phase;
- frames 157–228: `UI → 의` restoration;
- frames 277–420: `BI → BIDI → BIDIΔ` self-extraction;
- frames 421–468: connected operative body `𝒰`;
- frames 469–516: complete executable Universal Operator sigil `𝒰_`.

The bundled master MP4 plays the complete 25-second composition. Each sequence
also has its own MP4 and animated GLB. The GLBs retain editable animation data
for real-time engines.

## Web

The demo uses the MP4 as its primary visual runtime and automatically retains
the live SVG construction if video loading fails. It pauses at the restoration
state for `prefers-reduced-motion` users and exposes a frame-accurate scrubber.

Production websites should preload only metadata, provide the presence PNG as
a poster, and defer the animated GLB until interaction requires spatial control.
Do not make a multi-megabyte 3D file the favicon or first-contentful-paint path.

## Native products

Bundle the manifest and resolve its assets through the application resource
system. RealityKit, SceneKit, game engines, and WebGL runtimes should load the
published GLB rather than the `.blend`; Blender is a production dependency, not
a customer runtime dependency. Preserve the root transform and named objects so
eye attention, body projection, and kernel extraction remain independently
addressable.

## Universal Operator and repository identity

`𝒰_` is the canonical executable Universal Operator sigil across code-language,
terminal, package, and small technical surfaces. Bare `𝒰` is its reduced
mathematical body, not a second identity. The sigil does not silently rename
`.cdc`, packages, formal theorem namespaces, or compatibility surfaces. Those
remain governed by the migration audit; any language-name or extension
migration remains an explicit versioned decision.

## Invariants across every consumer

1. The phase angle is `0.125 rad` (`7.161972°`).
2. The master body remains a single one-sided band with one boundary loop.
3. Eyes are literal product anatomy, never ornamental dots.
4. `ö` and operative `𝒰` are projections of one conserved topology. The body's
   `Δ` remains a secondary witness while the primary `BIDIΔ` mark closes from
   exactly three type-derived strokes.
5. A surface gets at most one horizontal invariant: ground, cursor, fold, or blink.
6. Reduced-size assets simplify appearance without changing semantic ownership.
7. Korean/Chinese resonances emerge in motion and composition; they are not
   substituted for language or presented as etymological claims.
8. The parent word remains visible while its internal `BI` vacates and projects
   the reflected `DI`; parent `BI` restores only after closure, and no prebuilt
   BIDI mark may replace that causal sequence.
9. The M-derived Delta edge must visibly seed on the source glyph's inner
   diagonal before detaching; a free-floating third bar is not conformant.
10. `𝒰_` retains a right-hand cursor overhang both standalone and inside the
    resolved Delta lockup, where it must remain clear of the triangle base.
11. During generation, `𝒰` projects from the conserved aperture and `s`
    unfurls from the operator-facing contact edge; independent glyph arrivals
    are not conformant.

## Release authority

Run `scripts/release_identity_3d.sh`. A consumer-facing artifact is releasable
only after topology validation, GLB/PNG/MP4 checks, manifest hashes, the complete
calculus suite, and whitespace validation all pass. MCP manipulation is useful
for live design work but never replaces the deterministic generator or gate.

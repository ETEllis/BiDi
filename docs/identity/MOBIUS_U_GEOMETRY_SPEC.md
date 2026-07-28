# Möbi𝒰s Source Geometry

## Constants

| invariant | value | role |
| --- | ---: | --- |
| phase angle `φ` | `0.125 rad = 7.161972°` | common bias for seam, `i`, ground, and fold |
| eye ratio | `1.4 ×` typographic `i` dot | separates embodied and subjective ocular registers |
| `𝒰` visual height | approximately `88%` of surrounding lowercase mass | compensates for white fill and blue keyline |
| `𝒰` keyline | `3–4 px` at the 740 × 240 master | survives dark and light surfaces without glow |
| `s` residue | `−2°` conceptual counter-rotation | returns changed rather than mechanically reset |

The `0.125` correspondence is an authored visual inscription of the runtime-
checked holonomy, not a theorem that brand geometry follows from the calculus.
Finite sheet parity is mechanized; continuous holonomy remains a queued proof
obligation.

## Master wordmark

Master view box: `740 × 240`.

The `M`, `ö`, `b`, `i`, and `s` begin from the Syne display family used in the
original Möbius landing page, converted to outlines so the asset has no runtime
font dependency. The operator begins from the STIX Two Math script `𝒰` outline
to preserve mathematical recognition, then receives the identity keyline,
scale, and optical placement. Each glyph is positioned optically rather than by
font metrics.

The phase trajectory is:

| glyph | vertical role | angular role | color role |
| --- | --- | --- | --- |
| `M` | anchor | `0°` | substrate |
| `ö` | slight dip/fold | seam at `φ` in embodied motion | body + paired eyes |
| `b` | lift | nearly upright | bridge |
| `i` | elevated approach | `+φ` forward bias | indigo intent |
| `𝒰` | slight descent | left inherits bias, right resolves | white/blue change |
| `s` | settled baseline | damped `2°` residue | cyan relation |

The counters are structural geometry, not shading. `M` uses one
non-self-intersecting outline with a clean open V, `b` has one continuous
seated base around a single enlarged interior aperture, and `s` is one
continuous conventional double curve with open upper and lower counters. Its
animation pivot is moved to the left contact edge without moving the final
outline, allowing the curve to unfurl from the `𝒰` terminal rather than
materializing at its own center. `𝒰` and `s` meet at one controlled tangent:
complementary pressure fields that remain legible as two projections of the
shared `us` event.

## Conserved aperture

The connected Blender body now supplies the conserved aperture in the
authoritative static wordmark frame. The portable vectors remain bounded
fallbacks, while the native product implementation remains a future source for
even higher-fidelity device behavior:

- eye centers: `(0.445, 0.214)` and `(0.555, 0.214)`;
- eye diameter: `0.0723` of the product mark frame;
- product loop center: approximately `(0.5, 0.53)`;
- product half twist: `π × progress` around the ribbon centerline.

The next product integration should export the Swift `MobiusBrandGeometry`
centerline into the same interchange format used by the web motion study. That
will let `ö` and `𝒰` share the exact product curve rather than merely matched
visual proportions.

## Blender master topology

`assets/identity/3d/mobius-identity-master.blend` is the authoritative spatial
body. `tools/blender/build_mobius_glyph_identity.py` builds the production rig
over the topology foundation in `tools/blender/build_mobius_identity.py`; the
binary is never an unexplained source.

- `2496` vertices, `4800` edges, and `2304` quad faces;
- Euler characteristic `V − E + F = 0`;
- one connected boundary cycle with `384` boundary edges;
- one reversed cross-section seam, witnessing a single half-twist;
- shape keys `Basis`, `UProjection`, and `DeltaProjection` over the identical
  vertex/face topology;
- two separate animated eye objects whose identity survives each projection.

The `UProjection` does not cut the band. The front traversal describes the
visible U while the closed return retraces it at negative depth. Orthographic
occlusion therefore produces `𝒰` while the mesh remains a closed Möbius band.
The connected triangular state remains a secondary topology projection of the
same mesh. The primary `BIDIΔ` identity triangle is separately—and
intentionally—closed by exactly three strokes derived from `M`, `i`, and the
phase ground.

## Glyph derivation geometry

- The portable wordmark imports as independent `M`, `O`, `B`, paired-eye,
  `I`, `U`, `S`, and two ground curves. The SVG `O` and dots are registration
  guides only; the rendered static frame seats the connected body and literal
  3D eyes in their place.
- `BIDI` contains four live descendants: `B/I` first vacates the parent word
  and travels outward as the source pair; a second pair projects from it. The
  copied `B` crosses zero x-scale and resolves at negative x-scale as `D` while
  copied `I` remains invariant. Parent `BI` restores only after Delta closes.
- The structural Hangul state uses the connected body as `ㅇ`, a phase-corrected
  horizontal ground as `ㅡ`, and a separately lineaged, compact i-stem as `ㅣ`.
  Their committed positions are validated as one syllabic block.
- During wordmark generation, the typographic `𝒰` begins at the conserved
  aperture and establishes the hinge before the contact-anchored `s` opens from
  its return terminal. Independent bottom-up arrivals are a validation failure.
- The M-derived Delta edge first highlights the actual right-inner diagonal of
  `M`, then peels, translates, and grows into the first triangle edge. The
  other two edges retain their direct `i` and phase-ground lineages.
- The primary Delta endpoints close within `0.22` Blender units; the current
  three measured gaps are recorded in the validation report.

`scripts/verify_identity_3d.sh` recomputes the topology from the saved Blender
file and validates every committed GLB, PNG, SVG, checksum, semantic state, and
web-size limit.

## Small-size behavior

- At `≥ 64 px`, use the full `𝒰_` keyline rendering.
- At `24–63 px`, simplify the entry aperture and keep one blue edge.
- At `< 24 px`, use the solid monochrome `𝒰_`; do not retain a white interior
  whose keyline cannot survive rasterization.
- Never use glow to recover an edge that geometry has lost.

## Color tokens

| token | dark surface | light surface |
| --- | --- | --- |
| substrate | `#394db8` | `#07152c` |
| subjective indigo | `#707ddd` | `#27316f` |
| operator edge | `#137dff` | `#0c6fd8` |
| operator core | `#f8fbff` | `#ffffff` |
| relational cyan | `#31e2e7` | `#008d9b` |

These are flat identity inks. The embodied ribbon may retain a real surface
gradient because it distinguishes front/back orientation; decorative bloom is
not part of the static wordmark.

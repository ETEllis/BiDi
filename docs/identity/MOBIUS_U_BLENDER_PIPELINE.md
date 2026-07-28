# Möbi𝒰s Blender Production Pipeline

## Authority

The versioned generator is authoritative:

```text
tools/blender/build_mobius_glyph_identity.py
    ↳ tools/blender/build_mobius_identity.py (topology foundation)
    ↓
assets/identity/3d/mobius-identity-master.blend
    ↓
GLB components + transparent render witnesses + identity-manifest.json
```

The `.blend` file is an approved production artifact, not the only source. A
fresh scene can be rebuilt from the Python generator and tested independently.

## Local installation

Blender `5.2.0 LTS` is installed at:

```text
/Volumes/ET External/Applications/Blender.app
```

Homebrew exposes the launcher at `/opt/homebrew/bin/blender`. The application
and download cache live on `ET External`; the repository retains only the
compact master, interchange assets, selected renders, and deterministic code.

## Optional live MCP control

The live bridge is pinned to:

```text
repository  https://github.com/ahujasid/blender-mcp
commit      9ad355a56dfa7598788085f1b0091c010eaebb07
addon sha   331adfb119bcc04f12e562b53b1c576cceb8d7c3aabbbebb9a72c28258c75163
checkout    /Volumes/ET External/01_AGENT_STATE/tooling/blender-mcp
```

Codex's global `blender` MCP entry runs that pinned checkout through
`/opt/homebrew/bin/uv`. The Blender add-on is enabled, its prompt/code/screenshot
telemetry consent is disabled, and the MCP server is bound to localhost port
`9876`. Restart Codex after first installation so a new task can discover the
tool.

The MCP may inspect and manipulate a live scene, but it is not a release
authority. All accepted changes must return to the generator and pass the
deterministic gate.

## Commands

```bash
./scripts/build_identity_3d.sh
./scripts/verify_identity_3d.sh
./scripts/release_identity_3d.sh
```

The glyph generator owns the 600-frame type, restoration, extraction, and
family rig. The topology foundation owns the connected band mesh and shared
projection helpers. The build wrapper requires an explicit completion witness because Blender can
return a zero process status after a Python traceback. The validator reopens
the master and checks topology rather than trusting construction intent.

## Release contract

A release is green only when all of the following hold:

1. The body has Euler characteristic `0`, one boundary component, and one
   reversed seam.
2. `Basis`, `UProjection`, and secondary `DeltaProjection` share the identical mesh.
3. Both literal eyes exist and carry animation.
4. Every GLB has a valid glTF 2 header and remains below `5 MB`.
5. Ten semantic golden frames are distinct `960 × 960` PNGs, and the embodied
   static wordmark is a `1440 × 420` production PNG.
6. The manifest checksums every stable generated artifact.
7. The component family includes animated wordmark, `i𝒰s`, `UI → 의`, literal
   `BI → BIDI → BIDIΔ`, reduced connected body `𝒰`, and canonical Universal
   Operator sigil `𝒰_` projections.
8. `BIDI` is made causally: parent `BI` vacates, source `BI` travels outward,
   projected `BI` crosses the inversion plane, `B` crystallizes at negative-x
   scale as `D`, and the original `BI` restores only after Delta closes. No
   finished kernel SVG enters the master.
9. Delta is exactly three lineaged type strokes whose endpoints close within
   tolerance; the connected body's Delta projection is secondary only.
10. The complete calculus verification suite and `git diff --check` remain green.

## Product integration

Consumers should read `assets/identity/3d/identity-manifest.json` rather than
hard-code filenames. The animated master is intended for web/product runtimes;
the static GLBs and SVGs provide bounded variants for icons, prompts, packages,
documentation, and reduced-motion surfaces. KIMI or another product lane can
integrate this manifest without inheriting Blender as a runtime dependency.

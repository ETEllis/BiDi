# Möbi𝒰s Naming and Compatibility Audit

## Audit baseline and current truth

The audit began at merge `8cfe48f`, with five typed task frameworks, the
continuous two-cycle loop, Universal Operator closure, and formal finite sheet
parity. The identity working branch has since added executable SVG/Blender/GLB/
MP4 projections and reconciliation documents, but the package, source format,
runtimes, and paper still use BiDi/CDC names.

## Rename surface

The human-facing project title appears in nine core files:

- `README.md`
- `BIDI_CALCULUS_CORE.md`
- `BIDI_COHERENCE_DELTA_CALCULUS.md`
- `CITATION.cff`
- `kernel.cdc`
- `paper/README.md`
- `paper/arxiv/main.tex`
- `pyproject.toml`
- `assets/bidi-cdc-icon.svg`

The compatibility surface is much larger: `.cdc`, `cdc_boot.py`, `cdc_*`
runtimes, build commands, generated source names, paper listings, and gate
assertions occur across 45 tracked files.

## Decision

This branch does **not** mass-rename `.cdc`, runtime binaries, package metadata,
or the paper. The identity can ship independently; language migration cannot.

## Staged route

1. **Identity layer** — add source geometry, wordmarks, sigils, motion, and
   explicit hierarchy. Completed on this branch.
2. **Public-name decision** — accept or reject Möbi𝒰s as the repository-facing
   name while retaining the CDC kernel designation.
3. **Alias implementation** — if `.u` is desired, accept both `.cdc` and `.u`
   inputs through one parser and prove byte-for-byte equivalent AST/runtime
   behavior on the complete fixture corpus.
4. **Tool aliases** — add `u`/`mobius` command aliases without deleting
   `cdc_boot` or `cdc_*` compatibility entry points.
5. **Documentation migration** — introduce the hierarchy consistently in the
   README, language manual, paper, citation metadata, and generated output.
6. **Deprecation window** — only after released aliases, test parity, and
   external-consumer inventory may `.cdc` become historical rather than
   canonical.

## Falsifiers

Do not advance the extension migration if any of these fail:

- the same source under `.cdc` and `.u` produces different indexed
  declarations, runtime trace, bridge coordinate, or evolved output;
- the full formal/runtime/paper gate is not green;
- tooling or syntax highlighting treats `.u` as an unrelated file type;
- the name is confused with UI-only source strongly enough to obscure the
  language's general role.

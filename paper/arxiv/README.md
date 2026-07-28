# arXiv Source Bundle

This folder is intentionally flat and dependency-light.

`main.tex` is the current paper projection of the formal kernel **and** the
native implementation through ABI 1.4: canonical frontend, fused execution,
guarded durable closure, package lifecycle, and the bounded RFTC local control
plane. Implementation language remains constrained by the repository's
verification and claim boundaries.

Primary source:

```text
main.tex
```

Compile when a TeX toolchain is available:

```bash
tectonic main.tex
```

or:

```bash
pdflatex main.tex
pdflatex main.tex
```

The source avoids figures, BibTeX, shell escape, minted, and nonstandard
classes. It uses an inline `thebibliography` environment so no `.bib` or `.bbl`
file is required.

The repository authority is:

```bash
./scripts/verify.sh --require-formal
```

from the repository root. That gate compiles the paper together with the native
runtime and formal mirrors, preventing the manuscript from drifting away from
the executable artifact.

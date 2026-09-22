# CDC research preprint

This folder contains the paper and reproducible source for **BiDi Coherence-Delta Calculus: Hybrid State Reduction and Recurrence-Gated Variational Analysis** by Edward Ellis.

- [Paper](main.pdf)
- [Source bundle](arxiv-source.zip)
- [Public introduction](https://etellis.github.io/BiDi-web/)

The complete manuscript source is:

```text
main.tex
```

Compile with:

```bash
tectonic main.tex
```

or:

```bash
pdflatex main.tex
pdflatex main.tex
```

The source requires no external figures, BibTeX, shell escape, minted or custom classes. Its inline bibliography contains each cited work once. The repository's verification tools reproduce the versioned runtime results and finite formal statements described in the paper.

The manuscript concerns the CDC kernel and U1/U2 analysis within the wider BiDi system. It is prepared for arXiv; no submission or peer review is claimed. AI assistance is disclosed in the paper.

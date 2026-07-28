# Paper

`arxiv/main.tex` is the intentionally flattened arXiv-oriented source for the
BiDi Coherence-Delta Calculus paper.

The July 28 revision reconciles the original calculus exposition with the
canonical Grammar-1 frontend, ABI 1.4, guarded `cdc_store`, typed receipts,
build/install/x lifecycle, frozen-oracle decision, and the explicitly classical
RFTC control-plane boundary.

The source is conservative LaTeX and avoids figures, external BibTeX, shell
escape, minted, and custom classes.

The style is intentionally Knuth-adjacent: compact Computer Modern/TeX
presentation, literate-programming structure, and source fragments treated as
part of the exposition rather than decorative examples.

Compile when TeX is available:

```bash
cd paper/arxiv
pdflatex main.tex
pdflatex main.tex
```

Or with Tectonic:

```bash
cd paper/arxiv
tectonic main.tex
```

For arXiv submission, first run the repository-wide required gate:

```bash
./scripts/verify.sh --require-formal
```

Then upload the TeX source from `paper/arxiv/` and select the license
intentionally during arXiv submission.

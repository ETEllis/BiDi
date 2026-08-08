# Paper

[`arxiv/main.tex`](arxiv/main.tex) is the flattened, dependency-light source for
the Coherence-Delta Calculus 0.3.0 paper within the BiDi Universal Operator
System.

The paper stays CDC-first for a reason: it formalizes the language/kernel and the
U1/U2 release argument rather than claiming the complete repository as one
mathematical result. It is reconciled to the executable release boundary:

- exactly three primitive reductions: `flow`, `commit`, and `nest`;
- U1 lifted-cover closure is distinct from complete-state recurrence;
- U2 path tangent is distinct from monodromy and multipliers;
- full or explicit relative recurrence gates every return spectrum;
- the current native-v1 mutations are separated from richer specified
  semantics; and
- finite formal claims, generic polarity covariance, canonical-loop results,
  and physical interpretations retain separate scopes.

The bundle avoids figures, external BibTeX, shell escape, minted, and custom
classes. It uses an inline bibliography and standard LaTeX packages.

Compile with Tectonic:

```bash
cd paper/arxiv
tectonic main.tex
```

Or with pdfLaTeX:

```bash
cd paper/arxiv
pdflatex main.tex
pdflatex main.tex
```

The repository release gate compiles this source and rejects stale public
claims through the surrounding source/receipt checks. For arXiv submission,
upload the contents of `paper/arxiv/` and select the license intentionally.

# BiDi website

The public site is [etellis.github.io/BiDi-web](https://etellis.github.io/BiDi-web/). Its commit decision executes the ordered-prefix rule in [U](model.u), compiled to [WebAssembly](model.wasm). Every running prefix must remain nonnegative. The final sum may be positive.

All 81 four-trit inputs agree with the ordered-prefix specification, including the 35 that pass. The browser checks the [binary receipt](model.wasm.json) and exact input bounds before allowing a decision. Controls stay disabled while loading or after an asset failure. A held commit leaves previous latches intact.

Phase flow and trit quantization remain browser floating-point previews under the displayed equations. The compiled module checks the four resulting trits. This distinction matters: the page does not claim the full native CDC numeric profile, issue U1/U2 receipts or replace the recurrence analysis.

Publish this directory at the site root, keeping `model.u`, `model.wasm` and `model.wasm.json` together. Include `paper/main.pdf`, `paper/arxiv-source.zip` and `assets/banner.svg`. Hosting is static; compilation happens before publication. Keyboard controls, equation details and a no-script explanation remain available. The existing `demo/` console routes are unchanged.

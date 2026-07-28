# Product Surfaces

Two surfaces over one runtime: a native macOS operator app and a
self-contained web console. Neither reimplements the calculus — both present
what the shipped `cdc` binary actually produced.

```text
ui/
├── design/mobius-tokens.json     canonical design tokens (single source of truth)
├── macos/CDCStudio/              SwiftUI operator app (SwiftPM, no dependencies)
└── web/console/index.html        self-contained console, bound to demo/replay.json
```

## The design decisions, and why they are these decisions

**The palette is the identity's colour law, not a mood board.** Cobalt-indigo
substrate → phase indigo → white/electric hinge → cyan resolve, taken from
`docs/identity/MOBIUS_U_IDENTITY_SYSTEM.md` and the shipped SVGs. One warm
token exists (`ember`) and it is reserved exclusively for a violated
invariant — it never appears as emphasis, branding, or an ordinary alert.

**Three states, never two.** The product's result carrier is balanced ternary,
so the interface has three outcomes: accepted (`+1`), held (`0`), violated
(`−1`). A held result is *equilibrium* — informative, admissible, and styled
as the calmest of the three. Every conventional UI would render it as a
yellow warning; doing so here would destroy the distinction the calculus
exists to make. The gate enforces that all three states remain separately
present and separately coloured on every surface.

**7.16197° is the only rotation in the product.** It is the wordmark's shipped
invariant, already gated by the identity asset contract. Every tilt — the app
mark, the console mark — uses exactly that angle.

**The signature visual is the acceptance condition, not an ornament.** The
double cover is drawn as a real Möbius ribbon with a half twist, depth-sorted
so the crossing is legible. One turn returns the projection with the sheet
inverted; only the second restores it. That is precisely what
`execute_universal` checks before it will enact a coordinate, so an operator
watching the band is watching the guarantee.

**Motion performs the second turn.** Animations are 720° restorations, never
spinners, and they yield to `prefers-reduced-motion`.

## macOS — CDC Studio

SwiftUI, SwiftPM, macOS 14+, **zero third-party dependencies**.

```sh
cd ui/macos/CDCStudio
swift build          # or: open Package.swift in Xcode
CDC_REPOSITORY=/path/to/BiDi-Coherence-Delta-Calculus swift run
```

Four workspaces, ordered the way the calculus runs: **Execution**
(flow · commit · nest, with the four counters kept separate), **Universal
Operator** (the double cover and the recorded-equals-decided closure),
**Store** (the three-way failure taxonomy), **Bridge** (the 64-cell codebook).

`ToolchainService` runs the real `build/cdc` binary and parses the runtime's
own report lines, so the app and the verification gate can never disagree
about what happened.

## Web — Möbi𝒰s Console

One file, no build step, no network. The runtime's replay record is embedded
and must be byte-identical to `demo/replay.json`; the gate fails otherwise.
Open `ui/web/console/index.html` directly, or serve the directory.

## Gate

`scripts/verify_ui.sh` (wired into `scripts/verify.sh`) enforces:

- design-token parity across JSON, Swift, and CSS — 15 colours, the identity
  tilt, and the ternary triad, with drift failing the build;
- the console is self-contained (no external `src`/`href`/`@import`), carries
  a language, a title, a described figure, and honours reduced motion;
- the console's embedded record byte-matches the runtime's output;
- the app's source set is complete, dependency-free, placeholder-free, and
  structurally balanced, and it invokes the real binary rather than modelling
  one.

## Honest boundaries

- **The Swift app is not compiled on Linux.** SwiftUI ships only on Apple
  platforms, so a Swift toolchain alone is not sufficient — the GitHub
  Ubuntu runner has `swift` but no SwiftUI, and attempting a build there
  fails on `import SwiftUI` no matter how correct the source is. The gate
  therefore builds CDC Studio only on macOS (`uname -s` = Darwin plus a
  toolchain); everywhere else it enforces structure and token parity and
  states the boundary. Compilation and visual QA happen on a macOS host.
- **The web console is verified by rendering**: it was loaded in Chromium
  with zero page errors and zero external requests. That is a render check,
  not a cross-browser certification.
- **Memory Manifold and Superposition surfaces are specified here but not
  built here.** They belong to their own repositories under the ownership
  split; when those repositories are available, they inherit
  `ui/design/mobius-tokens.json` verbatim so the family stays one system.

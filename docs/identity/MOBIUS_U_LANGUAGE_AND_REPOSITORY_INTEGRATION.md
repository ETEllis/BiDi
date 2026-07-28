# Möbi𝒰s Language and Repository Integration

Status: staged naming and compatibility proposal. This document does not itself
authorize a mass rename.

## One immediate correction

The canonical executable Universal Operator sigil is **`𝒰_`**. It is not a
file extension named `.U_`, and it is not merely a code-brand variant of `𝒰`.

```text
𝒰_  complete executable Universal Operator sigil
𝒰   reduced mathematical body used when the live horizon is omitted
_   live horizon / readiness / cursor projection of the horizontal invariant
```

The candidate future source extension is lowercase **`.u`**. The current
canonical source extension remains **`.cdc`** until the parser accepts both and
the full fixture corpus proves identical behavior.

## Proposed naming stack

| Surface | Name now | Proposed public treatment |
| --- | --- | --- |
| General theory | Universal Operator | keep unchanged and superordinate |
| Directional relation | Bidi-Coherence Delta / `bidiγΔ` | keep unchanged |
| Formal kernel | BiDi Coherence-Delta Calculus | keep as precise kernel name |
| Product | Möbius | keep product proper name |
| Language/runtime identity | BiDi CDC repository | consider **Möbi𝒰s — BiDi Coherence-Delta language** |
| Universal Operator sigil | none | ship canonical `𝒰_` immediately |
| Source extension | `.cdc` | later dual-read `.cdc` + `.u`; no deletion yet |

This allows public compression without erasing the formal ancestry:

> **Möbi𝒰s** is the executable language/runtime identity; **BiDi
> Coherence-Delta Calculus** is its formal kernel.

The wording remains provisional until the project-name decision is accepted.

## Why `.ui` is not the default

`UI` / `i𝒰` is one of the identity's strongest discoveries, but `.ui` is
already broadly read as “user interface.” Making it the source extension would
misclassify a general coherence language as a view-layer format.

The deeper `UI → i𝒰 → 의` relationship survives more powerfully in the glyph,
motion, and documentation than in a suffix that tooling will guess incorrectly.

## Repository-facing copy

If the public-name decision is accepted, the README should lead with:

> # Möbi𝒰s
>
> An executable language and runtime for BiDi Coherence-Delta Calculus. It
> translates differences across bounded frames, evolves continuous state,
> performs guarded commit/hold transitions, nests local and global context, and
> emits inspectable witnesses.

Then state the hierarchy before any identity mythology:

```text
𝒰_         canonical executable turn → cohere → project → return sigil
𝒰 / 𝒰_R   reduced and indexed mathematical notation for the same operator
bidiγΔ     directional cross-frame relation
CDC        formal executable kernel
Möbi𝒰s     public language/runtime identity
```

## CLI and prompt proposal

The implementation should add aliases before renaming existing commands.

```text
cdc_boot source.cdc          # compatibility command, remains supported
mobius run source.cdc        # proposed public alias
u run source.u               # optional compact alias after dual parser lands
𝒰_                           # visual REPL prompt, not a required shell token
```

Do not require users to type a mathematical Unicode character to execute the
tool. `𝒰_` is the canonical visual identity of the operator; ASCII aliases own
shell ergonomics.

## Source extension migration

### Stage 0 — identity only

- ship `𝒰_` assets and documentation;
- retain `.cdc`, current package names, theorem namespaces, binaries, and CI;
- add no silent MIME or editor reassociation.

### Stage 1 — dual parser

- accept `.cdc` and `.u` through the identical loader;
- reject content-based dialect differences;
- add fixtures containing every declaration and capability;
- compare indexed declarations, traces, witnesses, bridge coordinates, evolved
  output, and error locations byte-for-byte where formats permit.

### Stage 2 — tool aliases

- add `mobius` and optional `u` command aliases;
- preserve `cdc_boot.py`, `cdc_*` binaries, and documented old invocations;
- emit the same version and capability report from every alias.

### Stage 3 — editor and content types

- add syntax registration for `.u` without stealing unrelated files;
- publish explicit MIME/UTType identifiers;
- verify GitHub language detection and code search behavior;
- ship formatter/linter parity before calling `.u` canonical.

### Stage 4 — documentation default

- use `.u` in new examples only after Stage 1–3 pass;
- keep paired `.cdc` examples through a declared compatibility window;
- document that CDC is the kernel designation, not an abandoned predecessor.

### Stage 5 — optional canonical switch

Only a major release may declare `.u` the preferred extension. `.cdc` remains
accepted unless an external-consumer inventory and migration release justify a
future deprecation.

## Equivalence contract

For every source fixture `p`:

```text
parse(p.cdc) == parse(p.u)
index(p.cdc) == index(p.u)
execute(p.cdc) == execute(p.u)
trace(p.cdc) == trace(p.u)
witness(p.cdc) == witness(p.u)
```

The aliases must not become two surface dialects accidentally. If syntax or
semantics later diverge, that is a new language-design decision requiring its
own specification and version—not an identity migration.

## Formal and paper names

Lean/Coq namespaces, theorem names, paper claims, and artifact identifiers are
scientific compatibility surfaces. They should not be rewritten merely to
match the wordmark.

Recommended paper phrasing:

> We implement the BiDi Coherence-Delta Calculus (CDC), distributed publicly
> under the working language/runtime identity Möbi𝒰s.

This retains citability even if the public identity evolves again.

## Package and repository options

Ranked from least disruptive to most disruptive:

1. Keep repository slug; change only social preview and subtitle.
2. Add `Möbi𝒰s` to README/CITATION as public identity while retaining slug.
3. Create a stable `mobius-u` redirect/organization landing repository.
4. Rename the canonical repository only after clones, CI, releases, citations,
   package URLs, and external consumers have redirect plans.

The current branch should stop after option 1's identity assets and integration
documentation unless Edward explicitly authorizes the name migration.

## Universal Operator identity deployment

Use the canonical assets rather than typesetting ad hoc approximations:

- `<24 px`: solid monochrome `𝒰_`;
- `24–63 px`: simplified single-edge form;
- `≥64 px`: white core plus electric-blue keyline;
- terminals: blinking underscore at rest, `φ`-tilted fold line while active;
- file icons: static underscore; no blink in OS file lists;
- package badge: include plain-text `U_` alt label for compatibility.

## Language documentation structure

If Möbi𝒰s is adopted, the manual should separate four chapters:

1. **Kernel semantics** — exact grammar and runtime behavior.
2. **Framework layer** — transition, procedural, episodic, deliberative, and
   continuous-loop frameworks.
3. **Universal Operator composition** — how open `bidiγΔ` transport becomes
   guarded closure.
4. **Identity and notation** — why `Möbi𝒰s`, `𝒰_`, `i𝒰s`, `의`, and BIDIΔ are
   used, with evidence-status labels.

Do not mix the symbolic identity story into syntax reference pages where it
would obscure executable meaning.

## Migration falsifiers

Stop the naming/extension migration if any occurs:

- `.u` and `.cdc` produce different semantics or diagnostics;
- editors persistently classify `.u` as an unrelated format;
- the public name makes readers believe Möbius topology is the universal
  mathematical primitive;
- theorem or package redirects break citations and builds;
- the visual sigil requires Unicode input for ordinary CLI use;
- the rebrand outruns a precise one-sentence account of what the language does.

## Recommended decision

Ship `𝒰_` now as the canonical executable Universal Operator sigil across code,
terminal, package, and small technical surfaces. Preserve `.cdc` now. Build
`.u` as a proven alias before debating canonical status. Use `Möbi𝒰s` publicly
only with the subtitle “BiDi Coherence-Delta language/runtime,” keeping the
Universal Operator visibly above both product and language. Use bare `𝒰` only
as the reduced body of that same operator, never as a competing identity.

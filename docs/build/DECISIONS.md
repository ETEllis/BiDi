# Build Decisions — BiDi/CDC lane

Format: decision id, date, decision, rationale, consequences. Append-only.

## D1 — 2026-07-22 — Amendment adopted as binding over the toolchain plan

The CDC Toolchain + Memory Manifold Full-Build Amendment (2026-07-22) is
adopted as an amendment to `CDC_TOOLCHAIN_PLAN.md`, not a competing rewrite.
Where the original plan text conflicts, the amendment governs; the specific
supersessions are recorded in the plan's Amendment Record section. Execution
follows the amendment's Phase A–L sequence with this repository owning the
generic language/runtime/toolchain concerns only (no Memory Manifold product
code here beyond a small conformance fixture).

## D2 — 2026-07-22 — Interim digest algorithm

Amendment A3 mandates BLAKE3 (content) + Ed25519 (signatures) as canonical.
Neither is vendored yet. Until the vendored BLAKE3 lands (Phase B/D), Phase A
provenance records use git object ids plus SHA-256, labeled interim. All
interim records are re-digested with BLAKE3 when it lands; the re-digest is
recorded as evidence, and no public claim may rest on an interim digest.

## D3 — 2026-07-22 — Capability identifier partition

G9 (toolchain driver), H6–H11 (toolchain frameworks), H12–H17 (Memory
Manifold frameworks) reserved as listed in `CDC_MEMORY_MANIFOLD_INTERFACE.md`
§2. Live registry (A–F, G1–G8, H1–H5, U1) is untouched. No identifier reuse.

## D4 — 2026-07-22 — Remote-environment baseline

This lane executes in the Claude Code remote container against the GitHub
remote (`ETEllis/BiDi-Coherence-Delta-Calculus`). The operator-local branch
`codex/mobius-u-identity-system` @ `313f0a167a755b94f102969291a4f9ba415ed4f7`
named by the amendment was never pushed to this remote and is unreachable
here. The Phase A freeze therefore records the observed remote identity
(`origin/main` = 8cfe48f, work branch HEAD at freeze = 99747e0). If the
operator later pushes the local branch, a reconciliation pass compares it
against this baseline before any of its content is treated as canonical.

## D5 — 2026-07-22 — Grammar quirk preservation for parity

The legacy loader strips `#` comments before shell-style splitting
(`cdc_boot.py` `strip_comment` → `split_line`), so `#` truncates a line even
inside a quoted attribute value. The grammar-1 frontend must replicate this
behavior exactly while the legacy path is the differential oracle, and must
record it as a typed diagnostic candidate. Changing the behavior is a
grammar-version bump, never a silent fix.

## D13 — 2026-07-27 — BLAKE3 landed; D2 interim-digest gate CLOSED

`runtime/cdc_blake3.{h,c}` vendors a portable, dependency-free BLAKE3
(unkeyed hash mode, 32-byte output; keyed/derive/XOF modes are absent
rather than wrong). `cdc_digest` is now a thin surface over it and labels
digests `blake3:`. Verification: 31 reference vectors covering single
block, block boundary, chunk boundary (1024), and multi-level trees up to
17 chunks — each checked one-shot AND through irregular growing streaming
splits, under plain and ASan builds, gated in verify.sh against the
committed fixture `tests/fixtures/digest/blake3_vectors.txt` (generated
from the official bindings; the length-0/1/3 entries match the published
spec vectors). Evidence was re-digested by the vendored implementation
itself (`digest-file`), producing `evidence/gates/CT0/blake3-manifest.txt`
and `digest-migration.txt`; the interim `sha256-manifest.txt` is retained
unmodified as the historical record. Ed25519 signing remains queued and
unclaimed.

## D12 — 2026-07-27 — Post-merge branch reconciliation (ADR)

PR #3 was merged under the baton's Section 1 authorization after verifying
all three preconditions at the pinned head `82ab066` (remote head exact,
single required check completed/success, mergeable_state clean, no
drift); merge commit `3e851ff`, merge method `merge` to preserve the
commit identities referenced by gate evidence. The baton directs "create
the next CDC completion branch from the new main"; this environment
authorizes pushes only to `claude/bun-equivalent-build-plan-lxe772`, and
the harness rule for a merged PR is to restart that same branch name from
the default branch and open a NEW pull request. Reconciliation: the
branch was restarted from the merged `main` (`3e851ff`) and continuation
work opens a new PR. Substance matches the baton (fresh branch state from
new main, new PR); only the branch NAME is carried over, and PR #3 is
never reused.

## D11 — 2026-07-24 — Read faults are EIO, never torn tails

The f1f68c0 final re-review's surgical blocker is repaired: every scan
read now distinguishes EOF from an I/O fault. A short or zero fread with
the stream error indicator set (or the injected read fault armed)
surfaces as CDC_STORE_EIO — never SCAN_TAIL, never truncation, never a
handle. Non-regular log paths (directory, FIFO, device) are rejected by
fstat before a single byte is interpreted. A read-fault injection hook
(cdc_store_set_read_fail_after) makes the distinction permanently
testable. Permanent regressions in verify.sh (plain + ASan):
directory-as-log → EIO/no handle; injected fault before any seal →
EIO/bytes unchanged; injected fault after a seal → EIO/bytes
unchanged/no truncation; disarmed control opens clean. The 726-byte
mutation sweep, named high-length case, and seven-boundary crash matrix
are unchanged and green.

## D10 — 2026-07-24 — Store record format v2: authenticated framing

The 3122af5 pinned re-review's final blocker (unauthenticated payload_len:
a high-byte length mutation classified as a torn tail and truncated sealed
history) is repaired by versioning the record format to v2 ("CDC2"): every
header carries a framing tag digested over magic|type|seq|length, verified
BEFORE the length is trusted or any allocation occurs; a documented 64 MiB
per-record bound is enforced pre-allocation. Torn-tail classification is
reachable only after the framing tag proves the declared length authentic.
The seed phase has no persisted stores, so no migration exists. The
mutation matrix is now a FULL-BYTE SWEEP: all 726 bytes of the reference
sealed log flipped one at a time, each asserting ECORRUPT + no handle +
recovered=0 + byte-identical log (plain and ASan), plus the named
high-byte length case and the clean/valid-unsealed-tail controls. The
seven-boundary crash matrix is unchanged and green.

## D9 — 2026-07-24 — Independent merge-boundary review (bb3531b) adopted

The independent review at head `bb3531b` (REQUEST CHANGES, B1–B7) is
adopted as binding. All seven blockers are repaired with permanent
counterexamples in `verify.sh`:
B1/B2 — the store scanner is a typed three-state scan (CLEAN /
TORN-OR-UNSEALED TAIL / CORRUPT COMMITTED PREFIX) shared by
open/verify/replay/attest; sequence continuity and seal digests are
verified; corruption of a structurally complete record fails closed with
CDC_STORE_ECORRUPT, returns no handle, and NEVER mutates the log
(6 byte-flip counterexamples + clean and valid-unsealed-tail controls);
recovery truncation is itself made durable before recovery is reported.
B3 — HOLD authorization binds to the typed executable statement identity
(directive == runtime record form AND first argument == job id); the
unrelated-witness spoof is a permanent negative fixture.
B4 — zero executed runs is never green; fork/wait/child-output failures
count as failures; per-invocation child output paths.
B5 — a declared-but-incomplete reducer family is a typed fused error
(three one-kind-missing counterexamples).
B6 — identity 3D verifier creates its log directory.
B7 — file counts computed by the shell (no ls|wc), BSD-portable.
Deferred with the reviewer's structure intact: replacing stdout
classification with structured per-check result records folds into the
CT2/CT3 per-check-vector work already queued in RESUME_HERE; the
re-review gate's testable requirement (spoof rejection) is met now.
macOS-runner CI lane is queued as an operator-environment item.

## D8 — 2026-07-23 — Adversarial review adopted; identity branch integration

The PR #3 Adversarial Gate Review (2026-07-23) is adopted as binding:
verdict repair-and-continue. Both blocking defects (diagnostic fixed-slot
overflow; unreadable/special paths accepted as empty programs) are repaired
with permanent counterexample tests hard-gated in `verify.sh` (including
under ASan/UBSan). Gate-state language is corrected to: CT0 PARTIAL, CT1
PASS (with the counterexamples incorporated), CT2 SEED/PARTIAL; the plan
commit is not a gate. Review item C3 is resolved by an exact derived
statement gate (records + structural end lines).

D4's "unreachable/unpushed" statement about
`codex/mobius-u-identity-system` is hereby scoped to the 2026-07-22 freeze
moment: the branch now exists at
`origin/codex/mobius-u-identity-system` = `313f0a1`, sharing merge base
`8cfe48f` with the PR branch — visible parallel history, not corruption.
Integration decision: merge that branch into the PR branch as the combined
tip (the only authorized push target), resolving `scripts/verify.sh`,
`.gitignore`, and `README.md` overlaps intentionally, and require the
combined tree to pass the full gate before PR #3 leaves draft. Note: the
identity branch carries Python under `tools/blender/` — outside the
root-scoped host boundary enforced by kernel.cdc/verify.sh, and owned by
the identity/product lane, not the language substrate; recorded here so the
boundary claim stays precise.

## D7 — 2026-07-22 — Bootloader `--dump` differential record (gated host code)

`cdc_boot.py` gains a `--dump` flag emitting one declaration record per parsed
line (and raw token records for `expect` lines), used by gate CT1 to compare
the legacy loader field-for-field against the grammar-1 frontend
(`runtime/cdc_frontend_check.c dump`). This is host code inside the one
permitted host file, paired with the named deletion gate
**frontend-differential-dump**: the flag is deleted together with the legacy
line scanner when CT1 completes and no production command parses with the
legacy path. Dump mode changes no loader semantics (records are printed from
the same parse the loader already performs; expectations are not evaluated in
dump mode).

## D6 — 2026-07-22 — Single work branch and PR

All BiDi/CDC lane work lands on the session-designated branch
`claude/bun-equivalent-build-plan-lxe772` (draft PR #3), committing at gate
boundaries, rather than per-phase branches. Rationale: the branch is the
authorized push target for this execution environment; PR #3 carries CI
(`verify.sh --require-formal`) on every push, which enforces the
every-commit-green discipline the amendment requires.

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

## D22 — 2026-07-28 — Deletion gate step 1: the attribute-key shadowing defect

Migrating the runtimes off the legacy line scanner starts by reading it
closely, and reading it closely found a live defect.

`cdc_read_attr` located an attribute with `strstr(line, "key=")` — a bare
substring match with no token boundary. So an attribute whose NAME ENDS
WITH the key was read instead of the key:

    field f1 action-gain=9.0 gain=1.0 dt=0.125

reading `gain` returned **9.0**. Not missing — wrong, and confidently so,
which is why nothing downstream could notice. Field gain feeds nest
integration, so a source shaped that way would have produced beliefs nine
times too large with every expectation still "passing" against whatever the
wrong arithmetic produced.

The corpus never tripped it. The frontend differential has always reported
`collision=0`, and it still does — which is precisely why the defect
survived: the oracle agreed with the buggy reader because no input
distinguished them.

**The repair is the key match only.** The key must now begin a token: at
line start, or immediately after whitespace. Value extraction is unchanged
— raw, up to whitespace, quotes included — because the runtime's consumers
depend on that exact behavior and the grammar-1 frontend deliberately
differs there (the `quoting` divergence class, 4879 cases). Fixing the
value semantics at the same time would have been a silent behavior change
wearing a bug fix's clothes.

**Two counterexamples, because one was not enough.** `attr-boundary` pins
eight cases at the unit level, including the reverse direction (the longer
name must still read) and a suffix that is not an attribute at all. And
`tests/fixtures/frontend_boundary/shadowed_gain.cdc` executes it: the nest
job is expectation-pinned to the value the CORRECT gain produces, so the
fixture fails on the old reader and passes on the new one. Verified by
rebuilding the old reader and running both — the fixture fails, and the
real corpus stays green either way.

## D21 — 2026-07-28 — CT0: verdicts name their corpus; builds reproduce

A verdict that does not say what it ran on is a claim about nothing in
particular. `cdc test` and `cdc verify --vectors` now stamp a CORPUS
IDENTITY: an ordered digest over (basename, content-digest) for the exact
sources consumed.

Order is part of it, because the checkers evaluate in load order and their
results depend on it — the same files in a different order are a different
corpus. A partial corpus is never digested: if any input is unreadable the
digest is refused rather than computed over what happened to be readable,
and `cdc test` marks the run failed rather than reporting an unidentifiable
pass.

**The cross-check uses a different binary.** `cdc_frontend_check
corpus-digest` computes the same identity independently, and the gate
requires the two to agree. Checking a verdict's corpus with the same code
that produced the verdict would be circular. The counterexample completes
it: appending a line to a consumed source must change the identity, with
the probe restored before any assertion runs.

**Why the contract report is excluded.** It must stay byte-identical to the
bootloader, so it cannot carry a corpus line; the identity lives on the
vector stream instead, which has no oracle to match. That exclusion is a
consequence of bootloader parity and retires with `cdc_boot.py`, and it is
recorded here rather than left for a reader to notice.

**Reproducible builds, scoped honestly.** The unified binary built twice
from the same sources is byte-identical, which proves the build embeds no
timestamp, path, or nondeterministic ordering. That is same-machine,
same-compiler reproducibility. Cross-toolchain and cross-machine
reproducibility is a different and much stronger claim, and it is NOT made.

## D20 — 2026-07-28 — Closure witnesses complete the section-7 record

The parity vector's sixth field rendered "-" everywhere, because nothing
linked an executed effect to the claim declared about it. Those are
different things: the runtime knows what happened, the source says what was
supposed to happen, and until they are joined a reader has to take the
correspondence on trust.

A receipt now carries `witness=<id> closure=<digest>` — the witness bound
to that job and a digest of its canonical statement — and the execution
vector carries the digest through. The runtime does not EVALUATE witnesses;
the contract checker owns that. It only needs to name the one an effect
discharges, so the effect and the claim can be compared rather than
assumed to match.

Three details worth pinning:

- **A witness named without its digest is refused at emit.** "This effect
  discharges W" is only evidence if W is identified; a dangling name would
  look like a link while carrying nothing.
- **Truncation is refused, not tolerated.** The witness statement is
  digested, so a silently truncated statement would yield a confidently
  WRONG digest. The buffer matches the line limit and over-long statements
  fail loudly.
- **Unbound jobs render "-" and the gate names them.** 36 of 37 execution
  vectors carry a closure witness; the one that does not is `rival-latch`,
  the deliberately unbound helper in the compare-and-set counterexample.
  The gate asserts that identity, so a future regression that silently
  drops witnesses cannot hide behind the same count.

This completes the section-7 record: every field now carries an honest
value or an explicit "-", and no column is reserved-but-empty.

## D19 — 2026-07-28 — Lifecycle contract, and what determinism excludes

An executor that cannot be bounded or stopped is not embeddable. `cdc run`
now honours a budget (`CDC_MAX_EFFECTS`) and cancellation (SIGINT/SIGTERM,
with `CDC_CANCEL_AFTER` as a deterministic test hook).

**Both are enforced at EFFECT BOUNDARIES — between whole effects, never
inside one.** That placement is the entire guarantee: an effect either runs
completely or does not begin. For durable effects it is what makes a
stopped run safe, because a staged-but-uncommitted store transaction is
exactly "nothing happened".

The gate measures that rather than asserting it. At five different stop
points the store is opened externally and must report `open=ok`,
`recovered=0`, and `verify=ok` — intact, not merely recoverable — and
across all five stop points there must be exactly **two** distinct replay
identities: the state before the single durable append and the state after
it. A third value would mean an effect was observed half-applied.

**A stop is a hold, not a failure.** `budget-exhausted` and `cancelled` are
typed reasons in the same vocabulary as `balance-violation`; exit code 4
distinguishes a lifecycle stop from a violated expectation (1). A run that
was stopped says so in the carrier's own terms instead of dying.

**Configuration arrives by environment, not argv.** The legacy CLIs keep
byte-identical argument handling, so the unified driver's passthrough
parity gate is untouched — the same reasoning that put receipts on their
own channel in D17.

**What determinism covers, and what it deliberately does not.** Prose,
receipts, and vectors are byte-identical across runs, gated by running
twice and comparing. The store's INSTANCE identity (its uuid) is NOT
reproducible, and that is a design decision rather than an oversight:
making it reproducible would defeat the base-substitution defence from D16,
which relies on two stores with identical histories being distinguishable.
Determinism therefore covers observable outputs; attestation over a store
instance is excluded, and the gate states the exclusion where a reader will
meet it.

**Honest boundary on cancellation.** `CDC_CANCEL_AFTER` proves the boundary
semantics deterministically, at the same point the real signal is observed.
The handler itself is a single async-signal-safe assignment. End-to-end
asynchronous delivery timing under load is not gated here; a flaky timing
test would be worse evidence than saying so.

The whole-binary sanitizer sweep landed with this: the unified `cdc` binary
— the composition that actually ships — is now built under ASan/UBSan and
must produce a contract report, parity vectors, and gate line
byte-identical to the plain build. Previously only the frontend, the
persistence path, and the receipt carrier were instrumented.

## D18 — 2026-07-28 — Ordered per-check parity vectors (interface section 7)

Parity was compared as a byte-identical contract REPORT. That is real
evidence, but a report is prose: it carries no per-check identity a
consumer can compare field-for-field, and nothing makes ordering part of
the compared value rather than an incidental property of two texts that
happen to match.

Both `cdc verify` and `cdc test` now export the section-7 vector — one
record per check, in order:

    <identifier> <decision> <coordinate> <effects> <trace> <closure>

`decision` is the ternary carrier's own vocabulary (commit | hold | nest |
fail), never pass/fail, and the gate rejects a binary vocabulary appearing
in that column.

**The trace digest chains.** trace_i = digest(trace_{i-1} || effects_i), so
a record that moves position changes its own trace digest and every later
one. Ordering is therefore part of the value being compared, not something
a reader has to notice. The counterexample measures exactly that: swapping
two ADJACENT checks diverges 250 of 253 records. If only those two had
changed, ordering would be checkable but not load-bearing.

**What the contract oracle does and does not test.** The bootloader cannot
produce BLAKE3 digests — no stdlib BLAKE3, and shelling out per check would
be 250+ subprocesses — so its vectors are re-rendered in C from the report
it independently computed. The digest function is therefore shared and is
NOT under test; it does not need to be, being already gated by 31 reference
vectors. What IS under test is everything the two implementations compute
separately: each check's identifier, its verdict, its full evaluated label,
and their order. This is stated rather than glossed, because a reader could
otherwise mistake the shared digest for an independent confirmation.

**Fields with no honest value are "-".** Contract checks have no bridge
coordinate. No check carries a closure witness digest yet — that field is
reserved and empty rather than filled with a placeholder that would read as
evidence. Carrying closure witnesses on the receipt is the next open item,
and until it lands the column stays "-".

Execution vectors are rendered from the receipts (D17), with the receipt
line as the effects payload, so the vector and the receipt cannot describe
different effects. ABI gains `cdc_runtime_vectors`.

## D17 — 2026-07-28 — Effects are reported, not inferred from prose (ABI 1.3)

`cdc test` decided every verdict by string-matching the runtime's HUMAN
report line: `strstr(line, "status=held")`, then splitting `<form>=<jobid>`
out of the prose. That made the report format load-bearing for the gate.
Rewording a line could change a verdict; so could a payload that happened
to contain the text `status=accepted`.

A **typed effect receipt** is now the record of one executed effect, built
by the same code that produces the outcome, BEFORE the prose is printed.
Both the report line and the receipt render from that one struct, so they
cannot describe different things. Receipts carry job id, kind, op, the
balanced-ternary outcome, the typed reason, whether the source declared the
hold, and — for persistence — the durable and replay observations, sealed
and event counts, and the store generation.

Three decisions worth pinning:

- **The carrier is a separate channel, not a change to the existing one.**
  Receipts go to the path named by `CDC_RECEIPTS` and nothing is written
  when it is unset, so the human surface is byte-identical (gated by
  comparing two runs). Mixing them into stdout would have made the same
  mistake in a new format.
- **Parity is gated, not assumed.** The old prose classifier is retained as
  a SHADOW and its counts must match the receipt-derived counts exactly;
  `parity=0` appears in the gate line. If the two channels ever disagree —
  an effect gaining a receipt without a line, or the reverse — the build
  fails instead of the typed channel quietly winning. The counts did not
  move (`runs=24 commit=19 hold=8 nest=10`); what moved is the basis for
  them.
- **A mode with no effects emits an EMPTY stream, never no stream.** The
  runtime opens the receipt file eagerly, so a consumer can distinguish
  "nothing happened" from "this runtime does not report effects" — the
  second is a contract violation and has to be detectable. `cdc test`
  treats a missing stream as a hard failure and never falls back to prose,
  because silently falling back would restore the fragility being removed.

Hold authorization is still bound to the declaring statement's typed
identity (review B3): the consumer re-checks `declared_hold` against the
parsed program, so a receipt cannot authorize its own hold.

ABI minor version bumped to 1.3; `cdc_receipt.h` is part of the stable
boundary, so external consumers read effects without parsing reports.

## D16 — 2026-07-28 — Store generations, real serialization, and the claims withdrawn

An independent review of `1ea1ddd` found two mechanism defects and three
claims the code did not support. All are repaired; the claims that were
wrong are withdrawn rather than softened.

**Repaired — snapshot/compaction is one atomic generation transition.**
Previously `snapshot` published a base that `open` immediately treated as
active, and `compact` truncated the log afterwards: two public operations
with a crash window between them that left the store unopenable. The base
now lives in the log's own HEAD record, carrying the store uuid, a
monotonic generation, and an anchor over the HEAD it replaces. `snapshot`
writes `base.pending`, which `open` never reads; `compact` rebuilds the log
and activates it with fsync -> rename -> directory fsync. A kill at any of
the eight boundaries of that transition leaves the old generation or the
new one, never a mixture.

**Repaired — the fence is now backed by real mutual exclusion.** `commit`
scanned the sealed count and then appended with no lock, so two writers
could both pass the check before either wrote. An fcntl write lock on
`lock.cdcstore` is now held across re-scan + append + fsync(file) +
fsync(dir), and across compaction. The armed token is the whole
(generation, sealed, replay-state) triple, because a compaction can leave
the sealed count identical while replacing the representation underneath
it. The old test could not have caught this: it was sequential, so the
winner always finished before the loser checked.

**Repaired — attestation covers the base.** It digests from byte zero,
which now includes the HEAD, so two different histories compacted to the
same sealed count attest differently, and two stores with identical
histories still attest differently.

**Withdrawn claims.** "Authenticated snapshot" is wrong: the tags are
unkeyed digests that detect corruption, not forgery. The base is now
IDENTITY-BOUND (uuid plus generation), which defeats substitution and
stale replay, and that is the claim made. "Real compare-and-set" was wrong
at the reviewed head and is only true now that the lock exists. "Completed
store protocol" was premature. Compaction does not preserve replayable
history — it keeps a commitment to that history and discards the events;
the surviving property is replay IDENTITY, and the documents now say so.

**Not claimed, recorded as an open boundary.** Rolling an entire log file
back to a previous generation is detectable only by an observer who
retained the generation externally; nothing inside one directory can
distinguish "never compacted" from "rolled back". Unkeyed digests do not
stop an attacker who can rewrite the whole file. An external anchor plus
Ed25519 signing over the HEAD is the queued repair.

**Process change.** The provenance manifest is regenerated from
`git ls-files` and gated byte-for-byte, because a hand-maintained manifest
had drifted to 166 entries against 186 tracked files while claiming to
cover all of them. And the macOS app now has a required CI lane: the Linux
structural gate was correct as a platform guard but was the only required
UI check, so a surface that did not compile reached a green PR.

## D15 — 2026-07-28 — Persistence is a language form, not a host service (G10/H6)

`store` and `persist` are source directives (capability `H6`,
`framework_persistence.cdc`), so durable state is exercised through
`cdc run`/`cdc test` rather than as a C library with CDC branding.

The load-bearing decision is HOW the gate is wired: `op=append` does not
"check a policy and then call the store" — it invokes the identical
`execute_commit` that governs in-memory latching, and only an accepted
decision reaches `cdc_store_stage`/`cdc_store_commit`. A violated prefix
balance stages nothing, so there is no code path on which a held decision
could write. Latch-or-hold is therefore a property of the wiring, not a
convention observed at the call site.

Two consequences worth pinning:

- **Durability and replay identity are OBSERVED, never declared.** Every
  persist job recomputes the replay digest and the sealed-bytes attest
  before and after, and reports `durable=yes|no` / `replay=stable|changed`
  from that comparison. `expect-durable` and `expect-replay` check the
  observation; two permanent counterexamples (`overclaimed_durability.cdc`,
  `overclaimed_replay.cdc`) fail closed. Compaction is the one place the
  two identities legitimately diverge (`durable=yes replay=stable`), which
  is the D14 chain restated at the language level.
- **The A7 typed policy extends to durable mutation with no exception.**
  A persistence hold is an ordinary hold: it must be declared
  `expect-status=held` on its own `persist` statement or `cdc test --gate`
  fails, exactly as for a commit hold, even though the runtime exits 0
  (`silent_persist_hold.cdc`). The test runner needed one mode rule, no
  policy change — the ternary vocabulary already covered it.

Two hold reasons are added to the runtime vocabulary: `fence-violation`
(a stale compare-and-set view) and `compact-uncovered` (compaction with no
covering snapshot base). Both hold rather than write.

Honest boundary recorded in FRAMEWORKS.md and the matrix: these are
runtime-checked per-run properties with counterexamples, not theorems, and
`cdc_store` remains single-writer — the fence makes a stale writer fail
closed exactly once, after which its handle is spent and must be reopened.
The `.cdc` compare-and-set counterexample (`framework_persistence.cdc`,
two declared handles on one directory) respects that and does not write
again through the spent handle.

The `cdc_boot.py` additions this required are collect-only (two directives,
two step sets, two witness link forms, two expectation heads) and carry no
persistence semantics; they retire with the file under the existing
**toolchain-verify-parity** gate, recorded in the mandate.

## D14 — 2026-07-28 — Store protocol completion: resumable replay chain

> **Partly superseded by D16.** The "real compare-and-set" claim below
> was wrong when written: commit checked the sealed count and then
> appended with no mutual exclusion, so two writers could both pass.
> The snapshot/compaction sequence described below also had a crash
> window between publishing the base and truncating the log. Both are
> repaired in D16; read that entry for the mechanism in force.

snapshot, compact, and compare-and-set fence are implemented, completing the
generic store protocol. The replay digest changed from a one-pass fold to a
RESUMABLE CHAIN (state_i = digest(state_{i-1} || record_digest_i)) so a
snapshot can record a state the scan resumes from. This is what makes
compaction honest: after compacting, the replay identity is byte-identical
to the pre-compaction value while the attest digest over raw log bytes
legitimately changes — semantic identity survives a physical rewrite, and
the two digests answer different questions by design.

Snapshots are authenticated exactly like log records (magic | version |
sealed | events | state | tag over all of it), written temp -> fsync ->
rename -> directory fsync. A tampered snapshot fails the OPEN rather than
seeding a wrong base: all 85 snapshot bytes are swept individually and every
one fails closed. Compaction refuses unless the snapshot covers the entire
sealed prefix (sealed, events, and state all matching), so it can never
discard a record the base does not account for.

The fence is a real compare-and-set: arming pins the sealed count a writer
believes it is extending, and commit re-reads the log and refuses with
ESTATE if another writer moved it — proven by a two-handle stale-writer
counterexample asserting the loser wrote zero bytes. This is the MM1
stale-writer primitive.

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

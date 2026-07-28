# Proposal: delete `cdc_boot.py` and close Mandate Gate 5

> **DECIDED — 2026-07-28: Option A (freeze).** The operator chose to
> freeze `cdc_boot.py` as a CI-only differential oracle rather than
> delete it: "Do not delete a valuable independent reference until
> native coverage renders it genuinely redundant." Executed as D33: the
> kernel floor is `python-files == 0` with the oracle exempt BY NAME
> (exemption rendered in every report), `bootloader minimal` still
> enumerates the raw file set, the freeze banner is in the file itself,
> and Gate 5 is recorded closed on the RUNTIME dependency in
> `NATIVE_SELF_HOSTING_MANDATE.md`. The analysis below is preserved as
> the decision's record.

**Status: PROPOSED — NOT EXECUTED. Requires Edward's approval.**

This touches `kernel.cdc`, which is the language contract, so it is written
up rather than done. Everything below is reversible until the kernel floor
changes; that flip is the point of no return.

## What the bootloader still does

Five things, all in `scripts/verify.sh`:

| line | use | is it load-bearing? |
|---|---|---|
| 81 | `py_compile` syntax check | no — only meaningful while the file exists |
| 90 | `python-files == ["cdc_boot.py"]` host boundary | no — the assertion becomes `== []` |
| 158, 360 | the **contract report**, oracle for `toolchain-verify-parity` | **YES** |
| 183 | `--dump`, oracle for the **frontend differential** (5342 records) | **YES** |
| 237, 421 | **rejection parity** — every invalid fixture must be rejected by both | **YES** |

Three of the five are real oracles. They are the reason the native
implementations are trustworthy rather than merely self-consistent.

## What is actually lost

This is the part worth being blunt about.

`cdc_boot.py` is the only **independent** implementation of the language's
collect-and-check semantics. It is a different implementation, in a
different language, written before the C one. Every gate that compares them
— the byte-identical contract report, the 5342-record frontend dump, the
rejection parity over the invalid-fixture corpus — derives its value
entirely from that independence.

Delete it and those three gates do not get weaker. **They stop existing.**
The C frontend and the C contract evaluator would be checked only against
themselves and against expectations written by the same hand. A defect
present in both the parser and its test would be invisible — which is
precisely the class of defect the differential has been catching (D22's
attribute shadowing was found by reading, not by the gate, because the gate
compared two readers that agreed).

There is no way to replace this by adding more C tests. Independence is the
property, and it cannot be manufactured from inside the same implementation.

## What deletion would look like

```diff
--- a/kernel.cdc
+++ b/kernel.cdc
-  expect host-debt <= 1
-  expect python-files == 1
-  expect bootloader minimal == true
+  expect host-debt == 0
+  expect python-files == 0
```

and in `scripts/verify.sh`: remove the `py_compile` step, change the host
boundary assertion to `py == []`, and delete the three differential gates
(contract report parity, frontend dump differential, rejection parity)
because their oracle is gone.

Net: the repository would contain **zero** Python and would verify itself
entirely in C. That is the mandate's stated end state.

## Recommendation

**Do not delete yet.** Not because the migration is incomplete — it is
complete, both runtimes are on the grammar-1 frontend and the C contract
evaluator has been byte-identical to the bootloader for every commit in
this branch. Delete it when the *independence* it provides has been replaced
or is judged no longer worth its cost, and that is a judgement about risk
appetite, not a technical blocker.

Two options that preserve independence, either of which would make deletion
straightforwardly safe:

1. **Freeze the oracle instead of deleting it.** Keep `cdc_boot.py` in the
   tree but mark it non-production: exempt it from `python-files` by name,
   run it only in CI as the differential oracle, and forbid new features in
   it. The mandate's intent — the language does not depend on a Python host
   to *run* — is satisfied; what remains is a test fixture that happens to
   be written in Python.

2. **Replace the oracle before removing it.** Port the collect-and-check
   semantics to a second, deliberately independent implementation (a
   different language, or a from-spec rewrite that does not share code with
   `cdc_registry.c`), stand up the same three differentials against it, and
   then delete the bootloader with its oracles intact.

Option 1 is cheap and honest. Option 2 is expensive and stronger. Deleting
outright is cheapest and weakest, and it is the only one of the three that
silently reduces what the gate can detect.

## The ask

Which of these do you want?

- **A — Freeze:** keep `cdc_boot.py` as a CI-only oracle, renegotiate
  `python-files == 1` to a by-name exemption, and record Gate 5 as closed
  on the *runtime* dependency rather than on file count.
- **B — Replace then delete:** commission the second implementation first.
- **C — Delete now:** flip the floor to `== 0`, accept the loss of the three
  differentials, and record it explicitly in the obligation matrix as a
  reduction in verification coverage.

I recommend **A**. It reaches the mandate's actual goal — no host dependency
in the language's execution path — without trading away the only
independent check the project has. `python-files == 1` was always a proxy
for "the language does not need a host to run"; the proxy has drifted from
the property, and A realigns them.

I will not touch `kernel.cdc` until you choose.

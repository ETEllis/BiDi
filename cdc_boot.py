#!/usr/bin/env python3
"""
cdc_boot: minimal Python bootloader for native .cdc contracts.

This file is intentionally small. It does not implement the calculus reducer,
the semantic registry, or the witness suite in Python. Those live in .cdc files.
The bootloader only reads .cdc source, records native declarations, and verifies
declared expectations.
"""
from __future__ import annotations

import shlex
import sys
from dataclasses import dataclass, field
from pathlib import Path


WITNESS_LINK_FORMS = {
    "reducer": ("flow", "commit", "nest"),
    "guard": ("guard",),
    "trace": ("trace",),
    "measure": ("measure",),
    "policy": ("policy",),
    "bridge": ("bridge",),
    "counter": ("counter",),
    "compile": ("compile",),
    "interpret": ("interpret",),
    "council": ("deliberate",),
    "evolution": ("evolve",),
    "universal": ("universal",),
}

FORM_PRIMITIVES = {"deliberate": "council", "evolve": "evolve"}


@dataclass
class BootState:
    root: Path
    dump: bool = False
    kernel: dict[str, str] = field(default_factory=dict)
    terms: set[str] = field(default_factory=set)
    rules: set[str] = field(default_factory=set)
    provides: set[str] = field(default_factory=set)
    bootloader_steps: set[str] = field(default_factory=set)
    invariants: dict[str, dict[str, str]] = field(default_factory=dict)
    capabilities: dict[str, dict[str, str]] = field(default_factory=dict)
    frameworks: dict[str, dict[str, str]] = field(default_factory=dict)
    witnesses: dict[str, dict[str, str]] = field(default_factory=dict)
    reducer_forms: dict[str, dict[str, dict[str, str]]] = field(default_factory=dict)
    guard_steps: set[str] = field(default_factory=set)
    reducer_steps: set[str] = field(default_factory=set)
    trace_steps: set[str] = field(default_factory=set)
    measure_steps: set[str] = field(default_factory=set)
    policy_steps: set[str] = field(default_factory=set)
    bridge_steps: set[str] = field(default_factory=set)
    counter_steps: set[str] = field(default_factory=set)
    compile_steps: set[str] = field(default_factory=set)
    interpret_steps: set[str] = field(default_factory=set)
    proof_steps: set[str] = field(default_factory=set)
    council_steps: set[str] = field(default_factory=set)
    evolution_steps: set[str] = field(default_factory=set)
    universal_steps: set[str] = field(default_factory=set)
    expectations: list[tuple[str, list[str], str]] = field(default_factory=list)


def strip_comment(line: str) -> str:
    return line.split("#", 1)[0].strip()


def split_line(line: str, filename: str, line_no: int) -> list[str]:
    try:
        return shlex.split(line)
    except ValueError as exc:
        raise SyntaxError(f"{filename}:{line_no}: {exc}") from exc


def kv(tokens: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for token in tokens:
        if "=" in token:
            key, value = token.split("=", 1)
            out[key] = value
    return out


def flags(tokens: list[str]) -> list[str]:
    return [token for token in tokens if "=" not in token]


def parse_file(state: BootState, path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    for line_no, raw in enumerate(text.splitlines(), start=1):
        line = strip_comment(raw)
        if not line or line == "end":
            continue
        tokens = split_line(line, str(path), line_no)
        if not tokens:
            continue
        cmd, rest = tokens[0], tokens[1:]
        args, attrs = flags(rest), kv(rest)
        source = f"{path.name}:{line_no}"

        if state.dump:
            # Differential record for the grammar-1 frontend (gate CT1).
            # Deletion gate: frontend-differential-dump -- removed together
            # with the legacy line scanner once CT1 passes and the native
            # frontend is the only parser (DECISIONS D7).
            if cmd == "expect":
                print(f"{source}|expect|{' '.join(rest)}|")
            else:
                attr_text = ";".join(f"{k}={v}" for k, v in attrs.items())
                print(f"{source}|{cmd}|{','.join(args)}|{attr_text}|")

        if cmd == "kernel":
            if not args:
                raise SyntaxError(f"{source}: kernel requires a name")
            state.kernel = {"name": args[0], **attrs}
        elif cmd == "term":
            state.terms.update(args)
        elif cmd == "rule":
            state.rules.update(args)
        elif cmd == "provides":
            state.provides.update(args)
        elif cmd == "bootloader":
            state.bootloader_steps.update(args)
        elif cmd in {"invariant", "law"}:
            if not args:
                raise SyntaxError(f"{source}: {cmd} requires a key")
            key = args[0]
            state.invariants.setdefault(key, {})
            state.invariants[key].update({"kind": cmd, **attrs})
        elif cmd == "capability":
            if not args:
                raise SyntaxError(f"{source}: capability requires a key")
            key = args[0]
            state.capabilities.setdefault(key, {})
            state.capabilities[key].update(attrs)
        elif cmd == "framework":
            if not args:
                raise SyntaxError(f"{source}: framework requires a key")
            key = args[0]
            if key in state.frameworks:
                raise SyntaxError(f"{source}: duplicate framework {key!r}")
            state.frameworks[key] = {"source": source, **attrs}
        elif cmd == "witness":
            if not args:
                raise SyntaxError(f"{source}: witness requires an id")
            wid = args[0]
            state.witnesses[wid] = {"source": source, **attrs}
        elif cmd in {
            "field",
            "module",
            "cell",
            "channel",
            "guard",
            "counter",
            "flow",
            "commit",
            "nest",
            "trace",
            "measure",
            "policy",
            "bridge",
            "compile",
            "interpret",
            "proof",
            "council",
            "deliberate",
            "evolve",
            "universal",
        }:
            if not args:
                raise SyntaxError(f"{source}: {cmd} requires an id")
            key = args[0]
            state.reducer_forms.setdefault(cmd, {})
            state.reducer_forms[cmd][key] = {"source": source, "args": " ".join(args), **attrs}
            if cmd == "guard":
                state.guard_steps.add(key)
            if cmd in {"flow", "commit", "nest"}:
                state.reducer_steps.add(key)
            if cmd == "trace":
                state.trace_steps.add(key)
            if cmd == "measure":
                state.measure_steps.add(key)
            if cmd == "policy":
                state.policy_steps.add(key)
            if cmd == "bridge":
                state.bridge_steps.add(key)
            if cmd == "counter":
                state.counter_steps.add(key)
            if cmd == "compile":
                state.compile_steps.add(key)
            if cmd == "interpret":
                state.interpret_steps.add(key)
            if cmd == "proof":
                state.proof_steps.add(key)
            if cmd == "deliberate":
                state.council_steps.add(key)
            if cmd == "evolve":
                state.evolution_steps.add(key)
            if cmd == "universal":
                state.universal_steps.add(key)
        elif cmd == "expect":
            state.expectations.append((source, rest, line))
        else:
            raise SyntaxError(f"{source}: unknown directive {cmd!r}")


def python_files(state: BootState) -> list[Path]:
    return sorted(p for p in state.root.glob("*.py") if p.is_file())


def witnesses_for(state: BootState, key: str, field: str) -> list[str]:
    return [wid for wid, attrs in state.witnesses.items() if attrs.get(field) == key]


def resolve_witness_link(state: BootState, attrs: dict[str, str]) -> tuple[str | None, str]:
    links = [link for link in WITNESS_LINK_FORMS if link in attrs]
    if len(links) != 1:
        return None, f"needs exactly one executable link (got {links or 'none'})"
    link = links[0]
    job = attrs[link]
    for form in WITNESS_LINK_FORMS[link]:
        if job in state.reducer_forms.get(form, {}):
            return FORM_PRIMITIVES.get(form, form), f"{link}={job}"
    return None, f"{link}={job} has no declared job"


def check_framework_complete(state: BootState, key: str) -> tuple[bool, str]:
    framework = state.frameworks.get(key)
    if not framework:
        return False, f"framework {key} complete (missing framework declaration)"
    label = framework.get("label", "")
    if not label:
        return False, f"framework {key} complete (missing label)"
    required = [role for role in framework.get("requires", "").split(",") if role]
    permitted = {prim for prim in framework.get("permits", "").split(",") if prim}
    if not required or not permitted:
        return False, f"framework {key} complete (missing requires/permits contract)"
    bound: dict[str, str] = {}
    problems: list[str] = []
    for wid, attrs in sorted(state.witnesses.items()):
        if attrs.get("framework") != label:
            continue
        role = attrs.get("role", "")
        if not role:
            problems.append(f"{wid} missing role")
            continue
        if role not in required:
            problems.append(f"{wid} carries unknown role {role}")
            continue
        if role in bound:
            problems.append(f"role {role} duplicated by {wid}")
            continue
        primitive, link_detail = resolve_witness_link(state, attrs)
        if primitive is None:
            problems.append(f"{wid} {link_detail}")
            continue
        if primitive not in permitted:
            problems.append(f"{wid} role {role} uses unpermitted primitive {primitive}")
            continue
        bound[role] = wid
    missing = [role for role in required if role not in bound]
    if missing:
        problems.append(f"missing roles {missing}")
    detail = f"roles {len(bound)}/{len(required)}"
    if problems:
        detail += "; " + "; ".join(problems)
    return not problems, f"framework {key} complete ({detail})"


def compare(got: int | str, op: str, want: int | str) -> bool:
    if isinstance(got, int) and isinstance(want, int):
        if op == "==":
            return got == want
        if op == ">=":
            return got >= want
        if op == "<=":
            return got <= want
    if op == "==":
        return got == want
    raise ValueError(f"unsupported comparison: {got!r} {op} {want!r}")


def eval_expect(state: BootState, args: list[str]) -> tuple[bool, str]:
    if not args:
        return False, "empty expectation"

    head = args[0]
    if head == "native" and args[1:3] == ["substrate", "=="]:
        want = args[3]
        got = state.kernel.get("target", "")
        return got == want, f"native substrate == {want} (got {got or 'unset'})"

    if head == "host-debt":
        op, want = args[1], int(args[2])
        got = 1 if state.bootloader_steps else 0
        return compare(got, op, want), f"host-debt {op} {want} (got {got})"

    if head == "frameworks" and args[1:] == ["closed"]:
        labels = {fw.get("label", "") for fw in state.frameworks.values()}
        orphans = sorted({
            attrs["framework"]
            for attrs in state.witnesses.values()
            if "framework" in attrs and attrs["framework"] not in labels
        })
        detail = f"orphan bindings {orphans}" if orphans else f"{len(labels)} labels"
        return not orphans, f"frameworks closed ({detail})"

    if head in {"terms", "rules", "invariants", "witnesses", "capabilities", "frameworks"}:
        op, want = args[1], int(args[2])
        collections = {
            "terms": state.terms,
            "rules": state.rules,
            "invariants": state.invariants,
            "witnesses": state.witnesses,
            "capabilities": state.capabilities,
            "frameworks": state.frameworks,
        }
        got = len(collections[head])
        return compare(got, op, want), f"{head} {op} {want} (got {got})"

    if head == "provides":
        missing = [item for item in args[1:] if item not in state.provides]
        return not missing, f"provides {' '.join(args[1:])}" + (f" (missing {missing})" if missing else "")

    if head == "law":
        key = args[1]
        linked = witnesses_for(state, key, "invariant")
        return key in state.invariants and bool(linked), f"law {key} (witnesses {len(linked)})"

    if head == "capability":
        key = args[1]
        linked = witnesses_for(state, key, "capability")
        return key in state.capabilities and bool(linked), f"capability {key} (witnesses {len(linked)})"

    if head == "framework" and len(args) == 3 and args[2] == "complete":
        return check_framework_complete(state, args[1])

    if head == "witness":
        wid = args[1]
        return wid in state.witnesses, f"witness {wid}"

    if head == "reducer":
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"reducer {wid} (missing witness)"
        step = witness.get("reducer")
        ok = bool(step and step in state.reducer_steps)
        detail = f"step {step}" if step else "missing reducer link"
        return ok, f"reducer {wid} ({detail})"

    if head in {"guard", "trace", "measure", "policy", "bridge", "counter", "universal"}:
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"{head} {wid} (missing witness)"
        step = witness.get(head)
        step_sets = {
            "guard": state.guard_steps,
            "trace": state.trace_steps,
            "measure": state.measure_steps,
            "policy": state.policy_steps,
            "bridge": state.bridge_steps,
            "counter": state.counter_steps,
            "universal": state.universal_steps,
        }
        ok = bool(step and step in step_sets[head])
        detail = f"job {step}" if step else f"missing {head} link"
        return ok, f"{head} {wid} ({detail})"

    if head == "compile":
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"compile {wid} (missing witness)"
        step = witness.get("compile")
        ok = bool(step and step in state.compile_steps)
        detail = f"job {step}" if step else "missing compile link"
        return ok, f"compile {wid} ({detail})"

    if head == "proof":
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"proof {wid} (missing witness)"
        step = witness.get("proof")
        ok = bool(step and step in state.proof_steps)
        detail = f"job {step}" if step else "missing proof link"
        return ok, f"proof {wid} ({detail})"

    if head == "interpret":
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"interpret {wid} (missing witness)"
        step = witness.get("interpret")
        ok = bool(step and step in state.interpret_steps)
        detail = f"job {step}" if step else "missing interpret link"
        return ok, f"interpret {wid} ({detail})"

    if head == "council":
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"council {wid} (missing witness)"
        step = witness.get("council")
        ok = bool(step and step in state.council_steps)
        detail = f"job {step}" if step else "missing council link"
        return ok, f"council {wid} ({detail})"

    if head == "evolution":
        wid = args[1]
        witness = state.witnesses.get(wid)
        if not witness:
            return False, f"evolution {wid} (missing witness)"
        step = witness.get("evolution")
        ok = bool(step and step in state.evolution_steps)
        detail = f"job {step}" if step else "missing evolution link"
        return ok, f"evolution {wid} ({detail})"

    if head == "python-files":
        op, want = args[1], int(args[2])
        files = python_files(state)
        return compare(len(files), op, want), f"python-files {op} {want} (got {len(files)}: {[p.name for p in files]})"

    if head == "bootloader":
        if args[1:] == ["minimal", "==", "true"]:
            files = python_files(state)
            ok = len(files) == 1 and files[0].name == "cdc_boot.py"
            return ok, f"bootloader minimal == true (python files {[p.name for p in files]})"

    return False, f"unknown expectation: {' '.join(args)}"


def report(state: BootState) -> bool:
    print("=" * 74)
    print("  .cdc native contract report")
    print("=" * 74)
    passed = 0
    for source, args, _line in state.expectations:
        ok, label = eval_expect(state, args)
        passed += int(ok)
        print(f"  {'OK' if ok else 'FAIL'} {label}   [{source}]")
    print("-" * 74)
    print(f"  {passed}/{len(state.expectations)} expectations met")
    print(f"  {len(state.terms)} terms, {len(state.rules)} rules, {len(state.invariants)} invariants")
    print(f"  {len(state.capabilities)} capabilities, {len(state.frameworks)} frameworks, {len(state.witnesses)} native witnesses")
    print("=" * 74)
    return passed == len(state.expectations)


def main(argv: list[str]) -> int:
    root = Path(__file__).resolve().parent
    dump = "--dump" in argv
    argv = [arg for arg in argv if arg != "--dump"]
    paths = [root / arg for arg in argv] if argv else sorted(root.glob("*.cdc"))
    state = BootState(root=root, dump=dump)
    for path in paths:
        parse_file(state, path)
    if dump:
        return 0
    return 0 if report(state) else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

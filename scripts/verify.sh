#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

REQUIRE_FORMAL=0

usage() {
  cat <<'EOF'
Usage: ./scripts/verify.sh [--require-formal]

  --require-formal  Fail if Lean, Coq/Rocq, or Tectonic are unavailable.
EOF
}

while (($#)); do
  case "$1" in
    --require-formal)
      REQUIRE_FORMAL=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown verify option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

on_error() {
  local status=$?
  local line=${BASH_LINENO[0]:-${LINENO}}
  echo "verify failed at line ${line}: ${BASH_COMMAND} (exit ${status})" >&2
}
trap on_error ERR

run_step() {
  echo "+ $*"
  "$@"
}

require_or_skip() {
  local tool=$1
  local label=$2

  if command -v "$tool" >/dev/null 2>&1; then
    return 0
  fi

  if (( REQUIRE_FORMAL )); then
    echo "$tool is required for --require-formal ($label)" >&2
    exit 1
  fi

  echo "$tool not found; skipping $label"
  return 1
}

assert_fresh_file() {
  local generated=$1
  local tracked=$2
  local refresh_hint=$3

  test -s "$generated"
  if ! cmp -s "$generated" "$tracked"; then
    echo "$tracked is stale; regenerate with: $refresh_hint" >&2
    exit 1
  fi
}

mkdir -p build

echo "== Minimal Python bootloader syntax =="
python3 - <<'PY'
import py_compile

py_compile.compile("cdc_boot.py", cfile="build/cdc_boot.pyc", doraise=True)
PY

echo
echo "== Python host boundary =="
python3 - <<'PY'
from pathlib import Path

py = sorted(p.name for p in Path(".").glob("*.py"))
assert py == ["cdc_boot.py"], py
print("python host boundary: ok (cdc_boot.py only)")
PY

echo
echo "== Möbi𝒰s identity asset contract =="
python3 - <<'PY'
from pathlib import Path
import xml.etree.ElementTree as ET

root = Path(".")
identity = root / "assets" / "identity"
svgs = [
    identity / "mobius-embodied-mark.svg",
    identity / "mobius-u-operator.svg",
    identity / "mobius-u-wordmark-dark.svg",
    identity / "mobius-u-wordmark-light.svg",
    identity / "mobius-u-code-sigil.svg",
    identity / "mobius-u-code-sigil-dark.svg",
    identity / "mobius-ius-relational.svg",
    identity / "mobius-bi-seed.svg",
    identity / "mobius-bidi-kernel.svg",
    identity / "mobius-bidi-delta.svg",
]

for path in svgs:
    if not path.is_file():
        raise SystemExit(f"identity asset missing: {path}")
    node = ET.parse(path).getroot()
    if "viewBox" not in node.attrib:
        raise SystemExit(f"identity asset lacks viewBox: {path}")
    if not any(child.tag.endswith("title") for child in node):
        raise SystemExit(f"identity asset lacks accessible title: {path}")

for path in identity.glob("mobius-u-wordmark-*.svg"):
    text = path.read_text(encoding="utf-8")
    if "<text" in text:
        raise SystemExit(f"wordmark must use portable outlines, not live text: {path}")
    for token in ("7.16197", "circle", "stroke-linecap"):
        if token not in text:
            raise SystemExit(f"wordmark missing identity invariant {token!r}: {path}")

motion_text = (root / "demo" / "mobius-identity.html").read_text(encoding="utf-8")
for token in ("PRESENCE", "WORDMARK GENERATION", "I𝒰S PHASE", "UI RESTORATION", "의 RESTORATION", "BIDI SELF-EXTRACTION", "TRIADIC DELTA CLOSURE", "CODE SIGIL", "FAMILY LOCKUP", "prefers-reduced-motion", "0.125 rad", "master-video", "mobius-identity-master.mp4", "mobius-wordmark-static.png", "stageFrames"):
    if token not in motion_text:
        raise SystemExit(f"motion lab missing contract token {token!r}")

docs = [
    root / "docs" / "identity" / "MOBIUS_U_IDENTITY_SYSTEM.md",
    root / "docs" / "identity" / "MOBIUS_U_GEOMETRY_SPEC.md",
    root / "docs" / "identity" / "MOBIUS_U_MOTION_SPEC.md",
    root / "docs" / "identity" / "MOBIUS_U_MIGRATION_AUDIT.md",
    root / "docs" / "identity" / "MOBIUS_U_BLENDER_PIPELINE.md",
    root / "docs" / "identity" / "MOBIUS_U_INTEGRATION.md",
]
if not all(path.is_file() for path in docs):
    raise SystemExit("identity documentation set is incomplete")

print(f"mobius identity assets: ok ({len(svgs)} svg, {len(docs)} contracts, 1 motion lab)")
PY

run_step ./scripts/verify_identity_3d.sh

echo
run_step ./scripts/verify_ui.sh

echo
echo "== Native .cdc contract and witness suite =="
python3 cdc_boot.py

echo
echo "== Canonical frontend (grammar 1) differential [gate CT1] =="
command -v cc >/dev/null 2>&1 || {
  echo "cc is required for canonical frontend verification" >&2
  exit 1
}
rm -f build/cdc_frontend_check
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  runtime/cdc_frontend_check.c \
  runtime/cdc_abi.c \
  runtime/cdc_registry.c \
  runtime/cdc_store.c \
  runtime/cdc_digest.c \
  runtime/cdc_blake3.c \
  runtime/cdc_receipt.c \
  runtime/cdc_parser.c \
  runtime/cdc_ast.c \
  runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c \
  runtime/cdc_source.c \
  -o build/cdc_frontend_check
CDC_ROOT_SOURCES=$(ls ./*.cdc | LC_ALL=C sort)
# shellcheck disable=SC2086
python3 cdc_boot.py --dump $CDC_ROOT_SOURCES > build/frontend_dump_boot.txt
# shellcheck disable=SC2086
./build/cdc_frontend_check dump $CDC_ROOT_SOURCES > build/frontend_dump_native.txt
cmp build/frontend_dump_boot.txt build/frontend_dump_native.txt
FRONTEND_RECORDS=$(wc -l < build/frontend_dump_boot.txt)
test "$FRONTEND_RECORDS" -ge 5000
echo "frontend differential ok records=${FRONTEND_RECORDS}"
# shellcheck disable=SC2086
run_step ./build/cdc_frontend_check roundtrip $CDC_ROOT_SOURCES
# shellcheck disable=SC2086
./build/cdc_frontend_check attr-parity $CDC_ROOT_SOURCES | tee build/frontend_attr_parity.txt
grep -q " collision=0 " build/frontend_attr_parity.txt
grep -q " duplicate=0 " build/frontend_attr_parity.txt
grep -q " failed=0" build/frontend_attr_parity.txt
run_step ./build/cdc_frontend_check bounds
run_step ./build/cdc_frontend_check oom framework_loop.cdc
# Attribute-key boundaries [deletion-gate step 1]. The legacy reader matched
# `key=` as a bare substring, so an attribute whose NAME ends with the key
# was read instead: `gain` read `action-gain=9.0` as 9.0. Confidently wrong
# rather than missing, so nothing downstream could notice. The corpus never
# tripped it (collision=0), which is exactly why it survived.
run_step ./build/cdc_frontend_check attr-boundary
# Consumed-attribute inventory. This began as the precondition for the
# grammar-1 migration (D23): the legacy reader took a value raw up to
# whitespace while grammar-1 strips quotes, so migrating was
# behaviour-preserving only while no CONSUMED attribute was ever quoted.
#
# Both runtimes are now migrated (D24, D25), so grammar-1 handles quoting
# correctly and that hazard is gone. The check is kept, re-scoped and
# re-labelled rather than deleted, because it still enforces something
# real: the runtimes read values as bare tokens and compare them against
# expectation strings written in the sources, so a quoted consumed
# attribute would still mean the source and the runtime disagree about what
# the value IS. The floor below keeps it from going vacuous if the
# extraction ever stops matching — which is how it caught its own
# obsolescence during the migration.
CONSUMED_ATTRS=build/consumed_attrs.txt
grep -ohE 'stmt_(attr_copy|copy_attr|int_attr|double_attr)\(stmt, *"[^"]+"' \
  runtime/cdc_native_runtime.c runtime/cdc_bridge_runtime.c \
  | sed 's/.*"\(.*\)"/\1/' | LC_ALL=C sort -u > "$CONSUMED_ATTRS"
CONSUMED_COUNT=$(wc -l < "$CONSUMED_ATTRS")
# A broken extraction must not make this gate vacuous.
test "$CONSUMED_COUNT" -ge 90
QUOTED_CONSUMED=0
while read -r ATTR; do
  if grep -qhE "(^|[[:space:]])${ATTR}=\"" ./*.cdc tests/fixtures/*/*.cdc 2>/dev/null; then
    echo "runtime-consumed attribute carries a quoted value: ${ATTR}" >&2
    echo "  the legacy reader truncates it at the first space; migrating to" >&2
    echo "  the grammar-1 frontend would change its value" >&2
    QUOTED_CONSUMED=$((QUOTED_CONSUMED + 1))
  fi
done < "$CONSUMED_ATTRS"
test "$QUOTED_CONSUMED" = "0"
echo "consumed-attribute inventory ok (${CONSUMED_COUNT} attributes, 0 quoted)"

for fixture in tests/fixtures/frontend/*.cdc; do
  if python3 cdc_boot.py "$fixture" >/dev/null 2>&1; then
    echo "legacy loader accepted invalid fixture ${fixture}" >&2
    exit 1
  fi
  run_step ./build/cdc_frontend_check reject "$fixture"
done
echo "frontend rejection parity ok fixtures=$(ls tests/fixtures/frontend/*.cdc | wc -l)"
SANITIZED=0
if cc -std=c99 -Wall -Wextra -pedantic -O1 -fsanitize=address,undefined \
  runtime/cdc_frontend_check.c \
  runtime/cdc_abi.c \
  runtime/cdc_registry.c \
  runtime/cdc_store.c \
  runtime/cdc_digest.c \
  runtime/cdc_blake3.c \
  runtime/cdc_receipt.c \
  runtime/cdc_parser.c \
  runtime/cdc_ast.c \
  runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c \
  runtime/cdc_source.c \
  -o build/cdc_frontend_check_asan 2>/dev/null; then
  SANITIZED=1
  # shellcheck disable=SC2086
  run_step ./build/cdc_frontend_check_asan roundtrip $CDC_ROOT_SOURCES
  run_step ./build/cdc_frontend_check_asan bounds
  run_step ./build/cdc_frontend_check_asan attr-boundary
else
  echo "sanitizers unavailable; skipping instrumented frontend pass"
fi

echo
echo "== CT0 provenance manifest is head-bound [2026-07-28 review, finding 4] =="
# The manifest used to claim "all tracked files" while drifting to 166
# entries against 186 tracked files, omitting the BLAKE3 implementation that
# produced it, and carrying stale digests nothing checked. Two assertions
# now bind it to the head: the PATH SET catches added or removed files, and
# the BYTES catch modified ones.
./scripts/regen_provenance.sh build/blake3-manifest.txt
git ls-files | LC_ALL=C sort \
  | grep -v '^evidence/gates/CT0/blake3-manifest\.txt$' > build/tracked_paths.txt
grep '^blake3:' evidence/gates/CT0/blake3-manifest.txt \
  | sed 's/^blake3:[0-9a-f]*  //' | LC_ALL=C sort > build/manifest_paths.txt
if ! cmp -s build/tracked_paths.txt build/manifest_paths.txt; then
  echo "CT0 manifest does not cover the tracked set; regenerate with:" >&2
  echo "  ./scripts/regen_provenance.sh" >&2
  diff build/tracked_paths.txt build/manifest_paths.txt | head -20 >&2
  exit 1
fi
assert_fresh_file build/blake3-manifest.txt \
  evidence/gates/CT0/blake3-manifest.txt "./scripts/regen_provenance.sh"
echo "CT0 provenance ok entries=$(grep -c '^blake3:' evidence/gates/CT0/blake3-manifest.txt) tracked=$(wc -l < build/tracked_paths.txt)"
# Counterexample: modifying ANY tracked file must break the gate. The probe
# file is restored before any assertion runs, so a failure here cannot leave
# the working tree dirty.
PROVENANCE_PROBE=CITATION.cff
cp "$PROVENANCE_PROBE" build/provenance_probe.bak
printf '\n' >> "$PROVENANCE_PROBE"
set +e
./scripts/regen_provenance.sh build/manifest_probe.txt > /dev/null 2>&1
PROBE_RC=$?
set -e
cp build/provenance_probe.bak "$PROVENANCE_PROBE"
cmp "$PROVENANCE_PROBE" build/provenance_probe.bak
test "$PROBE_RC" = "0"
if cmp -s build/manifest_probe.txt evidence/gates/CT0/blake3-manifest.txt; then
  echo "provenance gate did not notice a modified tracked file" >&2
  exit 1
fi
echo "provenance gate rejects a modified tracked file (probe restored)"

echo
echo "== Stable ABI and unified driver skeleton [gate CT2 seed] =="
rm -f build/cdc
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  runtime/toolchain/main.c \
  runtime/toolchain/cmd_verify.c \
  runtime/toolchain/cmd_test.c \
  runtime/cdc_abi.c \
  runtime/cdc_registry.c \
  runtime/cdc_parser.c \
  runtime/cdc_ast.c \
  runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c \
  -DCDC_NATIVE_NO_MAIN -DCDC_BRIDGE_NO_MAIN \
  runtime/cdc_native_runtime.c \
  runtime/cdc_bridge_runtime.c \
  runtime/cdc_source.c \
  runtime/cdc_store.c \
  runtime/cdc_digest.c \
  runtime/cdc_blake3.c \
  runtime/cdc_receipt.c \
  -lm \
  -o build/cdc
run_step ./build/cdc version
./build/cdc version | grep -q "abi=1.3 grammar=1"
# shellcheck disable=SC2086
./build/cdc verify --parse $CDC_ROOT_SOURCES | tee build/cdc_verify_parse.txt
# Exact statement gate (review item C3): statements = dump records plus
# structural end lines; both counted from the same frozen corpus.
END_LINES=$(cat ./*.cdc | grep -cE '^[[:space:]]*end[[:space:]]*(#.*)?$')
# Review B7: count via the shell, not ls|wc (BSD wc pads with spaces).
set -- ./*.cdc
ROOT_FILE_COUNT=$#
EXPECTED_STATEMENTS=$((FRONTEND_RECORDS + END_LINES))
grep -q "cdc verify parse ok files=${ROOT_FILE_COUNT} statements=${EXPECTED_STATEMENTS}\$" \
  build/cdc_verify_parse.txt
echo "cdc verify statement gate exact: files=${ROOT_FILE_COUNT} statements=${EXPECTED_STATEMENTS}"
if ./build/cdc verify --parse tests/fixtures/frontend/unknown_directive.cdc \
  >/dev/null 2>&1; then
  echo "cdc verify accepted an invalid fixture" >&2
  exit 1
fi
echo "cdc driver rejection ok (typed JSON diagnostics)"
if ./build/cdc run >/dev/null 2>&1; then
  echo "cdc run should fail closed before Phase C" >&2
  exit 1
fi
echo "cdc unimplemented commands fail closed"
# Gate toolchain-verify-parity: the native contract evaluator must produce
# a byte-identical report to the bootloader, in success AND failure modes,
# with matching exit semantics. This is the recorded deletion gate for
# cdc_boot.py (Mandate Gate 5).
python3 cdc_boot.py > build/contract_boot.txt
# shellcheck disable=SC2086
./build/cdc verify --contract $CDC_ROOT_SOURCES > build/contract_native.txt
cmp build/contract_boot.txt build/contract_native.txt
echo "toolchain-verify-parity ok (byte-identical contract reports)"
# Ordered per-check parity vectors (interface section 7, amendment A5).
# Byte-identical reports already imply the same checks in the same order,
# but a report is prose: it carries no per-check identity a consumer can
# compare field-for-field, and nothing forces ordering to be part of the
# compared VALUE. The vector does both.
#
# The oracle side is re-rendered from the bootloader's independently
# computed report. The digest function is shared and is therefore not what
# is under test — it is already gated by 31 reference vectors. What is under
# test is everything the two implementations compute separately: each
# check's identifier, its verdict, its full evaluated label, and their
# order.
# shellcheck disable=SC2086
./build/cdc verify --vectors $CDC_ROOT_SOURCES > build/vectors_native.txt
./build/cdc_frontend_check vectors-from-report build/contract_boot.txt \
  > build/vectors_oracle.txt
# The oracle has no corpus record (the bootloader does not compute one),
# so compare the check records and check the corpus line separately.
grep -v '^corpus ' build/vectors_native.txt > build/vectors_native_checks.txt
cmp build/vectors_native_checks.txt build/vectors_oracle.txt
VECTOR_COUNT=$(wc -l < build/vectors_native_checks.txt)
test "$VECTOR_COUNT" -ge 250
# Every record carries all six section-7 fields.
awk '$1 == "corpus" { next } NF != 6 { print "malformed vector: " $0; exit 1 }' \
  build/vectors_native.txt
echo "per-check vector parity ok records=${VECTOR_COUNT} fields=6"
# Counterexample: ORDER is part of the compared value, not merely the
# sequence of comparisons. Swapping two ADJACENT checks must diverge far
# beyond those two records, because the trace digest chains forward.
python3 - <<'SWAP'
from pathlib import Path

lines = Path("build/contract_boot.txt").read_text().split("\n")
idx = [i for i, l in enumerate(lines)
       if l.startswith("  OK ") or l.startswith("  FAIL ")]
if len(idx) < 10:
    raise SystemExit("not enough check records to test ordering")
a, b = idx[3], idx[4]
lines[a], lines[b] = lines[b], lines[a]
Path("build/contract_boot_swapped.txt").write_text("\n".join(lines))
SWAP
./build/cdc_frontend_check vectors-from-report build/contract_boot_swapped.txt \
  > build/vectors_swapped.txt
if cmp -s build/vectors_oracle.txt build/vectors_swapped.txt; then
  echo "reordering two checks did not change the vector" >&2
  exit 1
fi
SWAP_DIFF=$(diff build/vectors_oracle.txt build/vectors_swapped.txt | grep -c '^<' || true)
# A swap of two adjacent records must propagate through the chain to the
# tail; if only those two differed, ordering would be checkable but not
# load-bearing.
test "$SWAP_DIFF" -gt 100
echo "vector ordering is load-bearing (2 adjacent checks swapped -> ${SWAP_DIFF}/${VECTOR_COUNT} records diverge)"
printf 'witness lonely-w capability=Z9 claim="fixture"\nexpect witness missing-w\nexpect capability Z9\nexpect frameworks closed\n' \
  > build/fixture_fail_expect.cdc
set +e
python3 cdc_boot.py build/fixture_fail_expect.cdc > build/contract_boot_neg.txt
BOOT_NEG_RC=$?
./build/cdc verify --contract build/fixture_fail_expect.cdc > build/contract_native_neg.txt
NATIVE_NEG_RC=$?
set -e
test "$BOOT_NEG_RC" = "1"
test "$NATIVE_NEG_RC" = "1"
cmp build/contract_boot_neg.txt build/contract_native_neg.txt
echo "toolchain-verify-parity failure-mode ok (identical FAIL reports, exit 1)"
# A11 linkability: both legacy runtimes must compile with their entry
# points excluded, proving the unified binary can link them (the standalone
# CLIs keep byte-identical behavior; conversion lands with full CT2).
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 -DCDC_NATIVE_NO_MAIN \
  -c runtime/cdc_native_runtime.c -o build/cdc_native_nomain.o
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 -DCDC_BRIDGE_NO_MAIN \
  -c runtime/cdc_bridge_runtime.c -o build/cdc_bridge_nomain.o
echo "runtime linkability ok (CDC_NATIVE_NO_MAIN + CDC_BRIDGE_NO_MAIN)"

echo
echo "== Unified driver passthrough parity [gate CT2] =="
# The unified binary must reproduce the standalone runtimes byte-for-byte,
# including exit codes, across every legacy mode family (parity is checked
# AFTER the standalone binaries are built later in this script would be too
# late — they are compiled here if absent).
rm -f build/cdc_native_runtime build/cdc_bridge_runtime
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  runtime/cdc_native_runtime.c runtime/cdc_source.c \
  runtime/cdc_store.c runtime/cdc_digest.c runtime/cdc_blake3.c \
  runtime/cdc_receipt.c \
  runtime/cdc_parser.c runtime/cdc_ast.c runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c -lm \
  -o build/cdc_native_runtime
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  runtime/cdc_bridge_runtime.c runtime/cdc_source.c \
  runtime/cdc_parser.c runtime/cdc_ast.c runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c \
  -o build/cdc_bridge_runtime
PASSTHROUGH_MODES=0
while IFS= read -r MODE_ARGS; do
  set +e
  # shellcheck disable=SC2086
  ./build/cdc_native_runtime $MODE_ARGS > build/passthrough_a.txt 2>&1
  RC_A=$?
  # shellcheck disable=SC2086
  ./build/cdc $MODE_ARGS > build/passthrough_b.txt 2>&1
  RC_B=$?
  set -e
  test "$RC_A" = "$RC_B"
  cmp build/passthrough_a.txt build/passthrough_b.txt
  PASSTHROUGH_MODES=$((PASSTHROUGH_MODES + 1))
done <<'MODES'
compile native_reducer.cdc
interpret native_reducer.cdc
prove native_reducer.cdc
surface native_surface.cdc
council council_bridge.cdc
evolve council_bridge.cdc
universal framework_loop.cdc
fused framework_loop.cdc
fused native_reducer.cdc
fused council_bridge.cdc
persist framework_persistence.cdc
fused framework_persistence.cdc
MODES
./build/cdc_bridge_runtime verify bridge64.cdc > build/passthrough_a.txt
./build/cdc bridge verify bridge64.cdc > build/passthrough_b.txt
cmp build/passthrough_a.txt build/passthrough_b.txt
./build/cdc_bridge_runtime lookup-dyadic bridge64.cdc 101011 > build/passthrough_a.txt
./build/cdc bridge lookup-dyadic bridge64.cdc 101011 > build/passthrough_b.txt
cmp build/passthrough_a.txt build/passthrough_b.txt
PASSTHROUGH_MODES=$((PASSTHROUGH_MODES + 2))
echo "unified passthrough parity ok modes=${PASSTHROUGH_MODES}"

echo
echo "== Fused single-process executor [gate CT3] =="
# cdc run = the fused executor: one parse, one live Runtime, every declared
# stage family; byte-identical to the native fused mode.
./build/cdc run framework_loop.cdc > build/fused_a.txt
./build/cdc_native_runtime fused framework_loop.cdc > build/fused_b.txt
cmp build/fused_a.txt build/fused_b.txt
grep -q "cdc fused ok stages=1 mode=universal source=framework_loop.cdc" build/fused_a.txt
./build/cdc run council_bridge.cdc > build/fused_a.txt
grep -q "cdc fused ok stages=2 source=council_bridge.cdc" build/fused_a.txt
./build/cdc run native_reducer.cdc > build/fused_a.txt
grep -q "cdc fused ok stages=1 source=native_reducer.cdc" build/fused_a.txt
if ./build/cdc run kernel.cdc >/dev/null 2>&1; then
  echo "fused run must fail closed on a source with no executable stage" >&2
  exit 1
fi
echo "fused executor ok (universal closure, multi-stage, fail-closed)"

echo
echo "== Typed effect receipts [gate CT3] =="
# `cdc test` used to decide verdicts by string-matching the runtime's HUMAN
# report line (strstr "status=held", then splitting "<form>=<jobid>" out of
# prose). The report format was load-bearing for the gate. Effects are now
# reported as typed records built by the same code that produces the
# outcome, and the gate classifies from those fields.
run_step ./build/cdc_frontend_check receipt-check
if [ "$SANITIZED" = "1" ]; then
  run_step ./build/cdc_frontend_check_asan receipt-check
fi
# The runtime honours the contract: with CDC_RECEIPTS set it ALWAYS creates
# the stream, so "no effects" is distinguishable from "emits no receipts".
rm -rf build/persistence-journal build/persistence-contended build/receipts.txt
CDC_RECEIPTS=build/receipts.txt ./build/cdc run framework_persistence.cdc \
  > build/receipts_prose.txt
test -f build/receipts.txt
RECEIPT_COUNT=$(grep -c "^cdc-receipt " build/receipts.txt)
test "$RECEIPT_COUNT" = "11"
# Every persist report line has exactly one receipt, and the typed fields
# say what the prose says.
PROSE_COUNT=$(grep -c "^persist=" build/receipts_prose.txt)
test "$PROSE_COUNT" = "$RECEIPT_COUNT"
grep -q "kind=persist job=journal-hold op=append outcome=0 reason=balance-violation declared-hold=1 trits=-+0 balance=violated witness=persistence-hold-native closure=blake3:.* durable=0 replay-stable=1" \
  build/receipts.txt
grep -q "kind=persist job=journal-latch op=append outcome=+1 reason=none declared-hold=0 trits=0+- balance=admissible witness=persistence-gate-native closure=blake3:.* durable=1 replay-stable=0" \
  build/receipts.txt
# Compaction is the record that proves durable and replay are independent
# observations, and it carries the generation the transition produced.
grep -q "kind=persist job=journal-compact op=compact outcome=+1 reason=none declared-hold=0 witness=persistence-compact-native closure=blake3:.* durable=1 replay-stable=1 sealed=1 events=1 generation=1" \
  build/receipts.txt
grep -q "kind=persist job=contended-stale op=append outcome=0 reason=fence-violation" \
  build/receipts.txt
# A mode with no effects still produces an EMPTY stream, never no stream.
rm -f build/receipts_empty.txt
CDC_RECEIPTS=build/receipts_empty.txt ./build/cdc surface native_surface.cdc \
  > /dev/null
test -f build/receipts_empty.txt
test ! -s build/receipts_empty.txt
# With the variable unset the human surface is byte-identical: receipts are
# an added channel, not a change to the existing one.
./build/cdc run framework_persistence.cdc > build/receipts_prose_b.txt
cmp build/receipts_prose.txt build/receipts_prose_b.txt
echo "effect receipts ok records=${RECEIPT_COUNT} empty-stream=1 prose-unchanged=1"

echo
echo "== Typed test runner [gate CT3 seed] =="
# The executable corpus, named once: the gate line, the vector export, the
# determinism rounds, and the sanitizer sweep must all run the same set, or
# the corpus identity they stamp would not be comparable.
DET_FILES="native_reducer.cdc native_surface.cdc council_bridge.cdc \
framework_transition.cdc framework_procedural.cdc framework_episodic.cdc \
framework_deliberative.cdc framework_loop.cdc framework_persistence.cdc"
# A7 policy: commit/hold/nest/fail reported separately (merged totals
# forbidden); every hold must be declared expect-status=held on its job or
# the gate fails, even when the underlying runtime exits 0.
./build/cdc test --gate \
  native_reducer.cdc native_surface.cdc council_bridge.cdc \
  framework_transition.cdc framework_procedural.cdc \
  framework_episodic.cdc framework_deliberative.cdc framework_loop.cdc \
  framework_persistence.cdc \
  | tee build/cdc_test_gate.txt
# The persistence framework contributes 8 accepted and 3 held durable
# records; all three holds are declared on their own persist statements, so
# the A7 policy applies to durable mutation with no policy exception.
grep -q "cdc test ok runs=24 commit=19 hold=8 (expected=8 unexpected=0) nest=10 fail=0 parity=0 corpus=blake3:" \
  build/cdc_test_gate.txt
# Negative: a source the legacy runtime fully accepts (exit 0) but whose
# hold is undeclared must fail the typed gate with unexpected=1 fail=0.
run_step ./build/cdc run tests/fixtures/test_runner/silent_hold.cdc
if ./build/cdc test --gate tests/fixtures/test_runner/silent_hold.cdc \
  > build/cdc_test_neg.txt 2>/dev/null; then
  echo "typed gate accepted an undeclared hold" >&2
  exit 1
fi
grep -q "(expected=0 unexpected=1) nest=1 fail=0 parity=0 corpus=blake3:" build/cdc_test_neg.txt
echo "typed gate rejects undeclared holds (runtime exit 0 notwithstanding)"
# Review B3: an unrelated witness carrying the job name and
# expect-status=held must NOT authorize the hold.
if ./build/cdc test --gate tests/fixtures/test_runner/spoofed_hold.cdc \
  > build/cdc_test_spoof.txt 2>/dev/null; then
  echo "typed gate accepted a spoofed hold authorization" >&2
  exit 1
fi
grep -q "(expected=0 unexpected=1)" build/cdc_test_spoof.txt
echo "typed gate rejects spoofed hold authorization (unrelated witness)"
# Review B4: zero executed runs is never green.
if ./build/cdc test --gate kernel.cdc > build/cdc_test_zero.txt 2>/dev/null; then
  echo "typed gate passed with zero executed runs" >&2
  exit 1
fi
grep -q "runs=0.*no executable stage selected" build/cdc_test_zero.txt
echo "typed gate fails on zero executed runs"
# Review B5: a declared-but-incomplete reducer family is a typed fused
# error, never silently skipped (three one-kind-missing counterexamples).
printf 'field f1 dt=0.125 gain=1.0 deadband=0.5\nmodule m1 field=f1 belief=0.0 prior=0.0 precision=1.0 action-gain=1.0\ncell m1.a module=m1 theta=0.0 amplitude=1.0 omega=0.0\nguard g1 cell=m1.a expect-state=open\n' > build/fused_base.txt
for MISSING in nest commit flow; do
  {
    cat build/fused_base.txt
    if [ "$MISSING" != "flow" ]; then
      printf 'flow f-x field=f1 duration=1.0\n'
    fi
    if [ "$MISSING" != "commit" ]; then
      printf 'commit c-x module=m1\n'
    fi
    if [ "$MISSING" != "nest" ]; then
      printf 'nest n-x parent=m1 child=m1\n'
    fi
  } > "build/fused_missing_${MISSING}.cdc"
  if ./build/cdc run "build/fused_missing_${MISSING}.cdc" >/dev/null 2>&1; then
    echo "fused run silently skipped an incomplete reducer (missing ${MISSING})" >&2
    exit 1
  fi
done
echo "fused run fails typed on incomplete reducer families (3 counterexamples)"

echo
echo "== Canonical digest: BLAKE3 reference vectors [closes D2] =="
# The vendored BLAKE3 must reproduce every reference vector one-shot AND
# through irregular streaming splits: single block, block boundary, chunk
# boundary (1024), and multi-level trees up to 17 chunks.
./build/cdc_frontend_check digest-vectors tests/fixtures/digest/blake3_vectors.txt \
  | tee build/digest_vectors.txt
grep -q "digest-vectors ok vectors=31 failed=0" build/digest_vectors.txt
if [ "$SANITIZED" = "1" ]; then
  run_step ./build/cdc_frontend_check_asan digest-vectors \
    tests/fixtures/digest/blake3_vectors.txt
fi

echo
echo "== Durable store substrate [gates CT4/MM1 seed] =="
# Digest vectors, replay determinism, typed statuses, and the crash matrix:
# injected failure at EVERY commit write/flush/sync boundary must recover
# to exactly the old or the new sealed state — never a partial batch.
rm -rf build/store_test build/store_crash build/store_corrupt
mkdir -p build/store_test build/store_crash build/store_corrupt
run_step ./build/cdc_frontend_check store-check build/store_test
./build/cdc_frontend_check store-crash build/store_crash | tee build/store_crash.txt
grep -q "store-crash ok boundaries=7 old=3 new=4" build/store_crash.txt
# Full mutation matrix (3122af5 re-review): EVERY byte of the sealed log
# flipped one at a time must fail closed (ECORRUPT, no handle,
# recovered=0, log bytes untouched), including the named high-byte length
# mutation; a fully valid unsealed tail stays recoverable and
# distinguished from corruption. The swept span grew from 726 to 903 bytes
# when the compaction base moved into the log's HEAD record (2026-07-28
# review, finding 1): the base is now covered by this sweep instead of
# living in a separate file open() had to trust.
./build/cdc_frontend_check store-corrupt build/store_corrupt | tee build/store_corrupt.txt
grep -q "store-corrupt ok swept=903 named=1 controls=2" build/store_corrupt.txt
# I/O-fault regressions (f1f68c0 re-review): read faults are EIO, never
# torn tails — a directory as log, a fault before any seal, and a fault
# after a seal must all fail closed with no handle, no truncation, and
# byte-identical logs.
rm -rf build/store_io
mkdir -p build/store_io
./build/cdc_frontend_check store-io build/store_io | tee build/store_io.txt
grep -q "store-io ok cases=3 controls=1" build/store_io.txt
# Protocol completion: snapshot, compact, and compare-and-set fence. The
# load-bearing claim is that compaction preserves the REPLAY identity (the
# chained semantic digest) while the attest digest over raw bytes changes,
# and that a stale writer's commit is refused without writing a byte. The
# snapshot is swept byte-by-byte: a tampered base must never be trusted.
# Out-of-process crash matrix: the child is genuinely SIGKILLed at each
# commit boundary, so unflushed stdio buffers are lost as in a power cut.
# The old/new split legitimately differs from the in-process matrix (a
# different loss mode), so only the invariants are gated: every child dies,
# and every recovery lands on exactly the old or the new sealed state — a
# partial state fails inside the harness itself.
rm -rf build/store_kill
mkdir -p build/store_kill
./build/cdc_frontend_check store-kill build/store_kill | tee build/store_kill.txt
grep -q "store-kill ok boundaries=7 killed=7" build/store_kill.txt
rm -rf build/store_protocol
mkdir -p build/store_protocol
./build/cdc_frontend_check store-protocol build/store_protocol \
  | tee build/store_protocol.txt
# The compaction base is the log's own HEAD record now, so tampering with
# it is tampering with the log: every byte of a compacted log fails closed.
# This replaces the old separate-snapshot sweep, which could only check a
# file open() should never have trusted in the first place.
grep -q "store-protocol HEAD sweep: 177/177 bytes fail closed" build/store_protocol.txt
grep -q "store-protocol ok snapshot=1 compact=1 fence=1 replay-identity-preserved=1" \
  build/store_protocol.txt
if [ "$SANITIZED" = "1" ]; then
  rm -rf build/store_crash_asan build/store_corrupt_asan build/store_io_asan
  mkdir -p build/store_crash_asan build/store_corrupt_asan build/store_io_asan
  run_step ./build/cdc_frontend_check_asan store-crash build/store_crash_asan
  run_step ./build/cdc_frontend_check_asan store-corrupt build/store_corrupt_asan
  run_step ./build/cdc_frontend_check_asan store-io build/store_io_asan
  rm -rf build/store_protocol_asan
  mkdir -p build/store_protocol_asan
  run_step ./build/cdc_frontend_check_asan store-protocol build/store_protocol_asan
fi

# 2026-07-28 review, findings 1 and 2. Both defects lived BETWEEN two
# individually-correct operations, so neither the crash matrix nor the
# protocol suite could see them:
#   1. the base was published as active before compaction truncated the
#      log (crash window), carried no store identity (substitutable), and
#      was invisible to attest (two histories, one attestation);
#   2. commit checked the sealed count and then appended with no mutual
#      exclusion (check-then-act race the sequential test could not enter).
# Both suites were verified to FAIL against deliberately re-broken builds
# before being accepted here.
rm -rf build/store_generation build/store_race
mkdir -p build/store_generation build/store_race
./build/cdc_frontend_check store-generation build/store_generation \
  | tee build/store_generation.txt
grep -q "snapshot-only reopen=ok handle=yes generation=0" build/store_generation.txt
grep -q "kill matrix: boundaries=8 .* mixed=0 unusable=0" build/store_generation.txt
grep -q "substitution twin-history=1 reopen=ok foreign-base-activated=0" \
  build/store_generation.txt
grep -q "compacted-attest store-diff=1 history-diff=1 attest-equal=0" \
  build/store_generation.txt
grep -q "stale-generation-base activated=0" build/store_generation.txt
grep -q "special-paths typed-eio=1 blocked=0" build/store_generation.txt
grep -q "store-generation ok atomic-transition=1 identity-bound=1 attest-covers-base=1" \
  build/store_generation.txt
./build/cdc_frontend_check store-race build/store_race | tee build/store_race.txt
grep -q "store-race ok rounds=3 winners=1/round refused=1/round corrupt=0" \
  build/store_race.txt
grep -q "commit-vs-compact commit-preserved=1" build/store_race.txt
if [ "$SANITIZED" = "1" ]; then
  rm -rf build/store_generation_asan build/store_race_asan
  mkdir -p build/store_generation_asan build/store_race_asan
  run_step ./build/cdc_frontend_check_asan store-generation \
    build/store_generation_asan
  run_step ./build/cdc_frontend_check_asan store-race build/store_race_asan
fi

echo
echo "== BiDi-gated durable persistence [gate CT4, capability H6] =="
# The store is a language citizen, not a C library with CDC branding:
# `store` and `persist` are source directives, so persistence is exercised
# through cdc run / cdc test, and durable mutation is gated by the SAME
# balanced-ternary barrier that governs in-memory latching.
rm -rf build/persistence-journal build/persistence-contended
./build/cdc run framework_persistence.cdc | tee build/persistence.txt
# The gate: an admissible barrier makes bytes durable and moves the replay
# identity; a violated barrier writes nothing at all.
grep -q "persist=journal-latch .* balance=admissible status=accepted reason=none sealed=1 events=1 durable=yes replay=changed" \
  build/persistence.txt
grep -q "persist=journal-hold .* balance=violated status=held reason=balance-violation sealed=1 events=1 durable=no replay=stable" \
  build/persistence.txt
# Compaction is the one place the two identities legitimately diverge.
grep -q "persist=journal-compact .* status=accepted .* durable=yes replay=stable" \
  build/persistence.txt
grep -q "persist=journal-compact-early .* status=held reason=compact-uncovered .* durable=no" \
  build/persistence.txt
# A real compare-and-set counterexample expressed in .cdc: two handles on
# one directory, the rival moves the log, the fenced writer's admissible
# append is refused before a byte is written.
grep -q "persist=rival-latch .* status=accepted .* durable=yes" build/persistence.txt
grep -q "persist=contended-stale .* balance=admissible status=held reason=fence-violation .* durable=no replay=stable" \
  build/persistence.txt
grep -q "native persistence ok stores=3 jobs=11 accepted=8 held=3" build/persistence.txt
echo "persistence gate ok (barrier-gated durable mutation)"

# Counterexample 1 — "a hold writes nothing", checked OUTSIDE the runtime
# that claims it: seed one accepted append, copy the sealed log, replay
# three violating appends over the same store, byte-compare.
rm -rf build/persistence-bytes
run_step ./build/cdc run tests/fixtures/persistence/latch_seed.cdc
cp build/persistence-bytes/log.cdcstore build/persistence_before.bin
test -s build/persistence_before.bin
run_step ./build/cdc run tests/fixtures/persistence/hold_writes_nothing.cdc
cmp build/persistence_before.bin build/persistence-bytes/log.cdcstore
echo "held appends leave the sealed log byte-identical ($(wc -c < build/persistence_before.bin) bytes, 3 holds)"

# Counterexamples 2 and 3 — durability and replay identity are OBSERVED,
# never taken from the declaration. Over-claiming either must fail closed.
for FIXTURE in overclaimed_durability overclaimed_replay; do
  if ./build/cdc run "tests/fixtures/persistence/${FIXTURE}.cdc" \
    > "build/persistence_${FIXTURE}.txt" 2>&1; then
    echo "persistence accepted an overclaimed ${FIXTURE} declaration" >&2
    exit 1
  fi
done
grep -q "persist durability expectation mismatch" \
  build/persistence_overclaimed_durability.txt
grep -q "persist replay expectation mismatch" \
  build/persistence_overclaimed_replay.txt
echo "persistence rejects overclaimed durability and replay identity"

# Counterexample 4 — A7 applies to durable holds: a persistence hold that
# is not declared on its own persist statement fails the typed gate even
# though the runtime exits 0.
run_step ./build/cdc run tests/fixtures/persistence/silent_persist_hold.cdc
if ./build/cdc test --gate tests/fixtures/persistence/silent_persist_hold.cdc \
  > build/persistence_silent.txt 2>/dev/null; then
  echo "typed gate accepted an undeclared durable hold" >&2
  exit 1
fi
grep -q "runs=1 commit=0 hold=1 (expected=0 unexpected=1) nest=0 fail=0 parity=0 corpus=blake3:" build/persistence_silent.txt
echo "typed gate rejects undeclared durable holds"
# Execution-side per-check vectors: one record per executed effect, in
# execution order, rendered from the receipts so the two cannot describe
# different effects.
./build/cdc test --gate --vectors build/test_vectors.txt \
  native_reducer.cdc native_surface.cdc council_bridge.cdc \
  framework_transition.cdc framework_procedural.cdc \
  framework_episodic.cdc framework_deliberative.cdc framework_loop.cdc \
  framework_persistence.cdc > /dev/null
awk 'NF != 6 { print "malformed execution vector: " $0; exit 1 }' \
  build/test_vectors.txt
EXEC_VECTORS=$(wc -l < build/test_vectors.txt)
# 19 commit + 8 hold + 10 nest, matching the gate line exactly.
test "$EXEC_VECTORS" = "37"
test "$(awk '$2 == "commit"' build/test_vectors.txt | wc -l)" = "19"
test "$(awk '$2 == "hold"' build/test_vectors.txt | wc -l)" = "8"
test "$(awk '$2 == "nest"' build/test_vectors.txt | wc -l)" = "10"
test "$(awk '$2 == "fail"' build/test_vectors.txt | wc -l)" = "0"
# The decision column is the ternary vocabulary, never pass/fail.
if grep -qE " (pass|true|false) " build/test_vectors.txt; then
  echo "execution vectors used a binary decision vocabulary" >&2
  exit 1
fi
# Closure witnesses (interface section 7, sixth field). An executed effect
# and the claim declared about it are different things; the closure digest
# is the link, so a consumer can check that what ran is what the source
# said would run. Jobs with no witness binding render "-" — honest, not a
# placeholder that would read as evidence.
WITNESSED=$(awk '$6 != "-"' build/test_vectors.txt | wc -l)
UNWITNESSED=$(awk '$6 == "-"' build/test_vectors.txt | wc -l)
test "$WITNESSED" = "36"
test "$UNWITNESSED" = "1"
# The single unwitnessed effect is the rival writer in the compare-and-set
# counterexample: a deliberately unbound helper job, not a dropped witness.
awk '$6 == "-"' build/test_vectors.txt | grep -q "rival-latch"
echo "closure witnesses ok bound=${WITNESSED} unbound=${UNWITNESSED} (rival-latch, by design)"

echo
echo "== CT0: reproducible binaries and corpus-bound verdicts =="
# A verdict that does not name what it ran on is a claim about nothing in
# particular. Every native verdict now carries the corpus identity: an
# ordered digest over (basename, content-digest) for the exact sources it
# consumed.
CORPUS_TEST=$(grep -o "corpus=blake3:[0-9a-f]*" build/cdc_test_gate.txt | head -1 | sed 's/corpus=//')
test -n "$CORPUS_TEST"
# Cross-checked by a DIFFERENT binary, so the gate is not trusting the same
# code that produced the claim.
# shellcheck disable=SC2086
CORPUS_INDEPENDENT=$(./build/cdc_frontend_check corpus-digest $DET_FILES \
  | awk '{print $2}')
test "$CORPUS_TEST" = "$CORPUS_INDEPENDENT"
# shellcheck disable=SC2086
CORPUS_VECTORS=$(grep '^corpus ' build/vectors_native.txt | awk '{print $2}')
# shellcheck disable=SC2086
CORPUS_ROOT=$(./build/cdc_frontend_check corpus-digest $CDC_ROOT_SOURCES \
  | awk '{print $2}')
test "$CORPUS_VECTORS" = "$CORPUS_ROOT"
echo "corpus-bound verdicts ok (cdc test and cdc verify --vectors agree with"
echo "  an independently computed corpus identity)"

# Counterexample: changing ANY consumed source must change the verdict's
# corpus. The probe is restored before any assertion runs.
CORPUS_PROBE=native_reducer.cdc
cp "$CORPUS_PROBE" build/corpus_probe.bak
printf '\n# corpus probe\n' >> "$CORPUS_PROBE"
set +e
# shellcheck disable=SC2086
CORPUS_CHANGED=$(./build/cdc_frontend_check corpus-digest $DET_FILES \
  | awk '{print $2}')
set -e
cp build/corpus_probe.bak "$CORPUS_PROBE"
cmp "$CORPUS_PROBE" build/corpus_probe.bak
if [ "$CORPUS_CHANGED" = "$CORPUS_TEST" ]; then
  echo "corpus identity did not change when a consumed source changed" >&2
  exit 1
fi
echo "corpus identity tracks source content (probe restored)"

# Reproducible native binaries: the same sources, built twice, byte-identical.
rm -f build/repro_a build/repro_b
for ROUND in a b; do
  cc -std=c99 -Wall -Wextra -pedantic -O2 \
    runtime/toolchain/main.c \
    runtime/toolchain/cmd_verify.c \
    runtime/toolchain/cmd_test.c \
    runtime/cdc_abi.c \
    runtime/cdc_registry.c \
    runtime/cdc_parser.c \
    runtime/cdc_ast.c \
    runtime/cdc_lexer.c \
    runtime/cdc_diagnostic.c \
    -DCDC_NATIVE_NO_MAIN -DCDC_BRIDGE_NO_MAIN \
    runtime/cdc_native_runtime.c \
    runtime/cdc_bridge_runtime.c \
    runtime/cdc_source.c \
    runtime/cdc_receipt.c \
    runtime/cdc_store.c \
    runtime/cdc_digest.c \
    runtime/cdc_blake3.c \
    -lm \
    -o "build/repro_${ROUND}"
done
cmp build/repro_a build/repro_b
./build/cdc_frontend_check digest-file build/repro_a > build/repro_digest.txt
echo "reproducible build ok ($(awk '{print $1}' build/repro_digest.txt))"
# Honest boundary: this is same-machine, same-compiler reproducibility. It
# proves the build embeds no timestamp, path, or nondeterministic ordering.
# Cross-toolchain and cross-machine reproducibility is a separate claim and
# is NOT made here.
echo "execution vector export ok records=${EXEC_VECTORS} commit=19 hold=8 nest=10 fail=0"

echo
echo "== Lifecycle contract: budgets, cancellation, determinism [gate CT3] =="
# An executor that cannot be bounded or stopped is not embeddable. Both are
# enforced at EFFECT BOUNDARIES — between whole effects, never inside one —
# which is the entire guarantee: an effect either runs completely or does
# not begin.
#
# A stop is a HOLD, not a failure: `budget-exhausted` and `cancelled` are
# typed reasons in the same vocabulary as `balance-violation`, and exit
# code 4 distinguishes a lifecycle stop from a violated expectation (1).
rm -rf build/persistence-journal build/persistence-contended
set +e
CDC_MAX_EFFECTS=3 ./build/cdc run framework_persistence.cdc \
  > build/lifecycle_budget.txt 2>&1
BUDGET_RC=$?
set -e
test "$BUDGET_RC" = "4"
grep -q "lifecycle=stop kind=persist job=journal-replay status=held reason=budget-exhausted effects=3" \
  build/lifecycle_budget.txt
echo "budget stop ok (exit 4, typed hold, 3 effects)"

# The load-bearing property: at EVERY stop point the store is intact. Not
# "recoverable" — intact: recovered=0 means no torn tail needed truncating,
# and the replay identity is either the pre-effect or the post-effect value,
# never something in between.
rm -rf build/lifecycle_states.txt
for STOP in 1 2 3 5 8; do
  rm -rf build/persistence-journal build/persistence-contended
  set +e
  CDC_CANCEL_AFTER="$STOP" ./build/cdc run framework_persistence.cdc \
    > "build/lifecycle_cancel_${STOP}.txt" 2>&1
  CANCEL_RC=$?
  set -e
  test "$CANCEL_RC" = "4"
  grep -q "status=held reason=cancelled" "build/lifecycle_cancel_${STOP}.txt"
  ./build/cdc_frontend_check store-inspect build/persistence-journal \
    >> build/lifecycle_states.txt
done
# Every stop left an openable, verifying store that needed no recovery.
test "$(grep -c "open=ok" build/lifecycle_states.txt)" = "5"
test "$(grep -c "recovered=0" build/lifecycle_states.txt)" = "5"
test "$(grep -c "verify=ok" build/lifecycle_states.txt)" = "5"
# And exactly two distinct replay identities across all stop points: the
# state before the single durable append, and the state after it. A third
# value would mean an effect was observed half-applied.
DISTINCT=$(sed 's/.*replay=//' build/lifecycle_states.txt | sort -u | wc -l)
test "$DISTINCT" = "2"
echo "cancellation ok (5 stop points, store intact at each, ${DISTINCT} replay identities: pre/post, none between)"

# Determinism: the same source produces byte-identical prose, receipts, and
# vectors across runs.
for ROUND in a b; do
  rm -rf build/persistence-journal build/persistence-contended
  # shellcheck disable=SC2086
  CDC_DETERMINISTIC=1 ./build/cdc test --gate --vectors "build/det_vec_${ROUND}.txt" \
    $DET_FILES > "build/det_out_${ROUND}.txt" 2>&1
  rm -rf build/persistence-journal build/persistence-contended
  CDC_DETERMINISTIC=1 CDC_RECEIPTS="build/det_rec_${ROUND}.txt" \
    ./build/cdc run framework_persistence.cdc > "build/det_prose_${ROUND}.txt"
done
cmp build/det_out_a.txt build/det_out_b.txt
cmp build/det_vec_a.txt build/det_vec_b.txt
cmp build/det_rec_a.txt build/det_rec_b.txt
cmp build/det_prose_a.txt build/det_prose_b.txt
# Honest boundary, gated rather than assumed: the store's INSTANCE identity
# (its uuid) is deliberately NOT reproducible. Making it reproducible would
# defeat the base-substitution defence, which relies on two stores with
# identical histories being distinguishable. So determinism covers the
# observable outputs — prose, receipts, vectors — and the attestation over a
# store instance is excluded by design, not by oversight.
rm -rf build/det_store_a build/det_store_b
mkdir -p build/det_store_a build/det_store_b
./build/cdc_frontend_check store-inspect build/det_store_a > build/det_id_a.txt
./build/cdc_frontend_check store-inspect build/det_store_b > build/det_id_b.txt
# identical (empty) histories, so replay identity MUST match ...
test "$(sed 's/.*replay=//' build/det_id_a.txt)" = "$(sed 's/.*replay=//' build/det_id_b.txt)"
echo "determinism ok (prose, receipts, and vectors byte-identical across runs;"
echo "  store instance identity excluded by design — see DECISIONS D19)"

# The persistence path owns store handles across a whole source file
# (three handles, two of them onto one directory), so it gets its own
# instrumented pass rather than riding on the frontend's.
if [ "$SANITIZED" = "1" ]; then
  run_step cc -std=c99 -Wall -Wextra -pedantic -O1 \
    -fsanitize=address,undefined \
    runtime/cdc_native_runtime.c \
    runtime/cdc_source.c \
    runtime/cdc_store.c \
    runtime/cdc_digest.c \
    runtime/cdc_blake3.c \
    runtime/cdc_receipt.c \
    runtime/cdc_parser.c \
    runtime/cdc_ast.c \
    runtime/cdc_lexer.c \
    runtime/cdc_diagnostic.c \
    -lm \
    -o build/cdc_persist_asan
  rm -rf build/persistence-journal build/persistence-contended \
    build/persistence-bytes
  run_step ./build/cdc_persist_asan persist framework_persistence.cdc
  run_step ./build/cdc_persist_asan persist \
    tests/fixtures/persistence/latch_seed.cdc
  run_step ./build/cdc_persist_asan persist \
    tests/fixtures/persistence/hold_writes_nothing.cdc
fi

echo
echo "== ABI boundary counterexamples [2026-07-23 adversarial review] =="
# Defect 1: rejected source under a >512-byte path must serialize complete
# JSON (two-pass render; no fixed-slot overflow).
LONG_SEG=$(printf 'x%.0s' $(seq 1 100))
LONG_DIR="build/longpath/${LONG_SEG}/${LONG_SEG}/${LONG_SEG}/${LONG_SEG}/${LONG_SEG}"
mkdir -p "$LONG_DIR"
cp tests/fixtures/frontend/unknown_directive.cdc "${LONG_DIR}/rejected.cdc"
LONG_PATH="${LONG_DIR}/rejected.cdc"
test "${#LONG_PATH}" -gt 512
./build/cdc_frontend_check abi-diag "$LONG_PATH" > build/abi_diag_long.txt
grep -qF "$LONG_PATH" build/abi_diag_long.txt
grep -q "abi-diag ok" build/abi_diag_long.txt
echo "abi long-path diagnostic ok (path ${#LONG_PATH} bytes)"
if ./build/cdc verify --parse "$LONG_PATH" >/dev/null 2>&1; then
  echo "cdc verify accepted the long-path rejected fixture" >&2
  exit 1
fi
# Defect 2: unreadable and special paths are CDC_ERR_IO, never empty
# accepted programs.
if ./build/cdc verify --parse . >/dev/null 2>&1; then
  echo "cdc verify accepted a directory as an empty program" >&2
  exit 1
fi
run_step ./build/cdc_frontend_check abi-io . io
run_step ./build/cdc_frontend_check abi-io /dev/null io
rm -f build/test_fifo
mkfifo build/test_fifo
timeout 10 ./build/cdc_frontend_check abi-io build/test_fifo io
rm -f build/test_fifo
run_step ./build/cdc_frontend_check io-mid-read build
if [ "$(id -u)" != "0" ]; then
  cp tests/fixtures/frontend/unknown_directive.cdc build/noread.cdc
  chmod 000 build/noread.cdc
  run_step ./build/cdc_frontend_check abi-io build/noread.cdc io
  chmod 644 build/noread.cdc
  rm -f build/noread.cdc
else
  echo "running as root; unreadable-file case skipped (permissions moot)"
fi
: > build/empty_unit.cdc
./build/cdc verify --parse build/empty_unit.cdc | \
  grep -q "cdc verify parse ok files=1 statements=0"
echo "zero-byte regular source ok (valid empty unit)"
run_step ./build/cdc_frontend_check oom-abi tests/fixtures/frontend/unknown_directive.cdc
if [ "$SANITIZED" = "1" ]; then
  run_step ./build/cdc_frontend_check_asan abi-diag "$LONG_PATH" > /dev/null
  run_step ./build/cdc_frontend_check_asan oom-abi tests/fixtures/frontend/unknown_directive.cdc
  run_step ./build/cdc_frontend_check_asan abi-io . io
fi

echo
echo "== Operational bridge runtime =="
command -v cc >/dev/null 2>&1 || {
  echo "cc is required for operational bridge verification" >&2
  exit 1
}
rm -f build/cdc_bridge_runtime
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  runtime/cdc_bridge_runtime.c \
  runtime/cdc_source.c \
  runtime/cdc_parser.c \
  runtime/cdc_ast.c \
  runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c \
  -o build/cdc_bridge_runtime
run_step build/cdc_bridge_runtime verify bridge64.cdc

dyadic_lookup="$(build/cdc_bridge_runtime lookup-dyadic bridge64.cdc 101011)"
case "$dyadic_lookup" in
  *"index=43"*"triadic=223"*) echo "$dyadic_lookup" ;;
  *) echo "unexpected dyadic lookup: $dyadic_lookup" >&2; exit 1 ;;
esac

triadic_lookup="$(build/cdc_bridge_runtime lookup-triadic bridge64.cdc 223)"
case "$triadic_lookup" in
  *"index=43"*"dyadic=101011"*) echo "$triadic_lookup" ;;
  *) echo "unexpected triadic lookup: $triadic_lookup" >&2; exit 1 ;;
esac

trace_projection="$(build/cdc_bridge_runtime project-trits bridge64.cdc '+0-+0-' council)"
case "$trace_projection" in
  *"occupancy=101101"*"index=45"*"triadic=231"*) echo "$trace_projection" ;;
  *) echo "unexpected trace projection: $trace_projection" >&2; exit 1 ;;
esac

run_step build/cdc_bridge_runtime run-jobs bridge64.cdc bridge_jobs.cdc
run_step build/cdc_bridge_runtime codebook 9
run_step build/cdc_bridge_runtime codebook 12
run_step build/cdc_bridge_runtime verify-codebook bridge512.cdc 9
run_step build/cdc_bridge_runtime verify-codebook bridge4096.cdc 12
echo "+ build/cdc_bridge_runtime emit-codebook 9 > build/bridge512.cdc"
build/cdc_bridge_runtime emit-codebook 9 > build/bridge512.cdc
echo "+ build/cdc_bridge_runtime emit-codebook 12 > build/bridge4096.cdc"
build/cdc_bridge_runtime emit-codebook 12 > build/bridge4096.cdc
assert_fresh_file \
  build/bridge512.cdc \
  bridge512.cdc \
  "build/cdc_bridge_runtime emit-codebook 9 > bridge512.cdc"
assert_fresh_file \
  build/bridge4096.cdc \
  bridge4096.cdc \
  "build/cdc_bridge_runtime emit-codebook 12 > bridge4096.cdc"
echo "+ build/cdc_bridge_runtime grid bridge64.cdc > build/bridge64-grid.txt"
build/cdc_bridge_runtime grid bridge64.cdc > build/bridge64-grid.txt
echo "+ build/cdc_bridge_runtime grid-svg bridge64.cdc > build/bridge64-grid.svg"
build/cdc_bridge_runtime grid-svg bridge64.cdc > build/bridge64-grid.svg
test -s build/bridge64-grid.txt
grep -q "class=\"bridge-cell\"" build/bridge64-grid.svg
grep -q "function selectCell" build/bridge64-grid.svg
assert_fresh_file \
  build/bridge64-grid.svg \
  assets/bridge64-grid.svg \
  "build/cdc_bridge_runtime grid-svg bridge64.cdc > assets/bridge64-grid.svg"

echo
echo "== Whole-binary sanitizer sweep [gate CT2/CT3] =="
# Until now only the frontend, the persistence path, and the receipt carrier
# were instrumented. The binary that actually SHIPS — the unified `cdc`
# driver with every runtime linked in — was not. This builds that exact
# composition under ASan/UBSan and runs the real command surface through it.
if [ "$SANITIZED" = "1" ]; then
  run_step cc -std=c99 -Wall -Wextra -pedantic -O1 \
    -fsanitize=address,undefined \
    runtime/toolchain/main.c \
    runtime/toolchain/cmd_verify.c \
    runtime/toolchain/cmd_test.c \
    runtime/cdc_abi.c \
    runtime/cdc_registry.c \
    runtime/cdc_parser.c \
    runtime/cdc_ast.c \
    runtime/cdc_lexer.c \
    runtime/cdc_diagnostic.c \
    -DCDC_NATIVE_NO_MAIN -DCDC_BRIDGE_NO_MAIN \
    runtime/cdc_native_runtime.c \
    runtime/cdc_bridge_runtime.c \
    runtime/cdc_source.c \
    runtime/cdc_receipt.c \
    runtime/cdc_store.c \
    runtime/cdc_digest.c \
    runtime/cdc_blake3.c \
    -lm \
    -o build/cdc_asan
  run_step ./build/cdc_asan version
  # shellcheck disable=SC2086
  run_step ./build/cdc_asan verify --parse $CDC_ROOT_SOURCES
  # shellcheck disable=SC2086
  ./build/cdc_asan verify --contract $CDC_ROOT_SOURCES > build/contract_asan.txt
  cmp build/contract_boot.txt build/contract_asan.txt
  # shellcheck disable=SC2086
  ./build/cdc_asan verify --vectors $CDC_ROOT_SOURCES > build/vectors_asan.txt
  cmp build/vectors_native.txt build/vectors_asan.txt
  rm -rf build/persistence-journal build/persistence-contended
  run_step ./build/cdc_asan run framework_persistence.cdc
  run_step ./build/cdc_asan run framework_loop.cdc
  run_step ./build/cdc_asan run council_bridge.cdc
  run_step ./build/cdc_asan bridge verify bridge64.cdc
  rm -rf build/persistence-journal build/persistence-contended
  # shellcheck disable=SC2086
  ./build/cdc_asan test --gate --vectors build/vectors_exec_asan.txt \
    $DET_FILES > build/cdc_test_asan.txt
  grep -q "cdc test ok runs=24 commit=19 hold=8 (expected=8 unexpected=0) nest=10 fail=0 parity=0 corpus=blake3:" \
    build/cdc_test_asan.txt
  cmp build/test_vectors.txt build/vectors_exec_asan.txt
  # The instrumented binary must agree with the plain one, not merely avoid
  # crashing: identical contract report, identical vectors, identical gate.
  echo "whole-binary sanitizer sweep ok (verify/run/test/bridge under ASan+UBSan,"
  echo "  contract + vectors + gate byte-identical to the plain build)"
else
  echo "sanitizers unavailable; skipping whole-binary sweep"
fi

echo
echo "== Native reducer runtime =="
rm -f build/cdc_native_runtime
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  runtime/cdc_native_runtime.c \
  runtime/cdc_source.c \
  runtime/cdc_store.c \
  runtime/cdc_digest.c \
  runtime/cdc_blake3.c \
  runtime/cdc_receipt.c \
  runtime/cdc_parser.c \
  runtime/cdc_ast.c \
  runtime/cdc_lexer.c \
  runtime/cdc_diagnostic.c \
  -o build/cdc_native_runtime \
  -lm
echo
echo "== Native WASM replay export surface =="
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 -Wno-unused-function \
  -c runtime/cdc_wasm_exports.c \
  -o build/cdc_wasm_exports.o
run_step cc -std=c99 -Wall -Wextra -pedantic -O2 \
  -c runtime/cdc_source.c \
  -o build/cdc_source.o
if command -v emcc >/dev/null 2>&1; then
  run_step emcc -O2 runtime/cdc_wasm_exports.c runtime/cdc_source.c \
    runtime/cdc_store.c runtime/cdc_digest.c runtime/cdc_blake3.c \
    runtime/cdc_receipt.c runtime/cdc_parser.c runtime/cdc_ast.c \
    runtime/cdc_lexer.c runtime/cdc_diagnostic.c \
    runtime/cdc_receipt.c \
    -sEXPORTED_FUNCTIONS='["_cdc_wasm_replay_json"]' \
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -o build/cdc_wasm_replay.js
  test -s build/cdc_wasm_replay.js
  test -s build/cdc_wasm_replay.wasm
else
  echo "emcc not found; skipping live WASM link"
fi
# End-to-end attribute-shadowing counterexample [deletion-gate step 1]:
# `action-gain` shadows `gain` on a field, and the nest integration that
# consumes gain is expectation-pinned to the correct value. Under the old
# substring reader this fails with a belief nine times too large, while the
# real corpus stays green either way — which is why the fixture must exist.
run_step ./build/cdc_native_runtime run \
  tests/fixtures/frontend_boundary/shadowed_gain.cdc

native_reducer="$(build/cdc_native_runtime run native_reducer.cdc)"
echo "$native_reducer"
grep -q "flow=reducer-flow .*theta council.b=0.250000" <<<"$native_reducer" || {
  echo "native reducer flow check failed" >&2
  exit 1
}
grep -q "commit=reducer-commit .*trits=0+- .*balance=admissible .*status=accepted .*reason=none" <<<"$native_reducer" || {
  echo "native reducer commit check failed" >&2
  exit 1
}
grep -q "commit=reducer-hold .*trits=-+0 .*balance=violated .*status=held .*reason=balance-violation" <<<"$native_reducer" || {
  echo "native reducer held-commit check failed" >&2
  exit 1
}
grep -q "nest=reducer-nest .*parent-belief=0.666667 .*child-prior=0.666667" <<<"$native_reducer" || {
  echo "native reducer nest check failed" >&2
  exit 1
}
grep -q "native reducer ok steps=4 flow=1 commit=2 nest=1" <<<"$native_reducer" || {
  echo "native reducer summary check failed" >&2
  exit 1
}
native_compile="$(build/cdc_native_runtime compile native_reducer.cdc)"
echo "$native_compile"
grep -q "native compile ok jobs=1 ops=4" <<<"$native_compile" || {
  echo "native compile check failed" >&2
  exit 1
}
native_interpret="$(build/cdc_native_runtime interpret native_reducer.cdc)"
echo "$native_interpret"
grep -q "ir-interpreter source=native_reducer.cdc ops=4" <<<"$native_interpret" || {
  echo "native IR interpreter did not compile IR" >&2
  exit 1
}
grep -q "native interpret ok ops=4 flow=1 commit=2 nest=1" <<<"$native_interpret" || {
  echo "native IR interpreter check failed" >&2
  exit 1
}
native_proof="$(build/cdc_native_runtime prove native_reducer.cdc)"
echo "$native_proof"
grep -q "proof=trit-walk-n6 .*total=729 .*admissible=267 .*localized=51 .*saturated=20 .*catalan=5" <<<"$native_proof" || {
  echo "native proof finite trit-walk check failed" >&2
  exit 1
}
grep -q "native proof ok jobs=1" <<<"$native_proof" || {
  echo "native proof summary check failed" >&2
  exit 1
}
native_surface="$(build/cdc_native_runtime surface native_surface.cdc)"
echo "$native_surface"
grep -q "guard=surface-guard .*state=open" <<<"$native_surface" || {
  echo "native surface guard check failed" >&2
  exit 1
}
grep -q "trace=surface-trace .*trits=+0-+0- .*events=4" <<<"$native_surface" || {
  echo "native surface trace check failed" >&2
  exit 1
}
grep -q "measure=surface-measure .*outcome=+0- .*potential=nonincrease" <<<"$native_surface" || {
  echo "native surface measurement check failed" >&2
  exit 1
}
grep -q "policy=surface-policy .*sampling=local .*commit=guarded .*adapt=recursive" <<<"$native_surface" || {
  echo "native surface policy check failed" >&2
  exit 1
}
grep -q "bridge=surface-bridge .*dyadic=101101 .*triadic=231" <<<"$native_surface" || {
  echo "native surface bridge check failed" >&2
  exit 1
}
grep -q "counter=surface-counter .*final=4" <<<"$native_surface" || {
  echo "native surface counter check failed" >&2
  exit 1
}
grep -q "native surface ok guards=1 traces=1 measures=1 policies=1 bridges=1 counters=1" <<<"$native_surface" || {
  echo "native surface summary check failed" >&2
  exit 1
}
native_replay="$(build/cdc_native_runtime replay native_reducer.cdc native_surface.cdc framework_loop.cdc)"
echo "$native_replay"
echo "$native_replay" > build/demo-replay.json
assert_fresh_file \
  build/demo-replay.json \
  demo/replay.json \
  "build/cdc_native_runtime replay native_reducer.cdc native_surface.cdc framework_loop.cdc > demo/replay.json"

echo
echo "== Runtime replay demo contract =="
export NATIVE_REDUCER_OUTPUT="$native_reducer"
export NATIVE_SURFACE_OUTPUT="$native_surface"
export TRACE_PROJECTION_OUTPUT="$trace_projection"
export NATIVE_REPLAY_OUTPUT="$native_replay"
python3 - <<'PY'
import json
import os
import re
from pathlib import Path


def match(pattern: str, text: str, label: str) -> tuple[str, ...]:
    found = re.search(pattern, text)
    if not found:
        raise SystemExit(f"demo replay could not read {label}")
    return found.groups()


html = Path("demo/index.html").read_text(encoding="utf-8")
payload = re.search(
    r'<script type="application/json" id="replay-data">\s*(.*?)\s*</script>',
    html,
    re.S,
)
if not payload:
    raise SystemExit("demo replay data block missing")
replay = json.loads(payload.group(1))
native_replay = json.loads(os.environ["NATIVE_REPLAY_OUTPUT"])
if replay != native_replay:
    raise SystemExit("demo replay data does not match native replay JSON")

native = os.environ["NATIVE_REDUCER_OUTPUT"]
surface = os.environ["NATIVE_SURFACE_OUTPUT"]
projection = os.environ["TRACE_PROJECTION_OUTPUT"]

(theta_raw,) = match(r"theta council\.b=([0-9.]+)", native, "flow theta")
accepted = match(
    r"commit=reducer-commit .*trits=([^ ]+) .*balance=([^ ]+) .*status=([^ ]+) .*reason=([^\n]+)",
    native,
    "accepted commit",
)
held = match(
    r"commit=reducer-hold .*trits=([^ ]+) .*balance=([^ ]+) .*status=([^ ]+) .*reason=([^\n]+)",
    native,
    "held commit",
)
nest = match(
    r"nest=reducer-nest .*up=([0-9.]+) .*parent-belief=([0-9.]+) .*child-prior=([0-9.]+)",
    native,
    "nest transfer",
)
trace = match(r"trace=surface-trace .*trits=([^ ]+) .*events=([0-9]+)", surface, "surface trace")
surface_bridge = match(
    r"bridge=surface-bridge .*dyadic=([^ ]+) .*triadic=([^\n]+)",
    surface,
    "surface bridge",
)
projected_bridge = match(
    r"occupancy=([^ ]+) index=([^ ]+) triadic=([^ ]+)",
    projection,
    "projected bridge",
)

universal = replay.get("universal")
if not universal:
    raise SystemExit("demo replay is missing the universal operator block")
if universal["operator"] != "U720" or universal["status"] != "accepted":
    raise SystemExit("demo replay universal block is not an accepted U720 result")
if not (universal["recordCoordinate"] == universal["decisionCoordinate"] == universal["enactedCoordinate"]):
    raise SystemExit("demo replay universal coordinates do not agree")

checks = [
    (replay["flow"]["thetaCouncilBRaw"], theta_raw, "flow raw theta"),
    (replay["flow"]["thetaCouncilB"], str(float(theta_raw)), "flow display theta"),
    (tuple(replay["commit"][k] for k in ("trits", "balance", "status", "reason")), accepted, "accepted commit"),
    (tuple(replay["hold"][k] for k in ("trits", "balance", "status", "reason")), held, "held commit"),
    (tuple(replay["nest"][k] for k in ("up", "parentBelief", "childPrior")), nest, "nest values"),
    ((replay["trace"]["trits"], replay["trace"]["events"]), trace, "trace values"),
    ((replay["bridge"]["dyadic"], replay["bridge"]["triadic"]), surface_bridge, "surface bridge values"),
    ((replay["bridge"]["dyadic"], replay["bridge"]["index"], replay["bridge"]["triadic"]), projected_bridge, "projected bridge values"),
]
for got, want, label in checks:
    if got != want:
        raise SystemExit(f"demo replay mismatch for {label}: {got!r} != {want!r}")

print("runtime replay demo: ok")
PY
council_output="$(build/cdc_native_runtime council council_bridge.cdc)"
echo "$council_output"
grep -q "council=bridge-council .*dyadic=101101 .*triadic=231 .*decision=adopt" <<<"$council_output" || {
  echo "native council deliberation check failed" >&2
  exit 1
}
grep -q "native council ok deliberations=1" <<<"$council_output" || {
  echo "native council summary check failed" >&2
  exit 1
}
self_evolution="$(build/cdc_native_runtime evolve council_bridge.cdc)"
echo "$self_evolution"
grep -q "evolution=bridge-coordinate-evolution coordinate=101101" <<<"$self_evolution" || {
  echo "native self-evolution check failed" >&2
  exit 1
}
grep -q "native evolution ok jobs=1" <<<"$self_evolution" || {
  echo "native self-evolution summary check failed" >&2
  exit 1
}
grep -q "self-evolution-bridge" build/evolved_native_reducer.cdc || {
  echo "evolved .cdc output is missing the bridge-written witness" >&2
  exit 1
}

echo
echo "== Native task frameworks =="
transition_run="$(build/cdc_native_runtime run framework_transition.cdc)"
echo "$transition_run"
grep -q "flow=transition-action .*theta fsm.s1=0.250000" <<<"$transition_run" || {
  echo "transition framework action check failed" >&2
  exit 1
}
grep -q "commit=transition-fire .*trits=+0- .*balance=admissible .*status=accepted .*reason=none" <<<"$transition_run" || {
  echo "transition framework fired-transition check failed" >&2
  exit 1
}
grep -q "commit=transition-blocked .*trits=-+0 .*balance=violated .*status=held .*reason=balance-violation" <<<"$transition_run" || {
  echo "transition framework blocked-transition check failed" >&2
  exit 1
}
grep -q "nest=transition-lift .*parent-belief=0.666667 .*child-prior=0.666667" <<<"$transition_run" || {
  echo "transition framework hierarchy check failed" >&2
  exit 1
}
transition_surface="$(build/cdc_native_runtime surface framework_transition.cdc)"
echo "$transition_surface"
grep -q "guard=transition-guard .*state=open" <<<"$transition_surface" || {
  echo "transition framework precondition check failed" >&2
  exit 1
}
grep -q "bridge=transition-bridge .*dyadic=101110 .*triadic=232" <<<"$transition_surface" || {
  echo "transition framework state-key check failed" >&2
  exit 1
}
grep -q "counter=transition-counter .*final=1" <<<"$transition_surface" || {
  echo "transition framework tally check failed" >&2
  exit 1
}
procedural_run="$(build/cdc_native_runtime run framework_procedural.cdc)"
echo "$procedural_run"
grep -q "commit=procedural-execute .*trits=0+- .*balance=admissible .*status=accepted .*reason=none" <<<"$procedural_run" || {
  echo "procedural framework step check failed" >&2
  exit 1
}
grep -q "commit=procedural-retry .*trits=-+0 .*balance=violated .*status=held .*reason=balance-violation" <<<"$procedural_run" || {
  echo "procedural framework retry check failed" >&2
  exit 1
}
grep -q "nest=procedural-consolidate .*parent-belief=0.666667 .*child-prior=0.666667" <<<"$procedural_run" || {
  echo "procedural framework consolidation check failed" >&2
  exit 1
}
procedural_compile="$(build/cdc_native_runtime compile framework_procedural.cdc)"
echo "$procedural_compile"
grep -q "native compile ok jobs=1 ops=4 source=framework_procedural.cdc" <<<"$procedural_compile" || {
  echo "procedural framework proceduralization check failed" >&2
  exit 1
}
procedural_interpret="$(build/cdc_native_runtime interpret framework_procedural.cdc)"
echo "$procedural_interpret"
grep -q "native interpret ok ops=4 flow=1 commit=2 nest=1 source=framework_procedural.cdc" <<<"$procedural_interpret" || {
  echo "procedural framework skilled-execution check failed" >&2
  exit 1
}
episodic_run="$(build/cdc_native_runtime run framework_episodic.cdc)"
echo "$episodic_run"
grep -q "commit=episodic-record .*trits=++0 .*balance=admissible .*status=accepted .*reason=none" <<<"$episodic_run" || {
  echo "episodic framework record check failed" >&2
  exit 1
}
grep -q "nest=episodic-consolidate .*parent-belief=0.666667 .*child-prior=0.666667" <<<"$episodic_run" || {
  echo "episodic framework consolidation check failed" >&2
  exit 1
}
episodic_surface="$(build/cdc_native_runtime surface framework_episodic.cdc)"
echo "$episodic_surface"
grep -q "trace=episodic-trace .*trits=++00-+ .*events=4" <<<"$episodic_surface" || {
  echo "episodic framework content check failed" >&2
  exit 1
}
grep -q "bridge=episodic-key .*dyadic=110011 .*triadic=303" <<<"$episodic_surface" || {
  echo "episodic framework key check failed" >&2
  exit 1
}
grep -q "counter=episodic-ordinal .*final=1" <<<"$episodic_surface" || {
  echo "episodic framework ordinal check failed" >&2
  exit 1
}

episodic_recall_by_content="$(build/cdc_bridge_runtime lookup-dyadic bridge64.cdc 110011)"
case "$episodic_recall_by_content" in
  *"index=51"*"triadic=303"*) echo "$episodic_recall_by_content" ;;
  *) echo "episodic recall by content failed: $episodic_recall_by_content" >&2; exit 1 ;;
esac
episodic_recall_by_key="$(build/cdc_bridge_runtime lookup-triadic bridge64.cdc 303)"
case "$episodic_recall_by_key" in
  *"index=51"*"dyadic=110011"*) echo "$episodic_recall_by_key" ;;
  *) echo "episodic recall by key failed: $episodic_recall_by_key" >&2; exit 1 ;;
esac
deliberative_council="$(build/cdc_native_runtime council framework_deliberative.cdc)"
echo "$deliberative_council"
grep -q "council=deliberative-council .*dyadic=111101 .*triadic=331 .*occupancy=5 .*quorum=4 .*decision=adopt" <<<"$deliberative_council" || {
  echo "deliberative framework quorum check failed" >&2
  exit 1
}
deliberative_enactment="$(build/cdc_native_runtime evolve framework_deliberative.cdc)"
echo "$deliberative_enactment"
grep -q "evolution=deliberative-enactment coordinate=111101" <<<"$deliberative_enactment" || {
  echo "deliberative framework enactment check failed" >&2
  exit 1
}
grep -q "^witness deliberative-decision-memory invariant=dyadic-triadic-closure coordinate=111101" build/enacted_decision.cdc || {
  echo "enacted decision output is missing the appended decision-memory witness" >&2
  exit 1
}

echo
echo "== Native task-loop composition =="
loop_run="$(build/cdc_native_runtime run framework_loop.cdc)"
echo "$loop_run"
grep -q "flow=loop-sense-1 .*theta agent.b=0.250000" <<<"$loop_run" || {
  echo "loop cycle-1 sense check failed" >&2
  exit 1
}
grep -q "nest=loop-integrate-1 .*parent-belief=0.666667" <<<"$loop_run" || {
  echo "loop cycle-1 integration check failed" >&2
  exit 1
}
grep -q "flow=loop-sense-2 .*theta agent.b=0.492228" <<<"$loop_run" || {
  echo "loop cycle-2 carried-state sense check failed" >&2
  exit 1
}
grep -q "nest=loop-integrate-2 .*parent-belief=1.333333 .*child-prior=1.333333" <<<"$loop_run" || {
  echo "loop cycle-2 cumulative integration check failed" >&2
  exit 1
}
grep -q "flow=loop-turn-half .*theta loop-cover.phase=6.283185" <<<"$loop_run" || {
  echo "loop half-turn cover check failed" >&2
  exit 1
}
grep -q "flow=loop-turn-full .*theta loop-cover.phase=12.566371" <<<"$loop_run" || {
  echo "loop full-turn cover check failed" >&2
  exit 1
}
grep -q "native reducer ok steps=8 flow=4 commit=2 nest=2 source=framework_loop.cdc" <<<"$loop_run" || {
  echo "loop two-cycle summary check failed" >&2
  exit 1
}
loop_interpret="$(build/cdc_native_runtime interpret framework_loop.cdc)"
grep -q "native interpret ok ops=8 flow=4 commit=2 nest=2 source=framework_loop.cdc" <<<"$loop_interpret" || {
  echo "loop skilled-execution check failed" >&2
  exit 1
}
loop_compile="$(build/cdc_native_runtime compile framework_loop.cdc)"
grep -q "native compile ok jobs=1 ops=8 source=framework_loop.cdc" <<<"$loop_compile" || {
  echo "loop proceduralization check failed" >&2
  exit 1
}
loop_surface="$(build/cdc_native_runtime surface framework_loop.cdc)"
echo "$loop_surface"
grep -q "bridge=loop-key .*dyadic=110101 .*triadic=311" <<<"$loop_surface" || {
  echo "loop key check failed" >&2
  exit 1
}
grep -q "counter=loop-cycle .*final=2" <<<"$loop_surface" || {
  echo "loop cycle-count check failed" >&2
  exit 1
}
loop_council="$(build/cdc_native_runtime council framework_loop.cdc)"
echo "$loop_council"
grep -q "council=loop-council .*dyadic=110101 .*triadic=311 .*occupancy=4 .*quorum=4 .*decision=adopt" <<<"$loop_council" || {
  echo "loop decision check failed" >&2
  exit 1
}
loop_enactment="$(build/cdc_native_runtime evolve framework_loop.cdc)"
echo "$loop_enactment"
grep -q "^witness loop-decision-memory invariant=dyadic-triadic-closure coordinate=110101" build/enacted_loop.cdc || {
  echo "enacted loop output is missing the appended decision-memory witness" >&2
  exit 1
}
loop_recall="$(build/cdc_bridge_runtime lookup-dyadic bridge64.cdc 110101)"
case "$loop_recall" in
  *"index=53"*"triadic=311"*) echo "$loop_recall" ;;
  *) echo "loop recorded-coordinate recall failed: $loop_recall" >&2; exit 1 ;;
esac

echo
echo "== Universal operator closure =="
universal_run="$(build/cdc_native_runtime universal framework_loop.cdc)"
echo "$universal_run"
grep -q "universal=loop-u720 .*holonomy=0.125000 .*half-projection=returned .*half-sheet=inverted .*full-projection=returned .*full-sheet=restored .*winding=2 .*record=110101 .*decision=110101 .*enacted=110101 .*status=accepted .*reason=none" <<<"$universal_run" || {
  echo "universal closure acceptance check failed" >&2
  exit 1
}
grep -q "universal-parity ops=8 ok" <<<"$universal_run" || {
  echo "universal interpreter parity check failed" >&2
  exit 1
}
grep -q "^witness loop-decision-memory invariant=universal-closure coordinate=110101 winding=2 sheet=restored holonomy=0.125000 status=accepted" build/enacted_loop.cdc || {
  echo "universal enactment is missing the appended universal-closure witness" >&2
  exit 1
}

sed -e '/^universal /s/full-step=loop-turn-full/full-step=loop-turn-half/' \
    -e '/^universal /s/expect-status=accepted expect-reason=none/expect-status=held expect-reason=full-sheet-mismatch/' \
    framework_loop.cdc > build/fixture_360_only.cdc
universal_360="$(build/cdc_native_runtime universal build/fixture_360_only.cdc)"
echo "$universal_360"
grep -q "universal=loop-u720 .*winding=1 .*status=held .*reason=full-sheet-mismatch" <<<"$universal_360" || {
  echo "universal 360-only fixture did not hold with full-sheet-mismatch" >&2
  exit 1
}

sed -e 's/^channel agent.a -> context.b id=loop-radiant/channel agent.a -> context.c id=loop-radiant/' \
    -e '/^universal /s/expect-status=accepted expect-reason=none/expect-status=held expect-reason=cone-not-reciprocal/' \
    framework_loop.cdc > build/fixture_nonreciprocal.cdc
universal_cone="$(build/cdc_native_runtime universal build/fixture_nonreciprocal.cdc)"
echo "$universal_cone"
grep -q "universal=loop-u720 .*status=held .*reason=cone-not-reciprocal" <<<"$universal_cone" || {
  echo "universal nonreciprocal fixture did not hold with cone-not-reciprocal" >&2
  exit 1
}

rm -f build/universal_mismatch_output.cdc
sed -e '/^council loop-council/s/members=agent,context/members=context,agent/' \
    -e '/^council loop-council/s/expect-dyadic=110101/expect-dyadic=101110/' \
    -e '/^council loop-council/s/expect-triadic=311/expect-triadic=232/' \
    -e '/^evolve loop-enact/s|output=build/enacted_loop.cdc|output=build/universal_mismatch_output.cdc|' \
    -e '/^universal /s/expect-status=accepted expect-reason=none/expect-status=held expect-reason=coordinate-mismatch/' \
    framework_loop.cdc > build/fixture_mismatch.cdc
universal_mismatch="$(build/cdc_native_runtime universal build/fixture_mismatch.cdc)"
echo "$universal_mismatch"
grep -q "universal=loop-u720 .*record=110101 .*decision=101110 .*status=held .*reason=coordinate-mismatch" <<<"$universal_mismatch" || {
  echo "universal mismatch fixture did not hold with coordinate-mismatch" >&2
  exit 1
}
if [ -e build/universal_mismatch_output.cdc ]; then
  echo "universal mismatch fixture must not create evolved output" >&2
  exit 1
fi

echo
echo "== Lean/Coq finite carrier and algebraic proofs =="
if require_or_skip lean "Lean finite carrier/algebra proof check"; then
  run_step lean formal/lean/CDCFinite.lean
  echo "lean finite carrier/algebra proof: ok"
fi

if require_or_skip coqc "Coq/Rocq finite carrier/algebra proof check"; then
  run_step coqc -q formal/coq/CDCFinite.v
  rm -f formal/coq/CDCFinite.vo formal/coq/CDCFinite.vos formal/coq/CDCFinite.vok formal/coq/CDCFinite.glob formal/coq/.CDCFinite.aux
  echo "coq finite carrier/algebra proof: ok"
fi

echo
echo "== Paper compile =="
if require_or_skip tectonic "paper compile"; then
  (cd paper/arxiv && run_step tectonic main.tex)
fi

echo
echo "All checks passed."

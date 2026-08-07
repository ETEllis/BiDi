#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

SKIP_FORMAL=0

usage() {
  cat <<'EOF'
Usage: ./scripts/verify_u2.sh [--skip-formal]

  --skip-formal  Run executable, spectral, receipt, and mutant gates only.
                 The repository-wide verifier applies its own formal-tool
                 availability policy afterward.
EOF
}

while (($#)); do
  case "$1" in
    --skip-formal)
      SKIP_FORMAL=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown U2 verify option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
  shift
done

on_error() {
  local status=$?
  local line=${BASH_LINENO[0]:-${LINENO}}
  echo "U2 verify failed at line ${line}: ${BASH_COMMAND} (exit ${status})" >&2
}
trap on_error ERR

mkdir -p build/u2 build/u2/coq

CC_BIN=${CC:-cc}
COMMON_CFLAGS=(-std=c99 -Wall -Wextra -pedantic -O2 -Iruntime)
SPECTRAL_CFLAGS=(-DCDC_U2_LAPACK_DGEES)
if [ "$(uname -s)" = "Darwin" ]; then
  LINALG_LIBS=(-lm -framework Accelerate)
else
  LINALG_LIBS=(-lm -llapack -lblas)
fi

echo "== U2 deterministic linear algebra and Schur validation =="
"$CC_BIN" "${COMMON_CFLAGS[@]}" \
  tests/u2/test_u2_linalg.c runtime/cdc_linalg.c \
  -lm -o build/u2/test_u2_linalg_no_backend
build/u2/test_u2_linalg_no_backend | tee build/u2/linalg_no_backend.txt
grep -q '^u2 system real Schur backend: unavailable (typed hold permitted)$' \
  build/u2/linalg_no_backend.txt
grep -q '^u2 linalg adversarial tests: ok$' build/u2/linalg_no_backend.txt

"$CC_BIN" "${COMMON_CFLAGS[@]}" \
  "${SPECTRAL_CFLAGS[@]}" \
  tests/u2/test_u2_linalg.c runtime/cdc_linalg.c \
  "${LINALG_LIBS[@]}" -o build/u2/test_u2_linalg
build/u2/test_u2_linalg | tee build/u2/linalg.txt
grep -q '^u2 system real Schur backend: ok$' build/u2/linalg.txt
grep -q '^u2 linalg adversarial tests: ok$' build/u2/linalg.txt

echo
echo "== U2 executable derivative, recurrence, saltation, and spectrum =="
"$CC_BIN" "${COMMON_CFLAGS[@]}" \
  "${SPECTRAL_CFLAGS[@]}" \
  tests/u2/test_u2_variational.c \
  runtime/cdc_variational.c runtime/cdc_linalg.c \
  "${LINALG_LIBS[@]}" -o build/u2/test_u2_variational
build/u2/test_u2_variational | tee build/u2/variational.txt
grep -q '^u2 variational adversarial tests: ok$' build/u2/variational.txt

echo
echo "== Permanent U2 mutant rejection =="
for mutant in \
  CDC_U2_MUTANT_REVERSE_PRODUCT \
  CDC_U2_MUTANT_OMIT_SALTATION \
  CDC_U2_MUTANT_OMIT_RESTORATION_DERIVATIVE \
  CDC_U2_MUTANT_THEORETICAL_NEST_DERIVATIVE \
  CDC_U2_MUTANT_ACCEPT_FALSE_RECURRENCE \
  CDC_U2_MUTANT_IGNORE_POLARITY_APERTURE \
  CDC_U2_MUTANT_SKIP_POLARITY_TANGENT \
  CDC_U2_MUTANT_ACCEPT_FALSE_POLARITY
do
  binary="build/u2/mutant_${mutant}"
  output="build/u2/mutant_${mutant}.txt"
  "$CC_BIN" "${COMMON_CFLAGS[@]}" "${SPECTRAL_CFLAGS[@]}" \
    "-D${mutant}" \
    tests/u2/test_u2_variational.c \
    runtime/cdc_variational.c runtime/cdc_linalg.c \
    "${LINALG_LIBS[@]}" -o "$binary"
  if "$binary" >"$output" 2>&1; then
    echo "U2 mutant survived: ${mutant}" >&2
    exit 1
  fi
  grep -q '^u2-variational-test:' "$output"
  echo "mutant rejected: ${mutant}"
done

echo
echo "== U2 native stability integration and deterministic receipts =="
"$CC_BIN" "${COMMON_CFLAGS[@]}" -pthread \
  runtime/cdc_native_runtime.c \
  runtime/cdc_variational.c runtime/cdc_linalg.c \
  runtime/cdc_source.c runtime/cdc_store.c runtime/cdc_digest.c \
  runtime/cdc_blake3.c runtime/cdc_receipt.c runtime/cdc_parser.c \
  runtime/cdc_ast.c runtime/cdc_lexer.c runtime/cdc_diagnostic.c \
  -lm -o build/u2/cdc_native_runtime_no_backend

"$CC_BIN" "${COMMON_CFLAGS[@]}" -pthread \
  "${SPECTRAL_CFLAGS[@]}" \
  runtime/cdc_native_runtime.c \
  runtime/cdc_variational.c runtime/cdc_linalg.c \
  runtime/cdc_source.c runtime/cdc_store.c runtime/cdc_digest.c \
  runtime/cdc_blake3.c runtime/cdc_receipt.c runtime/cdc_parser.c \
  runtime/cdc_ast.c runtime/cdc_lexer.c runtime/cdc_diagnostic.c \
  "${LINALG_LIBS[@]}" -o build/u2/cdc_native_runtime

rm -f build/u2/enacted_projection_hold.cdc
rm -f build/u2/enacted_true_relative.cdc
rm -f build/u2/enacted_primal_hold.cdc
build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_projection_hold.cdc \
  > build/u2/stability_a.txt
test ! -e build/u2/enacted_projection_hold.cdc
build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_projection_hold.cdc \
  > build/u2/stability_b.txt
test ! -e build/u2/enacted_projection_hold.cdc
test ! -e build/u2/enacted_true_relative.cdc
cmp build/u2/stability_a.txt build/u2/stability_b.txt

# Source identity is the exact grammar-1 canonical statement stream consumed
# by the runtime, not a second read of incidental raw file formatting.
cp tests/fixtures/u2/u720_projection_hold.cdc \
  build/u2/u720_projection_hold_comment_only.cdc
printf '\n# canonical source identity ignores comment-only formatting\n' >> \
  build/u2/u720_projection_hold_comment_only.cdc
build/u2/cdc_native_runtime stability \
  build/u2/u720_projection_hold_comment_only.cdc \
  > build/u2/stability_canonical_source.txt
cmp build/u2/stability_a.txt build/u2/stability_canonical_source.txt

build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_true_relative_marginal.cdc \
  > build/u2/stability_positive_a.txt
test ! -e build/u2/enacted_true_relative.cdc
build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_true_relative_marginal.cdc \
  > build/u2/stability_positive_b.txt
test ! -e build/u2/enacted_true_relative.cdc
cmp build/u2/stability_positive_a.txt build/u2/stability_positive_b.txt

sed 's/neutral-tolerance=0.000000001/neutral-tolerance=0.000000002/' \
  tests/fixtures/u2/u720_true_relative_marginal.cdc \
  > build/u2/u720_true_relative_semantic_change.cdc
build/u2/cdc_native_runtime stability \
  build/u2/u720_true_relative_semantic_change.cdc \
  > build/u2/stability_semantic_source.txt
python3 - build/u2/stability_positive_a.txt \
  build/u2/stability_semantic_source.txt <<'PY'
import json
import sys
from pathlib import Path

def one(path):
    records = [
        json.loads(line.removeprefix("u2-json="))
        for line in Path(path).read_text(encoding="utf-8").splitlines()
        if line.startswith("u2-json=")
    ]
    if len(records) != 1:
        raise SystemExit(f"expected one U2 record in {path}, got {len(records)}")
    return records[0]

before, after = map(one, sys.argv[1:])
if before.get("sourceDigest") == after.get("sourceDigest"):
    raise SystemExit("canonical source identity ignored a semantic token change")
if after.get("status") != "accepted":
    raise SystemExit("semantic identity probe unexpectedly changed the U2 verdict")
PY

build/u2/cdc_native_runtime_no_backend stability \
  tests/fixtures/u2/u720_true_relative_marginal.cdc \
  > build/u2/stability_no_backend.txt
test ! -e build/u2/enacted_true_relative.cdc

build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_primal_hold.cdc \
  > build/u2/stability_primal_hold_a.txt
test ! -e build/u2/enacted_primal_hold.cdc
build/u2/cdc_native_runtime stability \
  tests/fixtures/u2/u720_primal_hold.cdc \
  > build/u2/stability_primal_hold_b.txt
test ! -e build/u2/enacted_primal_hold.cdc
cmp build/u2/stability_primal_hold_a.txt \
  build/u2/stability_primal_hold_b.txt

python3 - build/u2/stability_a.txt <<'PY'
import copy
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])
records = []
for line in path.read_text(encoding="utf-8").splitlines():
    if line.startswith("u2-json="):
        records.append(json.loads(line.removeprefix("u2-json=")))

if len(records) != 2:
    raise SystemExit(f"expected exactly two canonical U2 records, got {len(records)}")

by_orbit = {record.get("orbit"): record for record in records}
expected = {"u720-full", "u720-cover-projection"}
if set(by_orbit) != expected:
    raise SystemExit(f"unexpected U2 orbit records: {sorted(by_orbit)}")

def validate_tangent_metadata(record):
    method_detail = record.get("methodDetail")
    if not isinstance(method_detail, dict):
        raise SystemExit("U2 receipt lacks granular method detail")
    if method_detail.get("flow", {}).get("localMap") != "explicit-euler-synchronous":
        raise SystemExit("U2 flow method detail drifted")
    if method_detail.get("guard", {}).get("binding", "sentinel") is not None:
        raise SystemExit("U2 guard detail invented an executable binding")
    finite_difference = record.get("finiteDifference")
    if finite_difference != {"status": "not-run", "step": None, "residual": None}:
        raise SystemExit(f"unexpected finite-difference placeholder: {finite_difference}")
    event_budget = record.get("eventBudget")
    if event_budget != {"status": "not-applicable", "limit": None, "used": None}:
        raise SystemExit(f"unexpected event-budget placeholder: {event_budget}")
    neutral = record.get("neutralModeRemoval")
    if neutral != {
        "status": "not-requested",
        "generatorDigest": None,
        "removedModes": None,
    }:
        raise SystemExit(f"unexpected neutral-removal placeholder: {neutral}")

def validate_discrete_state(record):
    discrete = record.get("discreteState")
    recurrence = record.get("recurrence") or {}
    coordinates = record.get("coordinates") or []
    theta_coordinates = [
        coordinate.get("name")
        for coordinate in coordinates
        if str(coordinate.get("name", "")).endswith(".theta")
    ]
    if not isinstance(discrete, dict):
        raise SystemExit("U2 receipt lacks selected-cell discrete state")
    if discrete.get("verified") is not recurrence.get("discreteStateVerified"):
        raise SystemExit("discrete-state verification drifted from recurrence metadata")
    for digest_key in ("initialDigest", "finalDigest"):
        if not str(discrete.get(digest_key, "")).startswith("blake3:"):
            raise SystemExit(f"U2 discrete state lacks {digest_key}")
    for phase in ("initial", "final"):
        entries = discrete.get(phase)
        if not isinstance(entries, list) or len(entries) != len(theta_coordinates):
            raise SystemExit(f"U2 {phase} discrete state has wrong length")
        for entry, coordinate_name in zip(entries, theta_coordinates):
            if entry.get("coordinate") != coordinate_name:
                raise SystemExit("discrete-state coordinate order drifted")
            if entry.get("cell") != coordinate_name[:-6]:
                raise SystemExit("discrete-state cell identity drifted")
            if entry.get("mode") not in {
                "unlatched",
                "latched:+",
                "latched:0",
                "latched:-",
            }:
                raise SystemExit(f"unknown discrete mode label: {entry.get('mode')}")
            if not isinstance(entry.get("hasLatch"), bool):
                raise SystemExit("discrete-state hasLatch flag is malformed")
            if entry.get("hasLatch"):
                if entry.get("latch") not in {"+", "0", "-"}:
                    raise SystemExit("latched discrete state lacks its trit")
            elif entry.get("latch") is not None or entry.get("mode") != "unlatched":
                raise SystemExit("unlatched discrete state fabricated a latch")

def validate_record(record):
    validate_tangent_metadata(record)
    validate_discrete_state(record)

for record in records:
    if record.get("schema") != "cdc.u2.stability.v1":
        raise SystemExit("unstable or missing U2 schema id")
    if record.get("runtimeIdentity") != "cdc-native-u2-v1":
        raise SystemExit("U2 receipt lacks a versioned runtime identity")
    if record.get("sourceDigestSurface") != "grammar-1-canonical":
        raise SystemExit("U2 receipt source identity is not parser-canonical")
    for digest_key in ("sourceDigest", "manifestDigest", "pathTangentDigest"):
        if not str(record.get(digest_key, "")).startswith("blake3:"):
            raise SystemExit(f"U2 receipt lacks {digest_key}")
    universal = record.get("universal") or {}
    if universal.get("status") != "accepted" or not str(
        universal.get("resultDigest", "")
    ).startswith("blake3:"):
        raise SystemExit("U2 receipt lacks its admitted U1 result binding")
    if record.get("analysis") != "tangent":
        raise SystemExit("held recurrence must retain its honest tangent analysis")
    dimension = record.get("dimension")
    if not isinstance(record.get("initialState"), list) or len(
        record["initialState"]
    ) != dimension:
        raise SystemExit("held U2 receipt lacks its initial executable state")
    if not isinstance(record.get("finalState"), list) or len(
        record["finalState"]
    ) != dimension:
        raise SystemExit("held U2 receipt lacks its final executable state")
    if not isinstance(record.get("pathTangent"), list) or len(
        record["pathTangent"]
    ) != dimension * dimension:
        raise SystemExit("held U2 receipt lacks its non-returning path tangent")
    if record.get("methods", {}).get("guard") != "unbound-no-saltation":
        raise SystemExit("U2 receipt hid the current guard-localization boundary")
    validate_record(record)
    if record.get("status") != "held":
        raise SystemExit(f"non-returning U720 analysis must hold: {record}")
    if record.get("monodromyDigest") is not None:
        raise SystemExit("held U2 record exposed a monodromy digest")
    if record.get("monodromy") not in (None, []):
        raise SystemExit("held U2 record exposed a monodromy matrix")
    if record.get("multipliers") not in (None, []):
        raise SystemExit("held U2 record exposed a spectrum")
    if record.get("classification") not in (None, "held"):
        raise SystemExit("held U2 record exposed a stability classification")
    if record.get("spectrumDiagnostics") is not None:
        raise SystemExit("held U2 record exposed spectrum diagnostics")

full = by_orbit["u720-full"]
if full.get("reason") not in {"recurrence-mode-mismatch", "recurrence-residual"}:
    raise SystemExit(f"full U720 hold has wrong reason: {full.get('reason')}")

projected = by_orbit["u720-cover-projection"]
if projected.get("reason") != "undeclared-quotient":
    raise SystemExit(f"projected U720 hold has wrong reason: {projected.get('reason')}")
recurrence = projected.get("recurrence") or {}
if recurrence.get("scope") != "projected" or recurrence.get("authorizesMonodromy") is not False:
    raise SystemExit("projection was promoted to relative recurrence")

for mutation in (
    ("missing discrete digest", lambda record: record["discreteState"].pop("initialDigest")),
    ("semantic drifted finite-difference status",
     lambda record: record["finiteDifference"].__setitem__("status", "ran")),
):
    mutated = copy.deepcopy(full)
    mutation[1](mutated)
    try:
        validate_record(mutated)
    except SystemExit:
        pass
    else:
        raise SystemExit(f"semantic-drift/malformed guard missed {mutation[0]}")

print("u2 canonical held-receipt contract: ok")
PY

python3 - build/u2/stability_positive_a.txt <<'PY'
import copy
import json
import math
import sys
from pathlib import Path

path = Path(sys.argv[1])
records = [
    json.loads(line.removeprefix("u2-json="))
    for line in path.read_text(encoding="utf-8").splitlines()
    if line.startswith("u2-json=")
]
if len(records) != 1:
    raise SystemExit(f"expected one accepted U2 record, got {len(records)}")
record = records[0]
if record.get("schema") != "cdc.u2.stability.v1":
    raise SystemExit("accepted U2 record has wrong schema")
if record.get("orbit") != "fixture-relative-orbit":
    raise SystemExit(f"accepted U2 record has wrong orbit: {record.get('orbit')}")
if record.get("status") != "accepted" or record.get("reason") != "none":
    raise SystemExit(f"true relative recurrence did not accept: {record}")
if record.get("analysis") != "monodromy":
    raise SystemExit("accepted recurrent receipt has wrong analysis level")
if record.get("runtimeIdentity") != "cdc-native-u2-v1":
    raise SystemExit("accepted receipt lacks a versioned runtime identity")
if record.get("sourceDigestSurface") != "grammar-1-canonical":
    raise SystemExit("accepted receipt source identity is not parser-canonical")
for digest_key in (
    "sourceDigest",
    "manifestDigest",
    "pathTangentDigest",
    "monodromyDigest",
):
    if not str(record.get(digest_key, "")).startswith("blake3:"):
        raise SystemExit(f"accepted U2 receipt lacks {digest_key}")
universal = record.get("universal") or {}
if universal.get("status") != "accepted" or not str(
    universal.get("resultDigest", "")
).startswith("blake3:"):
    raise SystemExit("accepted U2 receipt lacks its U1 result binding")

recurrence = record.get("recurrence") or {}
if recurrence.get("kind") != "relative" or recurrence.get("verified") is not True:
    raise SystemExit("relative recurrence receipt is missing verification")
if recurrence.get("authorizesMonodromy") is not True:
    raise SystemExit("verified relative recurrence did not authorize monodromy")
if recurrence.get("discreteStateVerified") is not True:
    raise SystemExit("accepted recurrence lost discrete-state verification")
if recurrence.get("restorationDerivativeApplied") is not True:
    raise SystemExit("relative recurrence omitted D-rho application")

method_detail = record.get("methodDetail")
if not isinstance(method_detail, dict):
    raise SystemExit("accepted receipt lacks granular method detail")
if method_detail.get("nest", {}).get("childPrior") != "overwrite-parent-belief":
    raise SystemExit("accepted receipt drifted from executable nest semantics")
if record.get("finiteDifference") != {"status": "not-run", "step": None, "residual": None}:
    raise SystemExit("accepted receipt finite-difference placeholder drifted")
if record.get("eventBudget") != {"status": "not-applicable", "limit": None, "used": None}:
    raise SystemExit("accepted receipt event-budget placeholder drifted")
if record.get("neutralModeRemoval") != {
    "status": "not-requested",
    "generatorDigest": None,
    "removedModes": None,
}:
    raise SystemExit("accepted receipt neutral-removal placeholder drifted")

dimension = record.get("dimension")
if dimension != 13:
    raise SystemExit(f"unexpected accepted tangent dimension: {dimension}")
coordinate_names = [coordinate.get("name") for coordinate in record.get("coordinates", [])]
expected_coordinates = [
    "agent.a.theta",
    "agent.b.theta",
    "agent.c.theta",
    "context.a.theta",
    "context.b.theta",
    "context.c.theta",
    "cover.phase.theta",
    "agent.belief",
    "agent.prior",
    "context.belief",
    "context.prior",
    "cover.belief",
    "cover.prior",
]
if coordinate_names != expected_coordinates:
    raise SystemExit(
        "accepted manifest is not the exact source-ordered Universal path closure: "
        f"{coordinate_names}"
    )
if any("dead" in name for name in coordinate_names):
    raise SystemExit("unrelated declaration island leaked into the U2 manifest")

discrete = record.get("discreteState")
if not isinstance(discrete, dict) or discrete.get("verified") is not True:
    raise SystemExit("accepted receipt lacks verified discrete-state metadata")
for digest_key in ("initialDigest", "finalDigest"):
    if not str(discrete.get(digest_key, "")).startswith("blake3:"):
        raise SystemExit(f"accepted receipt discrete state lacks {digest_key}")
for phase in ("initial", "final"):
    entries = discrete.get(phase)
    if not isinstance(entries, list) or len(entries) != 7:
        raise SystemExit(f"accepted receipt {phase} discrete-state length drifted")
    for entry, coordinate_name in zip(entries, expected_coordinates[:7]):
        if entry.get("coordinate") != coordinate_name:
            raise SystemExit("accepted discrete-state coordinate order drifted")
        if entry.get("cell") != coordinate_name[:-6]:
            raise SystemExit("accepted discrete-state cell identity drifted")
        if entry.get("mode") not in {
            "unlatched",
            "latched:+",
            "latched:0",
            "latched:-",
        }:
            raise SystemExit(f"accepted receipt discrete mode malformed: {entry}")

restoration = recurrence.get("restoration") or {}
if restoration.get("action") != "cover-phase-translation":
    raise SystemExit("relative receipt lacks its executable restoration action")
if restoration.get("coordinateIndex") != 6 or restoration.get("coordinate") != "cover.phase.theta":
    raise SystemExit("relative restoration is not bound to the lifted cover coordinate")
if not math.isclose(restoration.get("displacement"), 4.0 * math.pi,
                    rel_tol=0.0, abs_tol=1e-12):
    raise SystemExit("relative restoration carries the wrong cover displacement")
equivariance = restoration.get("equivarianceWitness") or {}
if equivariance != {
    "id": "isolated-affine-two-turn-cover",
    "verified": True,
    "fieldGain": 0,
    "fieldCellCount": 1,
    "incidentChannelCount": 0,
    "mutatingStepCount": 0,
}:
    raise SystemExit(f"relative restoration lacks its exact equivariance witness: {equivariance}")
section = restoration.get("sectionWitness") or {}
if section != {
    "id": "u1-two-turn-returned-restored",
    "verified": True,
    "winding": 2,
    "projection": "returned",
    "sheet": "restored",
}:
    raise SystemExit(f"relative restoration lacks its exact U1 section witness: {section}")
if restoration.get("derivativeRows") != dimension or restoration.get("derivativeColumns") != dimension:
    raise SystemExit("relative restoration derivative has the wrong shape")
if not str(restoration.get("derivativeDigest", "")).startswith("blake3:"):
    raise SystemExit("relative restoration derivative lacks its artifact digest")
restoration_derivative = restoration.get("derivative")
if not isinstance(restoration_derivative, list) or len(restoration_derivative) != dimension * dimension:
    raise SystemExit("relative restoration lacks the complete row-major D-rho artifact")
for row in range(dimension):
    for column in range(dimension):
        expected = 1.0 if row == column else 0.0
        if not math.isclose(
            restoration_derivative[row * dimension + column], expected,
            rel_tol=0.0, abs_tol=1e-12,
        ):
            raise SystemExit("relative restoration D-rho is not the witnessed identity")
for state_key in ("initialState", "finalState"):
    if not isinstance(record.get(state_key), list) or len(record[state_key]) != dimension:
        raise SystemExit(f"accepted receipt lacks complete {state_key}")
if not isinstance(record.get("pathTangent"), list) or len(
    record["pathTangent"]
) != dimension * dimension:
    raise SystemExit("accepted receipt lacks its pre-restoration path tangent")
events = record.get("events") or []
if not events or events[-1].get("kind") != "endpoint-restoration":
    raise SystemExit("accepted relative itinerary lacks endpoint restoration")

monodromy = record.get("monodromy")
if not isinstance(monodromy, list) or len(monodromy) != dimension * dimension:
    raise SystemExit("accepted record lacks complete row-major monodromy")
for row in range(dimension):
    for column in range(dimension):
        expected = 1.0 if row == column else 0.0
        if not math.isclose(monodromy[row * dimension + column], expected,
                            rel_tol=0.0, abs_tol=1e-10):
            raise SystemExit("identity relative monodromy mismatch")

multipliers = record.get("multipliers")
if not isinstance(multipliers, list) or len(multipliers) != dimension:
    raise SystemExit("accepted record lacks complete multiplier spectrum")
for multiplier in multipliers:
    if multiplier.get("mode") != "physical":
        raise SystemExit("unit mode was labelled gauge without a generator")
    for key, expected in (("real", 1.0), ("imag", 0.0), ("modulus", 1.0)):
        if not math.isclose(multiplier.get(key), expected,
                            rel_tol=0.0, abs_tol=1e-10):
            raise SystemExit(f"unit multiplier mismatch: {multiplier}")
if record.get("classification") != "marginal":
    raise SystemExit("identity return map was not classified marginal")
if record.get("backend") != "lapack-dgees":
    raise SystemExit(f"accepted spectrum used unexpected backend: {record.get('backend')}")
spectrum_diagnostics = record.get("spectrumDiagnostics") or {}
schur = spectrum_diagnostics.get("schur") or {}
if not math.isclose(spectrum_diagnostics.get("spectralRadius"), 1.0,
                    rel_tol=0.0, abs_tol=1e-10):
    raise SystemExit("accepted receipt lost its spectral radius")
if schur.get("validationTolerance") != record.get("tolerances", {}).get("schur"):
    raise SystemExit("accepted receipt Schur tolerance drifted")
for key in ("reconstructionResidual", "orthogonalityResidual", "triangularResidual"):
    value = schur.get(key)
    if not isinstance(value, (int, float)) or value < 0.0:
        raise SystemExit(f"accepted receipt Schur diagnostic malformed: {key}")
    if value > schur["validationTolerance"]:
        raise SystemExit(f"accepted receipt Schur diagnostic exceeded tolerance: {key}={value}")

def validate_mutation(candidate):
    candidate_discrete = candidate.get("discreteState") or {}
    if candidate.get("recurrence", {}).get("discreteStateVerified") is not True:
        raise SystemExit("mutation validator: recurrence verification drift")
    if candidate_discrete.get("verified") is not True:
        raise SystemExit("mutation validator: discrete verification drift")
    candidate_diag = candidate.get("spectrumDiagnostics") or {}
    if candidate_diag.get("spectralRadius") != 1.0:
        raise SystemExit("mutation validator: spectral radius drift")
    if (candidate_diag.get("schur") or {}).get("validationTolerance") != \
            candidate.get("tolerances", {}).get("schur"):
        raise SystemExit("mutation validator: Schur tolerance drift")

for mutation in (
    ("missing spectral radius",
     lambda candidate: candidate["spectrumDiagnostics"].pop("spectralRadius")),
    ("discrete verification drift",
     lambda candidate: candidate["discreteState"].__setitem__("verified", False)),
):
    mutated = copy.deepcopy(record)
    mutation[1](mutated)
    try:
        validate_mutation(mutated)
    except SystemExit:
        pass
    else:
        raise SystemExit(f"semantic-drift/malformed guard missed {mutation[0]}")

print("u2 accepted relative monodromy and Schur spectrum: ok")
PY

python3 - build/u2/stability_no_backend.txt <<'PY'
import json
import sys
from pathlib import Path

records = [
    json.loads(line.removeprefix("u2-json="))
    for line in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines()
    if line.startswith("u2-json=")
]
if len(records) != 1:
    raise SystemExit(f"expected one no-backend record, got {len(records)}")
record = records[0]
if record.get("status") != "held" or record.get("reason") != "spectral-backend-unavailable":
    raise SystemExit(f"no-backend runtime did not produce its typed hold: {record}")
if record.get("analysis") != "monodromy":
    raise SystemExit("no-backend hold did not retain monodromy analysis")
recurrence = record.get("recurrence") or {}
if recurrence.get("verified") is not True or recurrence.get("authorizesMonodromy") is not True:
    raise SystemExit("no-backend hold obscured the independently verified recurrence")
if recurrence.get("discreteStateVerified") is not True:
    raise SystemExit("no-backend hold lost discrete-state verification")
if not str(record.get("monodromyDigest", "")).startswith("blake3:"):
    raise SystemExit("no-backend hold lost its monodromy digest")
monodromy = record.get("monodromy")
dimension = record.get("dimension")
if not isinstance(monodromy, list) or len(monodromy) != dimension * dimension:
    raise SystemExit("no-backend hold lost its bound monodromy artifact")
if record.get("spectrumDiagnostics") is not None:
    raise SystemExit("no-backend hold fabricated spectrum diagnostics")
if record.get("multipliers") is not None or record.get("backend") is not None:
    raise SystemExit("no-backend runtime fabricated a multiplier backend")
if record.get("classification") != "held":
    raise SystemExit("no-backend hold lost its held classification")
print("u2 native spectral-backend-unavailable hold: ok")
PY

python3 - build/u2/stability_primal_hold_a.txt <<'PY'
import json
import sys
from pathlib import Path

records = [
    json.loads(line.removeprefix("u2-json="))
    for line in Path(sys.argv[1]).read_text(encoding="utf-8").splitlines()
    if line.startswith("u2-json=")
]
if len(records) != 1:
    raise SystemExit(f"expected one primal-hold record, got {len(records)}")
record = records[0]
if record.get("status") != "held" or record.get("reason") != "primal-held":
    raise SystemExit(f"invalid U1 path did not produce a typed primal hold: {record}")
if record.get("analysis") != "held":
    raise SystemExit("primal hold falsely advertised a tangent analysis")
universal = record.get("universal") or {}
if universal.get("status") != "held" or universal.get("reason") != "cone-not-reciprocal":
    raise SystemExit("U2 receipt lost the causal U1 hold reason")
for key in (
    "manifestDigest",
    "methodDetail",
    "finiteDifference",
    "eventBudget",
    "neutralModeRemoval",
    "recurrence",
    "discreteState",
    "dimension",
    "coordinates",
    "initialState",
    "finalState",
    "pathTangentDigest",
    "pathTangent",
    "spectrumDiagnostics",
    "monodromyDigest",
    "monodromy",
    "multipliers",
):
    if record.get(key) is not None:
        raise SystemExit(f"primal hold exposed invalid analysis field {key}")
print("u2 pre-snapshot primal hold contract: ok")
PY

if (( SKIP_FORMAL == 0 )); then
  echo
  echo "== U2 finite formal obligations (claim-ceiling bounded) =="
  command -v lean >/dev/null 2>&1
  command -v coqc >/dev/null 2>&1
  lean formal/lean/U2VariationalFinite.lean
  coqc -q -noglob -o build/u2/coq/U2VariationalFinite.vo \
    formal/coq/U2VariationalFinite.v
  echo "u2 finite identity/composition/order/polarity lemmas: ok"
fi

git diff --check
echo
if (( SKIP_FORMAL )); then
  echo "U2_RUNTIME_GATE_PASS"
else
  echo "U2_GATE_PASS"
fi

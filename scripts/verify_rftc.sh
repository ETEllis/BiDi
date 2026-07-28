#!/usr/bin/env bash
# Rapid falsification gate for Reference-Frame Topological Coherence (RFTC).
# The gate is intentionally classical: it validates collective reduction,
# topology, hidden granularity, bidirectional control, and the causal boundary
# that prevents those results from being mislabeled as quantum computation.
set -euo pipefail

cd "$(dirname "$0")/.."

compiler="${CC:-cc}"
out_dir="build/rftc"
source_file="experiments/rftc/rftc_crucible.c"
runtime_sources=(runtime/cdc_digest.c runtime/cdc_blake3.c)
common_flags=(-std=c99 -Wall -Wextra -pedantic)

mkdir -p "$out_dir"

echo "== RFTC: release build and deterministic replay =="
"$compiler" "${common_flags[@]}" -O2 "$source_file" \
  "${runtime_sources[@]}" -lm -o "$out_dir/rftc_crucible"

"$out_dir/rftc_crucible" --profile smoke \
  --json "$out_dir/verdict-a.json" --csv "$out_dir/metrics-a.csv"
"$out_dir/rftc_crucible" --profile smoke \
  --json "$out_dir/verdict-b.json" --csv "$out_dir/metrics-b.csv"

cmp "$out_dir/verdict-a.json" "$out_dir/verdict-b.json"
cmp "$out_dir/metrics-a.csv" "$out_dir/metrics-b.csv"
cmp "$out_dir/verdict-a.json" "demo/rftc-verdict.json"

jq -e '
  .schema == "rftc-crucible/v1" and
  .classification == "CLASSICAL_REFERENCE_FRAME_TOPOLOGICAL_COHERENCE" and
  .verdict == "PASS_FOUNDATIONAL_CLASSICAL_MECHANISM" and
  ([.experiments[].status] | all(. == "PASS")) and
  (.claims.notAllowed | index("quantum superposition") != null) and
  (.claims.notAllowed | index("entanglement") != null) and
  (.claims.notAllowed | index("quantum computational advantage") != null)
' "$out_dir/verdict-a.json" >/dev/null

python3 - experiments/rftc/ui/index.html demo/rftc-verdict.json <<'PY'
import re
import sys

html_path, verdict_path = sys.argv[1:3]
html = open(html_path).read()
match = re.search(
    r'<script type="application/json" id="rftc-data">\n(.*?)\n\s*</script>',
    html,
    re.S,
)
if not match:
    raise SystemExit("RFTC UI has no embedded evidence record")
runtime = open(verdict_path).read().rstrip("\n")
if match.group(1) != runtime:
    raise SystemExit("RFTC UI evidence differs from runtime verdict")
for token in ('lang="en"', "<title>", "prefers-reduced-motion", "aria-live"):
    if token not in html:
        raise SystemExit(f"RFTC UI missing accessibility token: {token}")
if re.search(r'(?:src|href)\s*=\s*"(?:https?:)?//', html):
    raise SystemExit("RFTC UI must remain self-contained")
print("RFTC UI self-contained, evidence-bound, reduced-motion aware")
PY

echo
echo "== RFTC: alternate-seed counterexample search =="
"$out_dir/rftc_crucible" --profile smoke --seed 0x5246544300abcdef \
  --json "$out_dir/verdict-alternate.json" \
  --csv "$out_dir/metrics-alternate.csv"
jq -e '
  .verdict == "PASS_FOUNDATIONAL_CLASSICAL_MECHANISM" and
  ([.experiments[].status] | all(. == "PASS"))
' "$out_dir/verdict-alternate.json" >/dev/null

if cmp -s <(jq -c '.experiments' "$out_dir/verdict-a.json") \
          <(jq -c '.experiments' "$out_dir/verdict-alternate.json"); then
  echo "alternate seed did not alter any experiment metric" >&2
  exit 1
fi

echo
echo "== RFTC: memory and undefined-behavior sanitizers =="
"$compiler" "${common_flags[@]}" -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  "$source_file" "${runtime_sources[@]}" -lm \
  -o "$out_dir/rftc_crucible_sanitized"
"$out_dir/rftc_crucible_sanitized" --profile smoke \
  --json "$out_dir/verdict-sanitized.json" \
  --csv "$out_dir/metrics-sanitized.csv"
jq -e '.verdict == "PASS_FOUNDATIONAL_CLASSICAL_MECHANISM"' \
  "$out_dir/verdict-sanitized.json" >/dev/null

cp "$out_dir/verdict-a.json" "$out_dir/verdict.json"
cp "$out_dir/metrics-a.csv" "$out_dir/metrics.csv"

echo
echo "RFTC rapid gate PASS"

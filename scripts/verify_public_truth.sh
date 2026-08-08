#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
mkdir -p build

fail() {
  echo "public truth gate: $*" >&2
  exit 1
}

require_text() {
  local file=$1
  local text=$2
  grep -Fq "$text" "$file" || fail "$file is missing current truth token: $text"
}

echo "== Public truth: release, grammar, ABI, and registry parity =="

PYPROJECT_VERSION=$(sed -n 's/^version = "\([^"]*\)"/\1/p' pyproject.toml)
CITATION_VERSION=$(sed -n 's/^version: "\([^"]*\)"/\1/p' CITATION.cff)
test -n "$PYPROJECT_VERSION" || fail "pyproject.toml has no project version"
test "$PYPROJECT_VERSION" = "$CITATION_VERSION" || \
  fail "pyproject/CITATION version mismatch: $PYPROJECT_VERSION != $CITATION_VERSION"
test "$PYPROJECT_VERSION" = "0.3.0" || \
  fail "release truth changed without reconciling this gate: $PYPROJECT_VERSION"

ABI_MAJOR=$(awk '/^#define CDC_ABI_VERSION_MAJOR / {print $3}' runtime/cdc_abi.h)
ABI_MINOR=$(awk '/^#define CDC_ABI_VERSION_MINOR / {print $3}' runtime/cdc_abi.h)
test "$ABI_MAJOR.$ABI_MINOR" = "1.5" || \
  fail "ABI truth changed without public reconciliation: $ABI_MAJOR.$ABI_MINOR"

if [ -x build/cdc ]; then
  VERSION_LINE=$(build/cdc version)
  test "$VERSION_LINE" = "cdc abi=1.5 grammar=1" || \
    fail "native version surface disagrees: $VERSION_LINE"
else
  require_text runtime/toolchain/main.c 'printf("cdc abi=%u.%u grammar=1'
  echo "build/cdc not present; source ABI/grammar surface checked"
fi

python3 cdc_boot.py > build/public-truth-contract.txt
EXPECTATIONS=$(sed -n 's/^  \([0-9][0-9]*\/[0-9][0-9]*\) expectations met$/\1/p' \
  build/public-truth-contract.txt)
COUNTS=$(sed -n 's/^  \([0-9][0-9]*\) terms, \([0-9][0-9]*\) rules, \([0-9][0-9]*\) invariants$/\1 \2 \3/p' \
  build/public-truth-contract.txt)
AGGREGATES=$(sed -n 's/^  \([0-9][0-9]*\) capabilities, \([0-9][0-9]*\) frameworks, \([0-9][0-9]*\) native witnesses$/\1 \2 \3/p' \
  build/public-truth-contract.txt)
test "$EXPECTATIONS" = "262/262" || fail "unexpected registry expectations: $EXPECTATIONS"
test "$COUNTS" = "20 22 16" || fail "unexpected term/rule/invariant counts: $COUNTS"
test "$AGGREGATES" = "46 6 4831" || fail "unexpected capability/framework/witness counts: $AGGREGATES"

require_text README.md 'Version **0.3.0**'
require_text README.md '262/262 expectations'
require_text README.md '20 terms · 22 rules · 16 invariants'
require_text README.md '46 capabilities · 6 frameworks · 4831 native witnesses'
require_text README.md 'ABI 1.5'
require_text README.md 'Within the language/runtime/semantic execution boundary'

require_text CDC_LANGUAGE.md 'Version 0.3.0'
require_text CDC_LANGUAGE.md '262/262 expectations'
require_text CDC_LANGUAGE.md '20 terms · 22 rules · 16 invariants'
require_text CDC_LANGUAGE.md '46 capabilities · 6 frameworks · 4831 native witnesses'
require_text CDC_LANGUAGE.md 'language/runtime/semantic execution boundary'

require_text VERIFICATION_OBLIGATION_MATRIX.md '`262/262` expectations'
require_text VERIFICATION_OBLIGATION_MATRIX.md '46 capabilities'
require_text VERIFICATION_OBLIGATION_MATRIX.md '4831 native witnesses'

require_text paper/arxiv/main.tex 'version 0.3.0'
require_text paper/arxiv/main.tex '262 accepted expectations'
require_text paper/arxiv/main.tex '46 capabilities'
require_text paper/arxiv/main.tex '4831 native'
require_text paper/arxiv/main.tex 'language/runtime/semantic execution boundary'

echo "== Public truth: U1/U2 and claim-ceiling parity =="
for file in README.md CDC_LANGUAGE.md FORMAL_SEMANTIC_SPINE.md \
  VERIFICATION_OBLIGATION_MATRIX.md; do
  require_text "$file" 'recurrence-mode-mismatch'
  require_text "$file" 'M_rel = I_13'
done
require_text README.md 'apertured oriented reciprocity'
require_text FORMAL_SEMANTIC_SPINE.md 'apertured oriented reciprocity'
require_text paper/arxiv/main.tex 'recurrence-mode-mismatch'
require_text paper/arxiv/main.tex 'no canonical-loop multipliers are emitted'

CURRENT_SURFACES=(
  README.md
  CDC_LANGUAGE.md
  FORMAL_SEMANTIC_SPINE.md
  VERIFICATION_OBLIGATION_MATRIX.md
  paper/arxiv/main.tex
  CITATION.cff
  pyproject.toml
  ui/web/console/index.html
  demo/index.html
)
STALE_PATTERN='238/238|4811/4811|14/14 invariant|38/38 capabilit|5/5 framework|ABI 1\.2|abi=1\.2|v0\.2\.4|version 0\.2\.4'
if rg -n -i "$STALE_PATTERN" "${CURRENT_SURFACES[@]}"; then
  fail "a current public surface contains stale release truth"
fi

echo "== Public truth: historical material is fail-closed labeled =="
require_text BIDI_COHERENCE_DELTA_CALCULUS.md \
  'Historical snapshot — not current release authority.'
require_text BIDI_CALCULUS_CORE.md \
  'Historical formal-design snapshot — not current release authority.'
require_text NATIVE_SELF_HOSTING_MANDATE.md \
  'Historical burn-down record — not current release authority.'
require_text docs/build/BUILD_STATE.md \
  'Historical branch ledger — not current release authority.'

echo "== Public truth: bibliography identity =="
sed -n 's/.*\\bibitem{\([^}]*\)}.*/\1/p' paper/arxiv/main.tex \
  | LC_ALL=C sort > build/public-bib-keys.txt
BIB_COUNT=$(awk 'END {print NR}' build/public-bib-keys.txt)
UNIQUE_BIB_COUNT=$(uniq build/public-bib-keys.txt | awk 'END {print NR}')
test "$BIB_COUNT" -ge 4 || fail "paper bibliography unexpectedly small: $BIB_COUNT"
test "$BIB_COUNT" = "$UNIQUE_BIB_COUNT" || fail "paper bibliography has duplicate keys"
test "$(grep -c '^henzinger1996$' build/public-bib-keys.txt)" = "1" || \
  fail "paper must carry exactly one Henzinger bibliography entry"

echo "PUBLIC_TRUTH_GATE_PASS"

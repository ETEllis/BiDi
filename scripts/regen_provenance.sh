#!/usr/bin/env bash
# Regenerates the CT0 canonical BLAKE3 provenance manifest deterministically
# from `git ls-files`, so the manifest is bound to the head it ships with.
#
# 2026-07-28 review, finding 4: the previous manifest claimed to cover "all
# tracked files" but was hand-maintained — it had drifted to 166 entries
# against 186 tracked files, omitted the BLAKE3 implementation it was
# produced by, and carried stale digests that nothing checked. A manifest
# nothing regenerates is a claim, not evidence.
#
# Exclusions (there is exactly one, and it is structural):
#   evidence/gates/CT0/blake3-manifest.txt — the manifest cannot contain its
#   own digest; including it would make the file unsatisfiable.
# Every other tracked file is covered, including this script.
#
# Usage: ./scripts/regen_provenance.sh [output-path]
set -euo pipefail

cd "$(dirname "$0")/.."

MANIFEST=evidence/gates/CT0/blake3-manifest.txt
OUT=${1:-$MANIFEST}
DIGEST=build/cdc_frontend_check

if [ ! -x "$DIGEST" ]; then
  echo "regen_provenance: $DIGEST not built (run scripts/verify.sh first)" >&2
  exit 1
fi

TMP=$(mktemp "${TMPDIR:-/tmp}/cdc-provenance.XXXXXX")
trap 'rm -f "$TMP"' EXIT

{
  echo "# CT0 canonical BLAKE3 manifest of every tracked file (DECISIONS D2 closed)."
  echo "# Regenerate with: ./scripts/regen_provenance.sh"
  echo "# Digests are produced by the vendored implementation itself:"
  echo "#   build/cdc_frontend_check digest-file <path>"
  echo "# Coverage is git ls-files in C-locale order, with exactly one"
  echo "# structural exclusion: this manifest, which cannot contain its own"
  echo "# digest. scripts/verify.sh gates both the path set and the bytes, so"
  echo "# adding or modifying any tracked file fails until this is rerun."
  echo "# The interim sha256-manifest.txt is retained unmodified as the"
  echo "# historical record of the pre-BLAKE3 era (DECISIONS D2)."
} > "$TMP"

git ls-files -z \
  | LC_ALL=C sort -z \
  | while IFS= read -r -d '' path; do
      case "$path" in
        "$MANIFEST") continue ;;
      esac
      [ -f "$path" ] || continue
      "$DIGEST" digest-file "$path"
    done >> "$TMP"

mkdir -p "$(dirname "$OUT")"
cp "$TMP" "$OUT"

ENTRIES=$(grep -c '^blake3:' "$OUT")
echo "provenance manifest: ${ENTRIES} entries -> ${OUT}"

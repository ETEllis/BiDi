#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

command -v node >/dev/null 2>&1 || {
  echo "node is required for the verified web console gate" >&2
  exit 1
}

echo "== Verified web console: source and generated-artifact contract =="
node --check ui/web/console/generate-verified-snapshot.mjs
node --check ui/web/console/test-console.mjs
node --check ui/web/console/browser-proof.mjs

# This test compares tracked receipts against fresh native runtime output before
# doing any regeneration. A stale snapshot therefore fails closed instead of
# being silently repaired by the verifier that is supposed to detect drift.
node ui/web/console/test-console.mjs

if [ -n "${CDC_BROWSER_DEBUG_ENDPOINT:-}" ]; then
  PROOF_DIR=${CDC_BROWSER_PROOF_DIR:-build/web-console-browser-proof}
  mkdir -p "$PROOF_DIR"
  node ui/web/console/browser-proof.mjs \
    "$CDC_BROWSER_DEBUG_ENDPOINT" "$PROOF_DIR"
else
  echo "browser endpoint not supplied; receipt/static gate passed"
  echo "set CDC_BROWSER_DEBUG_ENDPOINT to add live responsive browser proof"
fi

echo "WEB_CONSOLE_GATE_PASS"

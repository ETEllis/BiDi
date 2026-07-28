#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

./scripts/build_identity_3d.sh
./scripts/render_identity_motion.sh
./scripts/verify_identity_3d.sh --require-motion
./scripts/verify.sh
git diff --check

echo "MOBIUS_IDENTITY_FULL_RELEASE_PASS"

#!/usr/bin/env bash
# Product surface gate: the macOS app and the web console must stay locked to
# the canonical design tokens and to the runtime's real output. A UI that
# drifts from the identity system, or that renders invented data, fails here.
set -euo pipefail

cd "$(dirname "$0")/.."

TOKENS="ui/design/mobius-tokens.json"
SWIFT="ui/macos/CDCStudio/Sources/CDCStudio/DesignSystem/MobiusTokens.swift"
CONSOLE="ui/web/console/index.html"

echo "== Product surfaces: design-token parity =="
python3 - "$TOKENS" "$SWIFT" "$CONSOLE" <<'PY'
import json, re, sys

from pathlib import Path

tokens_path, swift_path, console_path = sys.argv[1:4]
tokens = json.load(open(tokens_path))
# The design system spans the token mirror and its component vocabulary.
design_dir = Path(swift_path).parent
swift = "\n".join(p.read_text() for p in sorted(design_dir.glob("*.swift")))
console = open(console_path).read()

# Every colour in the canonical token file must appear, verbatim, in both the
# Swift mirror (as 0xRRGGBB) and the console (as #rrggbb custom properties).
colors = {}
for group, entries in tokens["color"].items():
    for name, value in entries.items():
        if isinstance(value, str) and value.startswith("#"):
            colors[f"{group}.{name}"] = value.lower()

missing = []
for key, hex_value in colors.items():
    if f"0x{hex_value[1:]}" not in swift:
        missing.append(f"swift missing {key} ({hex_value})")
    if hex_value not in console.lower():
        missing.append(f"console missing {key} ({hex_value})")

# The identity tilt is the product's only rotation accent and is gated in the
# identity asset contract; all three surfaces must carry the same number.
tilt = str(tokens["geometry"]["identityTiltDegrees"])
if tilt not in swift:
    missing.append(f"swift missing identity tilt {tilt}")
if tilt not in console:
    missing.append(f"console missing identity tilt {tilt}")

# The ternary vocabulary must exist as three distinct, separately styled states
# on every surface — merging them is the failure this product exists to avoid.
for state, spec in tokens["ternary"].items():
    if state == "note":
        continue
    label = spec["label"]
    if label not in swift:
        missing.append(f"swift missing ternary label {label}")
    if label not in console:
        missing.append(f"console missing ternary label {label}")
    if spec["color"].lower() not in swift.lower() and \
       f"0x{spec['color'][1:]}" not in swift:
        missing.append(f"swift missing ternary colour for {state}")

if missing:
    for item in missing:
        print("token drift:", item, file=sys.stderr)
    raise SystemExit(1)

print(f"design tokens locked: {len(colors)} colours + tilt + ternary triad "
      f"across swift and web")
PY

echo
echo "== Product surfaces: console is self-contained and runtime-bound =="
python3 - "$CONSOLE" demo/replay.json <<'PY'
import json, re, sys

console_path, replay_path = sys.argv[1:3]
html = open(console_path).read()

# No external resource may be referenced: the console must render identically
# offline, and must not phone home from an operator's machine.
external = re.findall(r'(?:src|href)\s*=\s*"(?:https?:)?//[^"]+', html)
external += re.findall(r'@import\s+url\(', html)
if external:
    print("external references:", external, file=sys.stderr)
    raise SystemExit(1)

# The embedded record must be byte-identical to what the runtime emitted, so
# the console can never present data the toolchain did not produce.
match = re.search(r'<script type="application/json" id="replay-data">\n(.*?)\n</script>',
                  html, re.S)
if not match:
    print("console has no embedded replay-data block", file=sys.stderr)
    raise SystemExit(1)
embedded = match.group(1)
runtime = open(replay_path).read().rstrip("\n")
if embedded != runtime:
    print("embedded replay data differs from demo/replay.json", file=sys.stderr)
    raise SystemExit(1)
json.loads(embedded)

# Accessibility floor: a language, a title, a described figure, and motion that
# yields to the reader's preference.
for token in ('lang="en"', "<title>", 'role="img"', "prefers-reduced-motion"):
    if token not in html:
        print("console missing baseline requirement:", token, file=sys.stderr)
        raise SystemExit(1)

print("console self-contained, replay-bound, reduced-motion aware")
PY

echo
echo "== Product surfaces: macOS app structure =="
python3 - <<'PY'
from pathlib import Path
import sys

root = Path("ui/macos/CDCStudio")
required = [
    root / "Package.swift",
    root / "Sources/CDCStudio/CDCStudioApp.swift",
    root / "Sources/CDCStudio/DesignSystem/MobiusTokens.swift",
    root / "Sources/CDCStudio/DesignSystem/TernaryOutcome.swift",
    root / "Sources/CDCStudio/DesignSystem/DoubleCoverView.swift",
    root / "Sources/CDCStudio/Models/ReplayModel.swift",
    root / "Sources/CDCStudio/Services/ToolchainService.swift",
    root / "Sources/CDCStudio/Views/ExecutionView.swift",
    root / "Sources/CDCStudio/Views/OperatorView.swift",
    root / "Sources/CDCStudio/Views/StoreView.swift",
    root / "Sources/CDCStudio/Views/BridgeView.swift",
]
missing = [str(p) for p in required if not p.is_file()]
if missing:
    print("missing app sources:", missing, file=sys.stderr)
    raise SystemExit(1)

sources = list(root.rglob("*.swift"))
blob = "\n".join(p.read_text() for p in sources)

# The app is a surface over the shipped toolchain, never a reimplementation:
# it must invoke the real binary and must not carry placeholder data.
if "Process()" not in blob:
    print("app does not invoke the real cdc binary", file=sys.stderr)
    raise SystemExit(1)
for banned in ("TODO", "FIXME", "lorem", "Lorem", "placeholder data", "mockData"):
    if banned in blob:
        print("app contains placeholder marker:", banned, file=sys.stderr)
        raise SystemExit(1)

# No third-party dependencies: the product ships its own runtime.
package = (root / "Package.swift").read_text()
if ".package(" in package:
    print("app declares an external dependency", file=sys.stderr)
    raise SystemExit(1)

# Balanced brace/paren balance is a cheap structural signal on a platform we
# cannot compile here; it catches truncated or corrupted sources.
for path in sources:
    text = path.read_text()
    if text.count("{") != text.count("}"):
        print("unbalanced braces in", path, file=sys.stderr)
        raise SystemExit(1)
    if text.count("(") != text.count(")"):
        print("unbalanced parens in", path, file=sys.stderr)
        raise SystemExit(1)

print(f"macos app: {len(sources)} swift sources, no dependencies, "
      f"no placeholders, structurally balanced")
PY

# CDC Studio is a SwiftUI application, and SwiftUI ships only on Apple
# platforms. A Swift toolchain alone is not sufficient — the Linux CI runner
# has swift but no SwiftUI — so the build is attempted only on macOS. Off
# Apple platforms the structural gate above stands and the boundary is
# stated rather than faked (ui/README.md).
if [ "$(uname -s)" = "Darwin" ] && command -v swift >/dev/null 2>&1; then
  echo "macOS host with swift toolchain; building CDC Studio"
  (cd ui/macos/CDCStudio && swift build)
else
  echo "not an Apple platform (SwiftUI unavailable); CDC Studio is"
  echo "structurally gated here and compiled on a macOS host (see ui/README.md)"
fi

echo
echo "product surface gate: ok"

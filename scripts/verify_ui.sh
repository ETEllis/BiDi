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

# Inspect authored inputs only. `swift build` creates `.build/**/DerivedSources`
# beneath this tree; including those generated files made this gate
# history-dependent (a clean checkout passed once, then its own build output
# could fail the next run on compiler-generated text). Package.swift is an
# authored Swift source too, so keep it in the checked set explicitly.
sources = [
    root / "Package.swift",
    *sorted((root / "Sources").rglob("*.swift")),
    *sorted((root / "Tests").rglob("*.swift")),
]
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
# cannot compile here. Count authored Swift syntax, not braces inside strings
# (the receipt parser tests intentionally contain multiline JSON fixtures) or
# comments. The Darwin lane below remains the compiler authority.
def swift_code_only(text):
    out = []
    index = 0
    block_depth = 0
    length = len(text)
    while index < length:
        if block_depth:
            if text.startswith("/*", index):
                block_depth += 1
                index += 2
            elif text.startswith("*/", index):
                block_depth -= 1
                index += 2
            else:
                index += 1
            continue
        if text.startswith("//", index):
            newline = text.find("\n", index + 2)
            index = length if newline < 0 else newline + 1
            continue
        if text.startswith("/*", index):
            block_depth = 1
            index += 2
            continue
        if text.startswith('"""', index):
            closing = text.find('"""', index + 3)
            if closing < 0:
                raise ValueError("unterminated multiline string")
            index = closing + 3
            continue
        if text[index] == '"':
            index += 1
            escaped = False
            while index < length:
                char = text[index]
                index += 1
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    break
            else:
                raise ValueError("unterminated string")
            continue
        out.append(text[index])
        index += 1
    if block_depth:
        raise ValueError("unterminated block comment")
    return "".join(out)

for path in sources:
    text = path.read_text()
    try:
        code = swift_code_only(text)
    except ValueError as error:
        print(f"{error} in {path}", file=sys.stderr)
        raise SystemExit(1)
    if code.count("{") != code.count("}"):
        print("unbalanced braces in", path, file=sys.stderr)
        raise SystemExit(1)
    if code.count("(") != code.count(")"):
        print("unbalanced parens in", path, file=sys.stderr)
        raise SystemExit(1)

print(f"macos app: {len(sources)} swift sources, no dependencies, "
      f"no placeholders, structurally balanced")
PY

# Subprocess-drain contract (2026-07-28 review, finding 3). A SOURCE-level
# check, so it runs on every platform including the Linux gate: draining one
# pipe to EOF before starting the other deadlocks as soon as the child fills
# the undrained buffer. No compiler and no file count can catch that, so the
# pattern is gated directly.
python3 - <<'DRAINCHECK'
from pathlib import Path
import re

service = Path("ui/macos/CDCStudio/Sources/CDCStudio/Services/ToolchainService.swift")
text = service.read_text(encoding="utf-8")

# An unbounded read-to-EOF retains attacker-controlled output and couples the
# timeout to pipe closure. CDC Studio instead drains both pipes concurrently
# into explicit bounded collectors while process liveness owns the deadline.
if "readDataToEndOfFile" in text:
    raise SystemExit(
        "ToolchainService uses unbounded readDataToEndOfFile instead of the "
        "bounded streaming capture contract"
    )

for token, why in [
    ("DispatchGroup", "concurrent drain needs a completion group"),
    ("BoundedPipeCollector", "each stream needs a bounded incremental collector"),
    ("readabilityHandler", "pipes must drain concurrently while the child runs"),
    ("defaultStreamByteLimit = 1_048_576", "the per-stream byte cap must be explicit"),
    ("case truncated(limit: Int)", "capture truncation must be typed"),
    ("termination.wait(timeout:", "process liveness must own the timeout"),
    ("process.terminate()", "timeout must escalate to SIGTERM"),
    ("SIGKILL", "timeout must escalate to SIGKILL if SIGTERM is ignored"),
]:
    if token not in text:
        raise SystemExit(f"ToolchainService missing {token!r}: {why}")

tests = Path("ui/macos/CDCStudio/Tests/CDCStudioTests/ToolchainServiceTests.swift")
if not tests.is_file():
    raise SystemExit("the flooding-child counterexample is missing")
test_text = tests.read_text(encoding="utf-8")
for token in (
    "512 * 1024",
    "stderr was truncated",
    "timeout did not fire",
    "testChildClosingBothPipesCannotBypassProcessTimeout",
    "testPerStreamCaptureCapsAreTypedAndFailClosed",
    "testCancellationReservedBeforeLaunchCannotBeResetOrLost",
):
    if token not in test_text:
        raise SystemExit(f"counterexample missing {token!r}")

runtime_model = Path(
    "ui/macos/CDCStudio/Sources/CDCStudio/Models/RepositoryModel.swift"
).read_text(encoding="utf-8")
for token, why in [
    ("O_NOFOLLOW", "runtime capture must reject path-final symlinks"),
    ("Darwin.fstat", "runtime capture must bind checks to the opened descriptor"),
    ("maximumExecutableSize", "runtime capture must be explicitly bounded"),
    ("Darwin.mkdtemp", "prepared images need unique private directories"),
    ("mode_t(0o700)", "prepared executable permissions must be private"),
    ("PreparedRuntimeExecutable", "launches must use captured executable bytes"),
]:
    if token not in runtime_model:
        raise SystemExit(f"runtime preparation contract missing {token!r}: {why}")

for token, why in [
    ("RuntimeExecutableValidator.prepare", "every launch must prepare captured bytes"),
    ("defer { preparedRuntime.cleanup() }", "prepared bytes must be removed after each outcome"),
    ("preparedRuntime.executableURL", "the repository path must never be launched after capture"),
]:
    if token not in text:
        raise SystemExit(f"ToolchainService missing {token!r}: {why}")

repository_tests = Path(
    "ui/macos/CDCStudio/Tests/CDCStudioTests/RepositoryDiscoveryTests.swift"
).read_text(encoding="utf-8")
if "testPreparedRuntimeExecutesCapturedBytesAfterRepositoryPathReplacement" not in repository_tests:
    raise SystemExit("prepared-runtime path-replacement counterexample is missing")

receipt_parser = Path(
    "ui/macos/CDCStudio/Sources/CDCStudio/Services/U2ReceiptParser.swift"
).read_text(encoding="utf-8")
for token, why in [
    ("validateRecurrenceMeasurements", "reported recurrence data needs independent recomputation"),
    ("Darwin.fmod", "periodic coordinates must mirror the native runtime topology"),
    ("recurrence.relativeTolerance", "normalization must include the declared relative tolerance"),
    ("matchesRuntimeFloat", "reported and recomputed binary64 values need a narrow comparison"),
    ("producerVerified", "projected producer verification must match its reported diagnostics"),
    ("projected recurrence cannot carry an authoritative U2 claim", "maskless projections must remain held"),
]:
    if token not in receipt_parser:
        raise SystemExit(f"U2 receipt parser missing {token!r}: {why}")

receipt_tests = Path(
    "ui/macos/CDCStudio/Tests/CDCStudioTests/U2ReceiptParserTests.swift"
).read_text(encoding="utf-8")
for token in (
    "testRecurrenceReceiptMustMatchExecutableEndpointComputation",
    "1_000_000_000.0",
    "testRecurrenceRecomputationUsesAbsolutePlusRelativeTolerance",
    "testProjectedV1ReceiptRemainsHeldAndNonAuthoritativeWithoutEmittedMask",
    "forgedProducerMismatch",
    "testNonExecutableRelativeQuotientPlaceholderRemainsTypedNonAuthoritativeHold",
):
    if token not in receipt_tests:
        raise SystemExit(f"recurrence parser counterexample missing {token!r}")

pkg = Path("ui/macos/CDCStudio/Package.swift").read_text(encoding="utf-8")
if ".testTarget" not in pkg:
    raise SystemExit("Package.swift does not declare the test target")

ci = Path(".github/workflows/ci.yml").read_text(encoding="utf-8")
if "macos-14" not in ci or "swift test" not in ci:
    raise SystemExit(
        "a required macOS lane running swift build AND swift test is missing; "
        "the Linux structural gate is not sufficient on its own"
    )

print("subprocess drain contract: concurrent, bounded, counterexample present")
print("runtime preparation contract: descriptor-bound, private, counterexample present")
print("U2 recurrence receipt contract: independently recomputed, counterexamples present")
print("macOS compile lane: required in CI (macos-14)")
DRAINCHECK

# CDC Studio is a SwiftUI application, and SwiftUI ships only on Apple
# platforms. A Swift toolchain alone is not sufficient — the Linux CI runner
# has swift but no SwiftUI — so the build is attempted only on macOS. Off
# Apple platforms the structural and source-level gates above stand, and the
# compiler of record is the REQUIRED macos-14 CI lane. That lane exists
# because this gate alone let a non-compiling surface reach a green PR.
if [ "$(uname -s)" = "Darwin" ] && command -v swift >/dev/null 2>&1; then
  echo "macOS host with swift toolchain; building and testing CDC Studio"
  # The repository may live on a File Provider volume whose generated .build
  # products inherit FinderInfo/provenance xattrs. codesign correctly rejects
  # that detritus even when the authored sources are valid. Build into a fresh,
  # local scratch directory so the gate is reproducible and never strips xattrs
  # from user or source files.
  SWIFT_SCRATCH=$(mktemp -d "${TMPDIR:-/tmp}/cdcstudio-ui-gate.XXXXXX")
  cleanup_swift_scratch() {
    rm -rf -- "$SWIFT_SCRATCH"
  }
  trap cleanup_swift_scratch EXIT
  (cd ui/macos/CDCStudio && \
    swift build --scratch-path "$SWIFT_SCRATCH" && \
    swift test --scratch-path "$SWIFT_SCRATCH")
  cleanup_swift_scratch
  trap - EXIT
else
  echo "not an Apple platform (SwiftUI unavailable); CDC Studio is"
  echo "gated structurally here and compiled by the required macos-14 CI lane"
fi

echo
echo "product surface gate: ok"

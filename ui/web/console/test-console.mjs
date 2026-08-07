#!/usr/bin/env node
/** Deterministic data, semantics, and legacy-entry checks for the web console. */

import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";
import fs from "node:fs";
import path from "node:path";
import vm from "node:vm";

import {
  CAPTURES,
  buildSnapshot,
  runtimeReceiptText,
  validatesAnalysisBoundary,
} from "./generate-verified-snapshot.mjs";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "../../..");

function embeddedJson(html, elementId) {
  const escaped = elementId.replace(/[.*+?^${}()|[\]\\]/gu, "\\$&");
  const found = html.match(
    new RegExp(`<script type="application/json" id="${escaped}">\\s*([\\s\\S]*?)\\s*</script>`, "u"),
  );
  assert.ok(found, `missing embedded JSON block: ${elementId}`);
  return JSON.parse(found[1]);
}

const consoleHtml = fs.readFileSync(path.join(HERE, "index.html"), "utf8");
const demoHtml = fs.readFileSync(path.join(ROOT, "demo/index.html"), "utf8");
const browserProofSource = fs.readFileSync(path.join(HERE, "browser-proof.mjs"), "utf8");
const replay = JSON.parse(fs.readFileSync(path.join(ROOT, "demo/replay.json"), "utf8"));
const snapshot = JSON.parse(fs.readFileSync(path.join(HERE, "u2-snapshots.json"), "utf8"));

for (const [sourcePath, receiptPath] of CAPTURES) {
  assert.equal(fs.readFileSync(path.join(ROOT, receiptPath), "utf8"), runtimeReceiptText(sourcePath));
}
assert.deepEqual(snapshot, buildSnapshot(), "generated U2 snapshot is stale");
assert.deepEqual(embeddedJson(consoleHtml, "u2-snapshot-data"), snapshot);
assert.deepEqual(embeddedJson(consoleHtml, "replay-data"), replay);
assert.deepEqual(embeddedJson(demoHtml, "replay-data"), replay);
const executableScripts = [...consoleHtml.matchAll(/<script(?![^>]*type="application\/json")[^>]*>([\s\S]*?)<\/script>/gu)];
assert.ok(executableScripts.length > 0, "console executable script missing");
const executableSource = executableScripts.at(-1)[1];
new vm.Script(executableSource, { filename: "ui/web/console/index.html:inline" });

function extractEmbeddedAnalysisValidator(source) {
  const start = source.indexOf("function validatesAnalysisBoundary(record){");
  const end = source.indexOf("\n  function entry()", start);
  assert.ok(start >= 0 && end > start, "embedded analysis validator could not be isolated");
  return new vm.Script(`(${source.slice(start, end).trim()})`, {
    filename: "ui/web/console/index.html:validatesAnalysisBoundary",
  }).runInNewContext();
}

const embeddedAnalysisValidator = extractEmbeddedAnalysisValidator(executableSource);

const canonical = snapshot.entries.find((item) => item.id === "canonical-held");
const relative = snapshot.entries.find((item) => item.id === "relative-fixture");
assert.equal(canonical.receipt.status, "held");
assert.equal(canonical.receipt.multipliers, null);
assert.equal(canonical.receipt.monodromyDigest, null);
assert.equal(canonical.receipt.recurrence.authorizesMonodromy, false);
assert.equal(relative.role, "positive-relative-calibration-fixture");
assert.equal(relative.receipt.multipliers.length, 13);
assert.ok(relative.receipt.multipliers.every((item) => item.modulus === 1));

const expectedMethodDetail = {
  flow: {
    localMap: "explicit-euler-synchronous",
    jacobian: "analytic-exact",
    finiteDifferenceOracle: null,
  },
  commit: {
    kind: "scheduled-fixed-mode-reset",
    continuousDerivative: "identity-within-fixed-mode",
    saltation: "not-applicable",
  },
  nest: {
    localMap: "fixed-trit-overwrite",
    jacobian: "analytic-exact",
    childPrior: "overwrite-parent-belief",
  },
  guard: { binding: null, eventLocalization: null, saltation: null },
  spectrum: { solver: "validated-real-schur", ordering: "sorted-multipliers" },
};

function assertExpandedReceiptContract(entries) {
  const held = entries.find((item) => item.id === "canonical-held").receipt;
  const accepted = entries.find((item) => item.id === "relative-fixture").receipt;
  for (const record of [held, accepted]) {
    assert.deepEqual(record.methodDetail, expectedMethodDetail);
    assert.deepEqual(record.finiteDifference, { status: "not-run", step: null, residual: null });
    assert.deepEqual(record.eventBudget, { status: "not-applicable", limit: null, used: null });
    assert.deepEqual(record.neutralModeRemoval, {
      status: "not-requested",
      generatorDigest: null,
      removedModes: null,
    });
    assert.ok(Array.isArray(record.discreteState.initial));
    assert.ok(Array.isArray(record.discreteState.final));
    assert.equal(record.discreteState.initial.length, record.discreteState.final.length);
    assert.equal(record.initialState.length, record.dimension);
    assert.equal(record.finalState.length, record.dimension);
    assert.ok(record.initialState.every((value) => Number.isFinite(value)));
    assert.ok(record.finalState.every((value) => Number.isFinite(value)));
  }

  assert.equal(held.recurrence.discreteStateVerified, false);
  assert.equal(held.discreteState.verified, false);
  assert.notEqual(held.discreteState.initialDigest, held.discreteState.finalDigest);
  const heldChanges = held.discreteState.initial.flatMap((initial, index) => {
    const final = held.discreteState.final[index];
    return initial.mode === final.mode ? [] : [{ initial, final }];
  });
  assert.deepEqual(
    heldChanges.map(({ initial }) => initial.cell),
    ["agent.a", "agent.b", "agent.c"],
  );
  assert.deepEqual(
    heldChanges.map(({ final }) => final.mode),
    ["latched:+", "latched:+", "latched:0"],
  );
  assert.equal(held.spectrumDiagnostics, null);
  assert.equal(held.monodromyDigest, null);
  assert.equal(held.monodromy, null);

  assert.equal(accepted.recurrence.discreteStateVerified, true);
  assert.equal(accepted.discreteState.verified, true);
  assert.equal(accepted.discreteState.initialDigest, accepted.discreteState.finalDigest);
  assert.equal(accepted.classification, "marginal");
  assert.equal(accepted.multipliers.length, 13);
  assert.ok(
    accepted.multipliers.every(
      (multiplier) =>
        multiplier.mode === "physical" &&
        multiplier.real === 1 &&
        multiplier.imag === 0 &&
        multiplier.modulus === 1,
    ),
  );
  assert.deepEqual(accepted.spectrumDiagnostics, {
    spectralRadius: 1,
    schur: {
      reconstructionResidual: 0,
      orthogonalityResidual: 0,
      triangularResidual: 0,
      validationTolerance: 1e-10,
    },
  });
  assert.match(accepted.monodromyDigest, /^blake3:[0-9a-f]{64}$/u);
  assert.equal(accepted.monodromy.length, accepted.dimension * accepted.dimension);
  assert.ok(accepted.monodromy.every((item) => Number.isFinite(item)));
  const restoration = accepted.recurrence.restoration;
  assert.equal(
    restoration.derivativeDigest,
    "blake3:737995db71af62fc1f0e0beb6b2153bd7ddd20eacb8b745eaf4a413060123223",
  );
  assert.equal(restoration.derivative.length, accepted.dimension * accepted.dimension);
  assert.ok(
    restoration.derivative.every((value, index) => {
      const row = Math.floor(index / accepted.dimension);
      const column = index % accepted.dimension;
      return value === (row === column ? 1 : 0);
    }),
  );
}

assertExpandedReceiptContract(snapshot.entries);
const currentSpectralHoldReasons = [
  "spectral-backend-unavailable",
  "spectral-backend-failed",
  "variational-validation-failed",
];
const currentTangentHoldReasons = [
  "recurrence-mode-mismatch",
  "recurrence-residual",
  "undeclared-quotient",
];
function makeSpectralHold(reason = currentSpectralHoldReasons[0]) {
  const record = JSON.parse(JSON.stringify(relative.receipt));
  return Object.assign(record, {
    status: "held",
    reason,
    analysis: "monodromy",
    multipliers: null,
    spectrumDiagnostics: null,
    backend: null,
    classification: "held",
  });
}
function makeUnexecutableRelativeHold() {
  const record = JSON.parse(JSON.stringify(relative.receipt));
  Object.assign(record, {
    status: "held",
    reason: "undeclared-quotient",
    analysis: "tangent",
    monodromyDigest: null,
    monodromy: null,
    multipliers: null,
    spectrumDiagnostics: null,
    backend: null,
    classification: "held",
  });
  Object.assign(record.recurrence, {
    kind: "relative",
    scope: "relative",
    residual: 0,
    normalizedResidual: 0,
    verified: false,
    authorizesMonodromy: false,
    discreteStateVerified: false,
    restorationDerivativeApplied: false,
    restoration: null,
  });
  record.discreteState.verified = false;
  return record;
}
function makeProjectedHold() {
  const record = JSON.parse(JSON.stringify(canonical.receipt));
  record.reason = "undeclared-quotient";
  record.recurrence.scope = "projected";
  return record;
}
function assertAnalysisBoundaryContract(label, validator) {
  assert.equal(validator(canonical.receipt), true, `${label}: canonical recurrence hold rejected`);
  assert.equal(validator(relative.receipt), true, `${label}: accepted relative record rejected`);
  assert.equal(
    validator(makeUnexecutableRelativeHold()),
    true,
    `${label}: exact nonauthoritative undeclared-relative hold rejected`,
  );
  assert.equal(validator(makeProjectedHold()), true, `${label}: exact projected hold rejected`);
  for (const reason of currentSpectralHoldReasons) {
    assert.equal(validator(makeSpectralHold(reason)), true, `${label}: current hold ${reason} rejected`);
  }
  for (const reason of currentTangentHoldReasons) {
    const record = JSON.parse(JSON.stringify(canonical.receipt));
    record.reason = reason;
    assert.equal(validator(record), true, `${label}: current tangent hold ${reason} rejected`);
  }
  for (const reason of ["spectral-backend-unavailable", "spectrum-residual", "unknown-hold"]) {
    const record = JSON.parse(JSON.stringify(canonical.receipt));
    record.reason = reason;
    assert.equal(validator(record), false, `${label}: tangent stage accepted ${reason}`);
  }
  const tolerance = relative.receipt.tolerances.schur;
  const withinTolerance = JSON.parse(JSON.stringify(relative.receipt));
  withinTolerance.multipliers[0].real += tolerance / 2;
  withinTolerance.multipliers[0].imag = tolerance / 2;
  withinTolerance.multipliers[0].modulus += tolerance / 2;
  withinTolerance.spectrumDiagnostics.spectralRadius += tolerance / 2;
  assert.equal(validator(withinTolerance), true, `${label}: rejected in-tolerance +1 fixture drift`);
  const acceptedCounterexamples = [
    ["huge residual paired with false zero normalization", (record) => {
      record.recurrence.residual = 1e9;
      record.recurrence.normalizedResidual = 0;
    }],
    ["mismatched normalized residual", (record) => { record.recurrence.normalizedResidual = 0.5; }],
    ["endpoint drift hidden by unchanged recurrence summary", (record) => { record.finalState[0] += 0.25; }],
    ["coherent endpoint and displacement forgery", (record) => {
      record.finalState[6] += 0.25;
      record.recurrence.restoration.displacement += 0.25;
    }],
    ["coherent two-pi restoration forgery", (record) => {
      record.finalState[6] = 2 * Math.PI;
      record.recurrence.restoration.displacement = 2 * Math.PI;
    }],
    ["restoration displacement drift", (record) => {
      record.recurrence.restoration.displacement += 0.25;
    }],
    ["restoration action drift", (record) => {
      record.recurrence.restoration.action = "phase-translation";
    }],
    ["restoration coordinate mismatch", (record) => {
      record.recurrence.restoration.coordinate = record.coordinates[5].name;
    }],
    ["equivariance witness id drift", (record) => {
      record.recurrence.restoration.equivarianceWitness.id = "plausible-two-turn-cover";
    }],
    ["equivariance witness field gain drift", (record) => {
      record.recurrence.restoration.equivarianceWitness.fieldGain = 1;
    }],
    ["equivariance witness count drift", (record) => {
      record.recurrence.restoration.equivarianceWitness.fieldCellCount = 2;
    }],
    ["equivariance incident count drift", (record) => {
      record.recurrence.restoration.equivarianceWitness.incidentChannelCount = 1;
    }],
    ["equivariance mutating count drift", (record) => {
      record.recurrence.restoration.equivarianceWitness.mutatingStepCount = 1;
    }],
    ["section witness id drift", (record) => {
      record.recurrence.restoration.sectionWitness.id = "plausible-returned-restored";
    }],
    ["coherent section and universal winding drift", (record) => {
      record.recurrence.restoration.sectionWitness.winding = 1;
      record.universal.winding = 1;
    }],
    ["section projection drift", (record) => {
      record.recurrence.restoration.sectionWitness.projection = "projected";
    }],
    ["section sheet drift", (record) => {
      record.recurrence.restoration.sectionWitness.sheet = "returned";
    }],
    ["restoration derivative byte drift", (record) => {
      record.recurrence.restoration.derivative[1] = 1;
    }],
    ["restoration derivative digest drift", (record) => {
      record.recurrence.restoration.derivativeDigest = `blake3:${"0".repeat(64)}`;
    }],
    ["missing restoration derivative bytes", (record) => {
      record.recurrence.restoration.derivative = null;
    }],
    ["stable label over +1 fixture", (record) => { record.classification = "stable"; }],
    ["unstable label over +1 fixture", (record) => { record.classification = "unstable"; }],
    ["altered real component with lying modulus", (record) => { record.multipliers[0].real = 0.75; }],
    ["altered imaginary component with lying modulus", (record) => { record.multipliers[0].imag = 0.25; }],
    ["non-unit coherent stable spectrum", (record) => {
      record.classification = "stable";
      record.spectrumDiagnostics.spectralRadius = 0.9;
      record.multipliers.forEach((multiplier) => {
        multiplier.real = 0.9; multiplier.imag = 0; multiplier.modulus = 0.9;
      });
    }],
    ["altered spectral radius", (record) => { record.spectrumDiagnostics.spectralRadius = 0.9; }],
    ["gauge multiplier", (record) => { record.multipliers[0].mode = "gauge"; }],
    ["truncated multiplier set", (record) => { record.multipliers.pop(); }],
  ];
  for (const [description, mutate] of acceptedCounterexamples) {
    const record = JSON.parse(JSON.stringify(relative.receipt));
    mutate(record);
    assert.equal(validator(record), false, `${label}: accepted ${description}`);
  }
  const restorationPeriodMutation = JSON.parse(JSON.stringify(relative.receipt));
  restorationPeriodMutation.coordinates[6].period = 2 * Math.PI;
  assert.equal(
    validator(restorationPeriodMutation),
    false,
    `${label}: accepted a periodic coordinate for the unwrapped cover restoration`,
  );
  const canonicalPeriodMutation = JSON.parse(JSON.stringify(canonical.receipt));
  canonicalPeriodMutation.coordinates[6].period = 2 * Math.PI;
  assert.equal(
    validator(canonicalPeriodMutation),
    false,
    `${label}: accepted a coordinate-period mutation that hides the canonical residual`,
  );
  const undeclaredNearMisses = [
    ["forged residual", (record) => { record.recurrence.residual = 1e9; }],
    ["forged normalized residual", (record) => { record.recurrence.normalizedResidual = 0.5; }],
    ["verified default result", (record) => {
      record.recurrence.discreteStateVerified = true;
      record.recurrence.verified = true;
      record.discreteState.verified = true;
    }],
    ["authorizing default result", (record) => { record.recurrence.authorizesMonodromy = true; }],
    ["invented restoration", (record) => { record.recurrence.restoration = {}; }],
    ["applied missing derivative", (record) => {
      record.recurrence.restorationDerivativeApplied = true;
    }],
  ];
  for (const [description, mutate] of undeclaredNearMisses) {
    const record = makeUnexecutableRelativeHold();
    mutate(record);
    assert.equal(validator(record), false, `${label}: accepted undeclared-relative ${description}`);
  }
  const projectedNearMisses = [
    ["locally verified projection reported false", (record) => {
      record.recurrence.normalizedResidual = 0;
      record.recurrence.discreteStateVerified = true;
      record.discreteState.verified = true;
    }],
    ["verified projection", (record) => {
      record.recurrence.normalizedResidual = 0;
      record.recurrence.discreteStateVerified = true;
      record.recurrence.verified = true;
      record.discreteState.verified = true;
    }],
    ["authorizing projection", (record) => { record.recurrence.authorizesMonodromy = true; }],
  ];
  for (const [description, mutate] of projectedNearMisses) {
    const record = makeProjectedHold();
    mutate(record);
    assert.equal(validator(record), false, `${label}: accepted projected ${description}`);
  }
  const counterexamples = [
    ["non-current spectrum-residual reason", (record) => { record.reason = "spectrum-residual"; }],
    ["null monodromy digest", (record) => { record.monodromyDigest = null; }],
    ["empty monodromy digest", (record) => { record.monodromyDigest = ""; }],
    ["null monodromy matrix", (record) => { record.monodromy = null; }],
    ["empty monodromy matrix", (record) => { record.monodromy = []; }],
    ["short monodromy matrix", (record) => { record.monodromy = [1]; }],
    ["non-finite monodromy matrix", (record) => { record.monodromy[0] = Number.POSITIVE_INFINITY; }],
    ["unapplied relative derivative", (record) => { record.recurrence.restorationDerivativeApplied = false; }],
    ["missing relative restoration", (record) => { record.recurrence.restoration = null; }],
    ["unverified equivariance", (record) => { record.recurrence.restoration.equivarianceWitness.verified = false; }],
    ["unverified section", (record) => { record.recurrence.restoration.sectionWitness.verified = false; }],
    ["wrong restoration derivative shape", (record) => { record.recurrence.restoration.derivativeRows -= 1; }],
    ["unverified discrete state", (record) => { record.recurrence.discreteStateVerified = false; }],
    ["tangent-stage spectral hold", (record) => { record.analysis = "tangent"; }],
    ["multiplier leakage", (record) => { record.multipliers = relative.receipt.multipliers; }],
    ["backend leakage", (record) => { record.backend = "lapack-dgees"; }],
    ["diagnostic leakage", (record) => { record.spectrumDiagnostics = relative.receipt.spectrumDiagnostics; }],
  ];
  for (const [description, mutate] of counterexamples) {
    const record = makeSpectralHold();
    mutate(record);
    assert.equal(validator(record), false, `${label}: accepted ${description}`);
  }
}

assertAnalysisBoundaryContract("generator validator", validatesAnalysisBoundary);
assertAnalysisBoundaryContract("embedded validator", embeddedAnalysisValidator);
for (const mutate of [
  (entries) => delete entries[0].receipt.discreteState,
  (entries) => delete entries[1].receipt.spectrumDiagnostics.schur.triangularResidual,
  (entries) => {
    entries[0].receipt.methodDetail.flow.localMap = "drifted-method";
  },
  (entries) => {
    entries[1].receipt.recurrence.discreteStateVerified = false;
  },
  (entries) => delete entries[1].receipt.monodromy,
]) {
  const changed = JSON.parse(JSON.stringify(snapshot.entries));
  mutate(changed);
  assert.throws(() => assertExpandedReceiptContract(changed));
}

function rgb(hex) {
  return [1, 3, 5].map((index) => Number.parseInt(hex.slice(index, index + 2), 16));
}
function blend(foreground, background, alpha) {
  return foreground.map((channel, index) => channel * alpha + background[index] * (1 - alpha));
}
function luminance(color) {
  const channels = color.map((channel) => {
    const value = channel / 255;
    return value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4;
  });
  return channels[0] * 0.2126 + channels[1] * 0.7152 + channels[2] * 0.0722;
}
function contrast(first, second) {
  const values = [luminance(first), luminance(second)].sort((left, right) => right - left);
  return (values[0] + 0.05) / (values[1] + 0.05);
}
const paper = rgb("#f8fbff");
const raised = rgb("#122a52");
for (const alpha of [0.58, 0.64, 0.72]) {
  assert.ok(contrast(blend(paper, raised, alpha), raised) >= 4.5, `text opacity ${alpha} fails WCAG AA`);
}
assert.ok(consoleHtml.includes(".state-held{color:var(--paper)!important}"));
assert.ok(!consoleHtml.includes("font-size:9.5px"));
for (const token of [
  ".source-option span{display:block;margin-top:6px;color:var(--text-muted);font-size:11.5px}",
  ".fact dt{margin:0 0 5px;color:var(--text-muted);font-size:11.5px}",
  "font:500 11.5px/1.5 monospace",
  "zoomEquivalentPercent: 200",
  "normalContrastSamples",
]) {
  assert.ok(
    consoleHtml.includes(token) || browserProofSource.includes(token),
    `accessibility refinement token missing: ${token}`,
  );
}
for (const token of [
  'evidenceSection("discrete-state"',
  'dataset.evidence="discrete-changes"',
  'evidenceSection("spectrum-diagnostics"',
  'dataset.evidence="method-contract"',
  "Discrete-mode recurrence receipt",
  "Validated Schur diagnostics",
  "Method and placeholder contract",
  "none · explicit placeholder",
  "Spectrum held · ",
  "resolve spectral hold before multiplier claims",
]) {
  assert.ok(consoleHtml.includes(token), `expanded receipt UI token missing: ${token}`);
}
assert.ok(browserProofSource.includes("fatalStateHidden"));
assert.ok(!browserProofSource.includes("fatalHidden"));

const lowered = consoleHtml.toLowerCase();
assert.ok(lowered.includes("verified replay snapshot"));
assert.ok(lowered.includes("abi 1.5"));
assert.ok(!lowered.includes("abi 1.2"));
assert.ok(
  lowered.includes("source → u1 closure → tangent → recurrence → spectrum → receipt / paper"),
);
for (const token of [
  'role="tablist"',
  'aria-live="polite"',
  'aria-label="select recorded analysis"',
  "prefers-reduced-motion",
  "prefers-contrast",
  "320px",
  "768px",
  "1024px",
  "1440px",
]) {
  assert.ok(consoleHtml.includes(token), `missing console contract token: ${token}`);
}

assert.ok(demoHtml.includes("../ui/web/console/index.html"));
assert.ok(demoHtml.includes("location.replace"));
assert.ok(!demoHtml.toLowerCase().includes("renderbridge"));
assert.ok(!demoHtml.toLowerCase().includes("pipeline"));
console.log("web console contract: receipt-bound, ABI 1.5, accessible, legacy entry thin");

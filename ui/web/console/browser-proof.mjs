#!/usr/bin/env node
/**
 * Browser-level responsive and interaction proof for the self-contained console.
 *
 * Usage:
 *   node browser-proof.mjs http://127.0.0.1:9223 /tmp/cdc-web-proof
 *
 * Start Chrome with `--headless=new --remote-debugging-port=9223` first.
 */

import assert from "node:assert/strict";
import { fileURLToPath, pathToFileURL } from "node:url";
import fs from "node:fs";
import path from "node:path";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const endpoint = process.argv[2] || "http://127.0.0.1:9223";
const outputDirectory = path.resolve(process.argv[3] || "/tmp/cdc-web-proof");
fs.mkdirSync(outputDirectory, { recursive: true });

async function openTarget() {
  const target = await fetch(`${endpoint}/json/new?about:blank`, { method: "PUT" }).then((response) => {
    if (!response.ok) throw new Error(`could not create Chrome target: ${response.status}`);
    return response.json();
  });
  const socket = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((resolve, reject) => {
    socket.addEventListener("open", resolve, { once: true });
    socket.addEventListener("error", reject, { once: true });
  });

  let sequence = 0;
  const pending = new Map();
  const listeners = new Map();
  socket.addEventListener("message", (message) => {
    const payload = JSON.parse(String(message.data));
    if (payload.id) {
      const callback = pending.get(payload.id);
      if (!callback) return;
      pending.delete(payload.id);
      if (payload.error) callback.reject(new Error(payload.error.message));
      else callback.resolve(payload.result || {});
      return;
    }
    for (const callback of listeners.get(payload.method) || []) callback(payload.params || {});
  });

  function send(method, params = {}) {
    sequence += 1;
    const id = sequence;
    const promise = new Promise((resolve, reject) => pending.set(id, { resolve, reject }));
    socket.send(JSON.stringify({ id, method, params }));
    return promise;
  }

  function once(method, timeout = 5000) {
    return new Promise((resolve, reject) => {
      const callbacks = listeners.get(method) || [];
      const timer = setTimeout(() => {
        listeners.set(
          method,
          callbacks.filter((item) => item !== handler),
        );
        reject(new Error(`timed out waiting for ${method}`));
      }, timeout);
      function handler(params) {
        clearTimeout(timer);
        listeners.set(
          method,
          (listeners.get(method) || []).filter((item) => item !== handler),
        );
        resolve(params);
      }
      callbacks.push(handler);
      listeners.set(method, callbacks);
    });
  }

  return { socket, send, once };
}

const browser = await openTarget();
const exceptions = [];
await browser.send("Page.enable");
await browser.send("Runtime.enable");
await browser.send("Log.enable");

// Attach persistent event capture after enabling the domains.
browser.socket.addEventListener("message", (message) => {
  const payload = JSON.parse(String(message.data));
  if (payload.method === "Runtime.exceptionThrown") {
    exceptions.push(payload.params.exceptionDetails?.text || "runtime exception");
  }
  if (payload.method === "Log.entryAdded" && payload.params.entry?.level === "error") {
    exceptions.push(payload.params.entry.text || "console error");
  }
});

const cases = [
  { id: "1440-canonical-source", width: 1440, height: 900, source: "canonical-held", stage: "source" },
  { id: "1024-canonical-recurrence", width: 1024, height: 820, source: "canonical-held", stage: "recurrence" },
  { id: "768-relative-spectrum", width: 768, height: 900, source: "relative-fixture", stage: "spectrum" },
  { id: "1024-relative-receipt", width: 1024, height: 820, source: "relative-fixture", stage: "receipt", openMethodContract: true },
  {
    id: "320-canonical-spectrum-200pct-equivalent",
    width: 320,
    physicalWidth: 640,
    zoomEquivalentPercent: 200,
    deviceScaleFactor: 2,
    height: 780,
    source: "canonical-held",
    stage: "spectrum",
    keyboard: true,
  },
];

const report = [];
const pageUrl = pathToFileURL(path.join(HERE, "index.html")).href;
const compiledPaperPath = path.resolve(HERE, "../../../paper/arxiv/main.pdf");
assert.ok(fs.existsSync(compiledPaperPath), "compiled paper is missing from the verified checkout");

for (const testCase of cases) {
  await browser.send("Emulation.setEmulatedMedia", { media: "screen", features: [] });
  await browser.send("Emulation.setDeviceMetricsOverride", {
    width: testCase.width,
    height: testCase.height,
    deviceScaleFactor: testCase.deviceScaleFactor || 1,
    mobile: false,
  });
  const loaded = browser.once("Page.loadEventFired");
  await browser.send("Page.navigate", { url: pageUrl });
  await loaded;

  if (testCase.keyboard) {
    await browser.send("Runtime.evaluate", {
      expression: `document.querySelector('[data-source="${testCase.source}"]').click(); document.getElementById('tab-source').focus();`,
    });
    for (let index = 0; index < 4; index += 1) {
      await browser.send("Input.dispatchKeyEvent", { type: "keyDown", key: "ArrowRight", code: "ArrowRight" });
      await browser.send("Input.dispatchKeyEvent", { type: "keyUp", key: "ArrowRight", code: "ArrowRight" });
    }
  } else {
    await browser.send("Runtime.evaluate", {
      expression: `document.querySelector('[data-source="${testCase.source}"]').click(); document.querySelector('[data-stage="${testCase.stage}"]').click();`,
    });
  }
  if (testCase.openMethodContract) {
    await browser.send("Runtime.evaluate", {
      expression: `document.querySelector('[data-evidence="method-contract"]').open = true;`,
    });
  }

  const evaluated = await browser.send("Runtime.evaluate", {
    expression: `(() => {
      const offenders = [...document.querySelectorAll('body *')].filter((element) => {
        const box = element.getBoundingClientRect();
        const style = getComputedStyle(element);
        return box.width > 0 && style.position !== 'fixed' && (box.left < -1 || box.right > innerWidth + 1);
      }).slice(0, 12).map((element) => ({
        tag: element.tagName.toLowerCase(),
        id: element.id,
        className: String(element.className).slice(0, 80),
        bounds: [element.getBoundingClientRect().left, element.getBoundingClientRect().right]
      }));
      const active = document.querySelector('.orbit-tab[aria-selected="true"]');
      return {
        innerWidth,
        documentWidth: document.documentElement.scrollWidth,
        bodyWidth: document.body.scrollWidth,
        devicePixelRatio,
        offenders,
        fatalStateHidden: document.getElementById('fatal-state').hidden,
        claim: document.getElementById('claim-title').textContent.trim(),
        stage: active && active.dataset.stage,
        focusedStage: document.activeElement && document.activeElement.dataset && document.activeElement.dataset.stage,
        source: document.querySelector('.source-option[aria-checked="true"]').dataset.source,
        stageStatus: document.getElementById('stage-status').textContent.trim(),
        relativeMultiplierRows: document.querySelectorAll('.spectrum-layout tbody tr').length,
        hasUnitFigure: Boolean(document.querySelector('.unit-figure')),
        hasNotEmittedCopy: document.getElementById('stage-body').textContent.includes('Spectrum not emitted'),
        hasDiscreteStateEvidence: Boolean(document.querySelector('[data-evidence="discrete-state"]')),
        discreteChangeRows: document.querySelectorAll('[data-evidence="discrete-changes"] tbody tr').length,
        discreteEvidenceText: (document.querySelector('[data-evidence="discrete-state"]') || {}).textContent || '',
        hasSpectrumDiagnostics: Boolean(document.querySelector('[data-evidence="spectrum-diagnostics"]')),
        spectrumDiagnosticsText: (document.querySelector('[data-evidence="spectrum-diagnostics"]') || {}).textContent || '',
        hasMethodContract: Boolean(document.querySelector('[data-evidence="method-contract"]')),
        methodContractOpen: Boolean(document.querySelector('[data-evidence="method-contract"][open]')),
        methodContractText: (document.querySelector('[data-evidence="method-contract"]') || {}).textContent || '',
        compiledPaperHref: (document.querySelector('.proof-link[href$="paper/arxiv/main.pdf"]') || {}).getAttribute?.('href') || null
      };
    })()`,
    returnByValue: true,
  });
  const metrics = evaluated.result.value;
  assert.ok(metrics.documentWidth <= metrics.innerWidth, `${testCase.id}: document overflow`);
  assert.ok(metrics.bodyWidth <= metrics.innerWidth, `${testCase.id}: body overflow`);
  assert.deepEqual(metrics.offenders, [], `${testCase.id}: elements escape the viewport`);
  assert.equal(metrics.fatalStateHidden, true, `${testCase.id}: receipt validation failed`);
  assert.equal(metrics.stage, testCase.stage, `${testCase.id}: stage interaction failed`);
  assert.equal(metrics.source, testCase.source, `${testCase.id}: source interaction failed`);
  assert.ok(!metrics.claim.toLowerCase().includes("loading"), `${testCase.id}: claim did not render`);
  assert.equal(
    metrics.compiledPaperHref,
    "../../../paper/arxiv/main.pdf",
    `${testCase.id}: compiled paper route is unavailable`,
  );
  if (testCase.keyboard) assert.equal(metrics.focusedStage, testCase.stage, "keyboard focus did not follow stage");
  if (testCase.zoomEquivalentPercent === 200) {
    assert.equal(metrics.devicePixelRatio, 2, "200% equivalent proof did not use the 2x physical/CSS scale");
    assert.equal(testCase.physicalWidth / testCase.width, 2, "200% equivalent viewport ratio drifted");
  }
  if (testCase.source === "relative-fixture" && testCase.stage === "spectrum") {
    assert.equal(metrics.relativeMultiplierRows, 13, "relative fixture lost multiplier table");
    assert.equal(metrics.hasUnitFigure, true, "relative fixture lost unit-circle equivalent");
    assert.equal(metrics.hasSpectrumDiagnostics, true, "relative fixture lost Schur diagnostics");
    for (const token of ["Spectral radius", "Reconstruction residual", "Orthogonality residual", "Triangular residual", "Validation tolerance"]) {
      assert.ok(metrics.spectrumDiagnosticsText.includes(token), `relative fixture diagnostics lost ${token}`);
    }
  }
  if (testCase.source === "canonical-held" && testCase.stage === "recurrence") {
    assert.equal(metrics.hasDiscreteStateEvidence, true, "canonical recurrence lost discrete-state evidence");
    assert.equal(metrics.discreteChangeRows, 3, "canonical recurrence lost the three changed modes");
    assert.ok(metrics.discreteEvidenceText.includes("Discrete-mode verifiednot verified"), "canonical discrete-mode hold is not explicit");
    assert.ok(metrics.discreteEvidenceText.includes("Digest relationchanged"), "canonical digest change is not explicit");
  }
  if (testCase.openMethodContract) {
    assert.equal(metrics.hasMethodContract, true, "receipt lost the method contract");
    assert.equal(metrics.methodContractOpen, true, "method contract proof did not open its detail");
    for (const token of ["Flow local map", "Finite-difference oracle", "Commit saltation", "Guard event localization", "Guard saltation", "explicit placeholder", "Event budget", "Neutral-mode removal", "Spectrum solver"]) {
      assert.ok(metrics.methodContractText.includes(token), `method contract lost ${token}`);
    }
  }
  if (testCase.source === "canonical-held" && testCase.stage === "spectrum") {
    assert.equal(metrics.hasUnitFigure, false, "canonical hold fabricated a spectrum figure");
    assert.equal(metrics.hasNotEmittedCopy, true, "canonical hold omitted the no-spectrum reason");
  }

  const layout = await browser.send("Page.getLayoutMetrics");
  const height = Math.ceil(layout.cssContentSize.height);
  const screenshot = await browser.send("Page.captureScreenshot", {
    format: "png",
    fromSurface: true,
    captureBeyondViewport: true,
    clip: { x: 0, y: 0, width: testCase.width, height, scale: 1 },
  });
  const screenshotPath = path.join(outputDirectory, `${testCase.id}.png`);
  fs.writeFileSync(screenshotPath, Buffer.from(screenshot.data, "base64"));
  report.push({ ...testCase, contentHeight: height, screenshotPath, ...metrics });
}

// Execute the validator embedded in the shipping HTML against adversarial
// spectral-hold records. This is intentionally independent of the Node
// generator validator so either copy drifting fails browser proof.
const analysisBoundaryEvaluation = await browser.send("Runtime.evaluate", {
  expression: `(() => {
    const script = [...document.scripts].find((item) => item.textContent.includes('function validatesAnalysisBoundary(record){'));
    const start = script.textContent.indexOf('function validatesAnalysisBoundary(record){');
    const end = script.textContent.indexOf(String.fromCharCode(10) + '  function entry()', start);
    const validator = (0, eval)('(' + script.textContent.slice(start, end).trim() + ')');
    const data = JSON.parse(document.getElementById('u2-snapshot-data').textContent);
    const accepted = data.entries.find((item) => item.id === 'relative-fixture').receipt;
    const canonical = data.entries.find((item) => item.id === 'canonical-held').receipt;
    const currentReasons = ['spectral-backend-unavailable', 'spectral-backend-failed', 'variational-validation-failed'];
    const currentTangentReasons = ['recurrence-mode-mismatch', 'recurrence-residual', 'undeclared-quotient'];
    function make(reason = currentReasons[0]) {
      const record = JSON.parse(JSON.stringify(accepted));
      return Object.assign(record, {
        status: 'held', reason, analysis: 'monodromy', multipliers: null,
        spectrumDiagnostics: null, backend: null, classification: 'held'
      });
    }
    function makeUnexecutableRelativeHold() {
      const record = JSON.parse(JSON.stringify(accepted));
      Object.assign(record, {
        status: 'held', reason: 'undeclared-quotient', analysis: 'tangent',
        monodromyDigest: null, monodromy: null, multipliers: null,
        spectrumDiagnostics: null, backend: null, classification: 'held'
      });
      Object.assign(record.recurrence, {
        kind: 'relative', scope: 'relative', residual: 0, normalizedResidual: 0,
        verified: false, authorizesMonodromy: false, discreteStateVerified: false,
        restorationDerivativeApplied: false, restoration: null
      });
      record.discreteState.verified = false;
      return record;
    }
    function makeProjectedHold() {
      const record = JSON.parse(JSON.stringify(canonical));
      record.reason = 'undeclared-quotient';
      record.recurrence.scope = 'projected';
      return record;
    }
    function rejected(mutator) {
      const record = make(); mutator(record); return validator(record) === false;
    }
    function rejectedAccepted(mutator) {
      const record = JSON.parse(JSON.stringify(accepted)); mutator(record); return validator(record) === false;
    }
    function rejectedCanonical(mutator) {
      const record = JSON.parse(JSON.stringify(canonical)); mutator(record); return validator(record) === false;
    }
    function rejectedUnexecutable(mutator) {
      const record = makeUnexecutableRelativeHold(); mutator(record); return validator(record) === false;
    }
    function rejectedProjected(mutator) {
      const record = makeProjectedHold(); mutator(record); return validator(record) === false;
    }
    const withinTolerance = JSON.parse(JSON.stringify(accepted));
    const tolerance = withinTolerance.tolerances.schur;
    withinTolerance.multipliers[0].real += tolerance / 2;
    withinTolerance.multipliers[0].imag = tolerance / 2;
    withinTolerance.multipliers[0].modulus += tolerance / 2;
    withinTolerance.spectrumDiagnostics.spectralRadius += tolerance / 2;
    return {
      currentReasons: currentReasons.map((reason) => validator(make(reason))),
      currentTangentReasons: currentTangentReasons.map((reason) => {
        const record = JSON.parse(JSON.stringify(canonical)); record.reason = reason; return validator(record);
      }),
      withinToleranceAccepted: validator(withinTolerance),
      unexecutableRelativeHoldAccepted: validator(makeUnexecutableRelativeHold()),
      projectedHoldAccepted: validator(makeProjectedHold()),
      rejects: {
        hugeResidualFalseZeroNormalization: rejectedAccepted((record) => {
          record.recurrence.residual = 1e9; record.recurrence.normalizedResidual = 0;
        }),
        mismatchedNormalizedResidual: rejectedAccepted((record) => { record.recurrence.normalizedResidual = 0.5; }),
        hiddenEndpointDrift: rejectedAccepted((record) => { record.finalState[0] += 0.25; }),
        coherentEndpointDisplacementForgery: rejectedAccepted((record) => {
          record.finalState[6] += 0.25; record.recurrence.restoration.displacement += 0.25;
        }),
        coherentTwoPiRestorationForgery: rejectedAccepted((record) => {
          record.finalState[6] = 2 * Math.PI; record.recurrence.restoration.displacement = 2 * Math.PI;
        }),
        hiddenPeriodDrift: rejectedCanonical((record) => { record.coordinates[6].period = 2 * Math.PI; }),
        restorationPeriodDrift: rejectedAccepted((record) => { record.coordinates[6].period = 2 * Math.PI; }),
        restorationDisplacementDrift: rejectedAccepted((record) => { record.recurrence.restoration.displacement += 0.25; }),
        restorationActionDrift: rejectedAccepted((record) => { record.recurrence.restoration.action = 'phase-translation'; }),
        restorationCoordinateMismatch: rejectedAccepted((record) => { record.recurrence.restoration.coordinate = record.coordinates[5].name; }),
        equivarianceWitnessIdDrift: rejectedAccepted((record) => { record.recurrence.restoration.equivarianceWitness.id = 'plausible-two-turn-cover'; }),
        equivarianceFieldGainDrift: rejectedAccepted((record) => { record.recurrence.restoration.equivarianceWitness.fieldGain = 1; }),
        equivarianceFieldCellCountDrift: rejectedAccepted((record) => { record.recurrence.restoration.equivarianceWitness.fieldCellCount = 2; }),
        equivarianceIncidentCountDrift: rejectedAccepted((record) => { record.recurrence.restoration.equivarianceWitness.incidentChannelCount = 1; }),
        equivarianceMutatingCountDrift: rejectedAccepted((record) => { record.recurrence.restoration.equivarianceWitness.mutatingStepCount = 1; }),
        sectionWitnessIdDrift: rejectedAccepted((record) => { record.recurrence.restoration.sectionWitness.id = 'plausible-returned-restored'; }),
        coherentSectionUniversalWindingDrift: rejectedAccepted((record) => {
          record.recurrence.restoration.sectionWitness.winding = 1; record.universal.winding = 1;
        }),
        sectionProjectionDrift: rejectedAccepted((record) => { record.recurrence.restoration.sectionWitness.projection = 'projected'; }),
        sectionSheetDrift: rejectedAccepted((record) => { record.recurrence.restoration.sectionWitness.sheet = 'returned'; }),
        restorationDerivativeByteDrift: rejectedAccepted((record) => { record.recurrence.restoration.derivative[1] = 1; }),
        restorationDerivativeDigestDrift: rejectedAccepted((record) => { record.recurrence.restoration.derivativeDigest = 'blake3:' + '0'.repeat(64); }),
        missingRestorationDerivativeBytes: rejectedAccepted((record) => { record.recurrence.restoration.derivative = null; }),
        undeclaredRelativeForgedResidual: rejectedUnexecutable((record) => { record.recurrence.residual = 1e9; }),
        undeclaredRelativeForgedNormalized: rejectedUnexecutable((record) => { record.recurrence.normalizedResidual = 0.5; }),
        undeclaredRelativeVerified: rejectedUnexecutable((record) => {
          record.recurrence.discreteStateVerified = true; record.recurrence.verified = true; record.discreteState.verified = true;
        }),
        undeclaredRelativeAuthorizing: rejectedUnexecutable((record) => { record.recurrence.authorizesMonodromy = true; }),
        undeclaredRelativeInventedRestoration: rejectedUnexecutable((record) => { record.recurrence.restoration = {}; }),
        undeclaredRelativeAppliedDerivative: rejectedUnexecutable((record) => { record.recurrence.restorationDerivativeApplied = true; }),
        projectedLocallyVerifiedReportedFalse: rejectedProjected((record) => {
          record.recurrence.normalizedResidual = 0; record.recurrence.discreteStateVerified = true; record.discreteState.verified = true;
        }),
        projectedVerified: rejectedProjected((record) => {
          record.recurrence.normalizedResidual = 0; record.recurrence.discreteStateVerified = true;
          record.recurrence.verified = true; record.discreteState.verified = true;
        }),
        projectedAuthorizing: rejectedProjected((record) => { record.recurrence.authorizesMonodromy = true; }),
        nonCurrentReason: validator(make('spectrum-residual')) === false,
        spectralReasonAtTangentStage: (() => { const record = JSON.parse(JSON.stringify(canonical)); record.reason = 'spectral-backend-unavailable'; return validator(record) === false; })(),
        nonCurrentReasonAtTangentStage: (() => { const record = JSON.parse(JSON.stringify(canonical)); record.reason = 'spectrum-residual'; return validator(record) === false; })(),
        unknownReasonAtTangentStage: (() => { const record = JSON.parse(JSON.stringify(canonical)); record.reason = 'unknown-hold'; return validator(record) === false; })(),
        stableLabel: rejectedAccepted((record) => { record.classification = 'stable'; }),
        unstableLabel: rejectedAccepted((record) => { record.classification = 'unstable'; }),
        alteredRealLyingModulus: rejectedAccepted((record) => { record.multipliers[0].real = 0.75; }),
        alteredImaginaryLyingModulus: rejectedAccepted((record) => { record.multipliers[0].imag = 0.25; }),
        nonUnitCoherentStableSpectrum: rejectedAccepted((record) => {
          record.classification = 'stable'; record.spectrumDiagnostics.spectralRadius = 0.9;
          record.multipliers.forEach((multiplier) => { multiplier.real = 0.9; multiplier.imag = 0; multiplier.modulus = 0.9; });
        }),
        alteredSpectralRadius: rejectedAccepted((record) => { record.spectrumDiagnostics.spectralRadius = 0.9; }),
        gaugeMultiplier: rejectedAccepted((record) => { record.multipliers[0].mode = 'gauge'; }),
        truncatedMultipliers: rejectedAccepted((record) => { record.multipliers.pop(); }),
        nullDigest: rejected((record) => { record.monodromyDigest = null; }),
        emptyDigest: rejected((record) => { record.monodromyDigest = ''; }),
        nullMatrix: rejected((record) => { record.monodromy = null; }),
        emptyMatrix: rejected((record) => { record.monodromy = []; }),
        shortMatrix: rejected((record) => { record.monodromy = [1]; }),
        nonfiniteMatrix: rejected((record) => { record.monodromy[0] = Infinity; }),
        unappliedRelativeDerivative: rejected((record) => { record.recurrence.restorationDerivativeApplied = false; }),
        missingRelativeRestoration: rejected((record) => { record.recurrence.restoration = null; }),
        unverifiedEquivariance: rejected((record) => { record.recurrence.restoration.equivarianceWitness.verified = false; }),
        unverifiedSection: rejected((record) => { record.recurrence.restoration.sectionWitness.verified = false; }),
        wrongDerivativeShape: rejected((record) => { record.recurrence.restoration.derivativeRows -= 1; }),
        unverifiedDiscreteState: rejected((record) => { record.recurrence.discreteStateVerified = false; }),
        multiplierLeakage: rejected((record) => { record.multipliers = accepted.multipliers; }),
        backendLeakage: rejected((record) => { record.backend = 'lapack-dgees'; }),
        diagnosticLeakage: rejected((record) => { record.spectrumDiagnostics = accepted.spectrumDiagnostics; })
      }
    };
  })()`,
  returnByValue: true,
});
if (!analysisBoundaryEvaluation.result?.value) {
  throw new Error(
    `embedded validator proof did not return a value: ${JSON.stringify(analysisBoundaryEvaluation.exceptionDetails || analysisBoundaryEvaluation.result)}`,
  );
}
const analysisBoundaryProof = analysisBoundaryEvaluation.result.value;
assert.deepEqual(
  analysisBoundaryProof.currentReasons,
  [true, true, true],
  "embedded validator rejected an exact current spectral-hold reason",
);
assert.deepEqual(
  analysisBoundaryProof.currentTangentReasons,
  [true, true, true],
  "embedded validator rejected an exact current tangent-hold reason",
);
assert.equal(
  analysisBoundaryProof.withinToleranceAccepted,
  true,
  "embedded validator rejected the bounded fixture within its declared tolerance",
);
assert.equal(
  analysisBoundaryProof.unexecutableRelativeHoldAccepted,
  true,
  "embedded validator rejected the exact nonauthoritative undeclared-relative hold",
);
assert.equal(
  analysisBoundaryProof.projectedHoldAccepted,
  true,
  "embedded validator rejected the exact nonauthoritative projected hold",
);
for (const [counterexample, rejected] of Object.entries(analysisBoundaryProof.rejects)) {
  assert.equal(rejected, true, `embedded validator accepted ${counterexample}`);
}

// Explicit accessibility-media and normal-text contrast proof. This runs in a
// separate fresh navigation so the assertions cannot inherit the default case.
await browser.send("Emulation.setDeviceMetricsOverride", {
  width: 768,
  height: 900,
  deviceScaleFactor: 1,
  mobile: false,
});
await browser.send("Emulation.setEmulatedMedia", { media: "screen", features: [] });
{
  const loaded = browser.once("Page.loadEventFired");
  await browser.send("Page.navigate", { url: pageUrl });
  await loaded;
}
const accessibilityExpression = `(() => {
    function parseColor(input) {
      if (input.startsWith('#')) {
        const value = input.slice(1);
        return { r: parseInt(value.slice(0, 2), 16), g: parseInt(value.slice(2, 4), 16), b: parseInt(value.slice(4, 6), 16), a: 1 };
      }
      const parts = input.match(/[0-9.]+/g).map(Number);
      return { r: parts[0], g: parts[1], b: parts[2], a: parts.length > 3 ? parts[3] : 1 };
    }
    function composite(foreground, background) {
      return {
        r: foreground.r * foreground.a + background.r * (1 - foreground.a),
        g: foreground.g * foreground.a + background.g * (1 - foreground.a),
        b: foreground.b * foreground.a + background.b * (1 - foreground.a),
        a: 1
      };
    }
    function luminance(color) {
      const channels = [color.r, color.g, color.b].map((channel) => {
        const value = channel / 255;
        return value <= 0.04045 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4);
      });
      return channels[0] * 0.2126 + channels[1] * 0.7152 + channels[2] * 0.0722;
    }
    function contrast(selector, backgroundVariable) {
      const element = document.querySelector(selector);
      const foreground = parseColor(getComputedStyle(element).color);
      const background = parseColor(getComputedStyle(document.documentElement).getPropertyValue(backgroundVariable).trim());
      const rendered = composite(foreground, background);
      const light = luminance(rendered), dark = luminance(background);
      return {
        selector,
        fontSizePx: parseFloat(getComputedStyle(element).fontSize),
        ratio: (Math.max(light, dark) + 0.05) / (Math.min(light, dark) + 0.05)
      };
    }
    const samples = [
      contrast('.rail-intro', '--night'),
      contrast('.source-option[aria-checked="true"] span', '--raised'),
      contrast('.runtime-list dt', '--night'),
      contrast('.orbit-tab[data-stage="recurrence"] .stage-state', '--deep'),
      contrast('.fact dt', '--deep'),
      contrast('.provenance dt', '--night'),
      contrast('.footer', '--abyss')
    ];
    return {
      reducedMotion: matchMedia('(prefers-reduced-motion: reduce)').matches,
      increasedContrast: matchMedia('(prefers-contrast: more)').matches,
      sourceTransitionDuration: getComputedStyle(document.querySelector('.source-option')).transitionDuration,
      railBorderColor: getComputedStyle(document.querySelector('.rail')).borderColor,
      contrastSamples: samples,
      fatalStateHidden: document.getElementById('fatal-state').hidden
    };
  })()`;
const normalAccessibilityEvaluation = await browser.send("Runtime.evaluate", {
  expression: accessibilityExpression,
  returnByValue: true,
});
const normalAccessibilityProof = normalAccessibilityEvaluation.result.value;
await browser.send("Emulation.setEmulatedMedia", {
  media: "screen",
  features: [
    { name: "prefers-reduced-motion", value: "reduce" },
    { name: "prefers-contrast", value: "more" },
  ],
});
const accessibilityEvaluation = await browser.send("Runtime.evaluate", {
  expression: accessibilityExpression,
  returnByValue: true,
});
const accessibilityProof = accessibilityEvaluation.result.value;
accessibilityProof.normalContrastSamples = normalAccessibilityProof.contrastSamples;
assert.equal(accessibilityProof.reducedMotion, true, "reduced-motion media emulation did not apply");
assert.equal(accessibilityProof.increasedContrast, true, "increased-contrast media emulation did not apply");
const transitionMilliseconds = accessibilityProof.sourceTransitionDuration.split(",").map((duration) => {
  const value = Number.parseFloat(duration);
  return duration.trim().endsWith("ms") ? value : value * 1000;
});
assert.ok(
  transitionMilliseconds.every((duration) => duration <= 0.001),
  `reduced-motion transition remained active: ${accessibilityProof.sourceTransitionDuration}`,
);
assert.equal(accessibilityProof.fatalStateHidden, true, "accessibility-media navigation failed receipt validation");
for (const sample of accessibilityProof.normalContrastSamples) {
  assert.ok(sample.ratio >= 4.5, `${sample.selector}: contrast ${sample.ratio.toFixed(2)} < 4.5`);
  assert.ok(sample.fontSizePx >= 11, `${sample.selector}: essential text ${sample.fontSizePx}px < 11px`);
}

const accessibilityLayout = await browser.send("Page.getLayoutMetrics");
const accessibilityScreenshot = await browser.send("Page.captureScreenshot", {
  format: "png",
  fromSurface: true,
  captureBeyondViewport: true,
  clip: { x: 0, y: 0, width: 768, height: Math.ceil(accessibilityLayout.cssContentSize.height), scale: 1 },
});
accessibilityProof.screenshotPath = path.join(outputDirectory, "768-accessibility-media.png");
fs.writeFileSync(accessibilityProof.screenshotPath, Buffer.from(accessibilityScreenshot.data, "base64"));

assert.deepEqual(exceptions, [], `browser emitted errors: ${exceptions.join("; ")}`);
const proofReport = {
  status: "WEB_BROWSER_PROOF_PASS",
  outputDirectory,
  cases: report,
  analysisBoundaryProof,
  accessibilityProof,
};
fs.writeFileSync(
  path.join(outputDirectory, "browser-proof-report.json"),
  `${JSON.stringify(proofReport, null, 2)}\n`,
);
console.log(JSON.stringify(proofReport, null, 2));
browser.socket.close();

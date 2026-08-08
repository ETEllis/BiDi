#!/usr/bin/env node
/** Generate the deterministic, receipt-backed U2 web snapshot. */

import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { fileURLToPath } from "node:url";
import fs from "node:fs";
import path from "node:path";

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, "../../..");
const OUTPUT = path.join(HERE, "u2-snapshots.json");
const RUNTIME = path.join(ROOT, "build/u2/cdc_native_runtime");

export const CAPTURES = [
  ["framework_loop.cdc", "ui/web/console/receipts/canonical-held.txt"],
  [
    "tests/fixtures/u2/u720_true_relative_marginal.cdc",
    "ui/web/console/receipts/relative-calibration.txt",
  ],
];

function sha256(filePath) {
  return createHash("sha256").update(fs.readFileSync(filePath)).digest("hex");
}

function receiptRecordsFromText(text, label) {
  const records = text
    .split(/\r?\n/u)
    .filter((line) => line.startsWith("u2-json="))
    .map((line) => JSON.parse(line.slice("u2-json=".length)));
  if (records.length === 0) throw new Error(`no u2-json receipt in ${label}`);
  return records;
}

function receiptRecords(filePath) {
  return receiptRecordsFromText(fs.readFileSync(filePath, "utf8"), filePath);
}

export function runtimeReceiptText(sourcePath) {
  if (!fs.existsSync(RUNTIME)) throw new Error(`build the U2 runtime first: ${RUNTIME}`);
  let text = execFileSync(RUNTIME, ["stability", sourcePath], {
    cwd: ROOT,
    encoding: "utf8",
    stdio: ["ignore", "pipe", "pipe"],
  });
  if (!text.endsWith("\n")) text += "\n";
  receiptRecordsFromText(text, sourcePath);
  return text;
}

function captureReceipts() {
  for (const [sourcePath, outputPath] of CAPTURES) {
    const target = path.join(ROOT, outputPath);
    fs.mkdirSync(path.dirname(target), { recursive: true });
    fs.writeFileSync(target, runtimeReceiptText(sourcePath), "utf8");
  }
}

function abiMetadata() {
  const sourcePath = path.join(ROOT, "runtime/cdc_abi.h");
  const text = fs.readFileSync(sourcePath, "utf8");
  const major = text.match(/#define CDC_ABI_VERSION_MAJOR\s+(\d+)/u);
  const minor = text.match(/#define CDC_ABI_VERSION_MINOR\s+(\d+)/u);
  if (!major || !minor) throw new Error("could not read ABI version macros");
  return {
    major: Number(major[1]),
    minor: Number(minor[1]),
    grammar: 1,
    sourcePath: "runtime/cdc_abi.h",
    sourceSha256: sha256(sourcePath),
  };
}

function receiptProjection(receipt) {
  const recurrence = receipt.recurrence;
  const restoration = recurrence?.restoration;
  return {
    schema: receipt.schema,
    orbit: receipt.orbit,
    status: receipt.status,
    reason: receipt.reason,
    analysis: receipt.analysis,
    runtimeIdentity: receipt.runtimeIdentity,
    sourceDigest: receipt.sourceDigest,
    sourceDigestSurface: receipt.sourceDigestSurface,
    universal: receipt.universal,
    manifestDigest: receipt.manifestDigest,
    methods: receipt.methods,
    methodDetail: receipt.methodDetail,
    tolerances: receipt.tolerances,
    finiteDifference: receipt.finiteDifference,
    eventBudget: receipt.eventBudget,
    neutralModeRemoval: receipt.neutralModeRemoval,
    determinism: receipt.determinism,
    recurrence: !recurrence
      ? null
      : {
          kind: recurrence.kind,
          scope: recurrence.scope,
          residual: recurrence.residual,
          absoluteTolerance: recurrence.absoluteTolerance,
          relativeTolerance: recurrence.relativeTolerance,
          normalizedResidual: recurrence.normalizedResidual,
          verified: recurrence.verified,
          authorizesMonodromy: recurrence.authorizesMonodromy,
          discreteStateVerified: recurrence.discreteStateVerified,
          restorationDerivativeApplied: recurrence.restorationDerivativeApplied,
          restoration: !restoration
            ? null
            : {
                action: restoration.action,
                coordinate: restoration.coordinate,
                coordinateIndex: restoration.coordinateIndex,
                displacement: restoration.displacement,
                equivarianceWitness: restoration.equivarianceWitness,
                sectionWitness: restoration.sectionWitness,
                derivativeRows: restoration.derivativeRows,
                derivativeColumns: restoration.derivativeColumns,
                derivativeDigest: restoration.derivativeDigest,
                derivative: restoration.derivative,
              },
        },
    discreteState: receipt.discreteState,
    dimension: receipt.dimension,
    coordinates: receipt.coordinates,
    initialState: receipt.initialState,
    finalState: receipt.finalState,
    events: receipt.events,
    pathTangentDigest: receipt.pathTangentDigest,
    monodromyDigest: receipt.monodromyDigest,
    monodromy: receipt.monodromy,
    multipliers: receipt.multipliers,
    spectrumDiagnostics: receipt.spectrumDiagnostics,
    backend: receipt.backend,
    classification: receipt.classification,
  };
}

function hasAuthoritativeMethodContract(record) {
  const method = record.methodDetail;
  return (
    method?.flow?.localMap === "explicit-euler-synchronous" &&
    method.flow.jacobian === "analytic-exact" &&
    method.flow.finiteDifferenceOracle === null &&
    method?.commit?.kind === "scheduled-fixed-mode-reset" &&
    method.commit.continuousDerivative === "identity-within-fixed-mode" &&
    method.commit.saltation === "not-applicable" &&
    method?.nest?.localMap === "fixed-trit-overwrite" &&
    method.nest.jacobian === "analytic-exact" &&
    method.nest.childPrior === "overwrite-parent-belief" &&
    method?.guard?.binding === null &&
    method.guard.eventLocalization === null &&
    method.guard.saltation === null &&
    method?.spectrum?.solver === "validated-real-schur" &&
    method.spectrum.ordering === "sorted-multipliers" &&
    record.finiteDifference?.status === "not-run" &&
    record.finiteDifference.step === null &&
    record.finiteDifference.residual === null &&
    record.eventBudget?.status === "not-applicable" &&
    record.eventBudget.limit === null &&
    record.eventBudget.used === null &&
    record.neutralModeRemoval?.status === "not-requested" &&
    record.neutralModeRemoval.generatorDigest === null &&
    record.neutralModeRemoval.removedModes === null
  );
}

function hasDiscreteStateShape(record) {
  const state = record.discreteState;
  return (
    typeof state?.initialDigest === "string" &&
    typeof state.finalDigest === "string" &&
    Array.isArray(state.initial) &&
    Array.isArray(state.final) &&
    state.initial.length > 0 &&
    state.initial.length === state.final.length &&
    state.initial.every(
      (initial, index) =>
        typeof initial.cell === "string" &&
        typeof initial.mode === "string" &&
        initial.cell === state.final[index]?.cell &&
        typeof state.final[index]?.mode === "string",
    )
  );
}

export function validatesAnalysisBoundary(record) {
  if (!record || !record.recurrence) return false;
  const recurrence = record.recurrence;
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
  const digestPattern = /^blake3:[0-9a-f]{64}$/u;
  const identityDerivativeDigest =
    "blake3:737995db71af62fc1f0e0beb6b2153bd7ddd20eacb8b745eaf4a413060123223";
  function nearlyEqual(left, right) {
    if (!Number.isFinite(left) || !Number.isFinite(right)) return false;
    // Both values originated as binary64. This envelope covers transport and
    // cross-language roundoff only; it is not a recurrence-model tolerance.
    const scale = Math.max(1, Math.abs(left), Math.abs(right));
    return Math.abs(left - right) <= 64 * Number.EPSILON * scale;
  }
  function hasCurrentRelativeRestoration(candidate, restoration) {
    const coordinateIndex = restoration?.coordinateIndex;
    const equivariance = restoration?.equivarianceWitness;
    const section = restoration?.sectionWitness;
    const dimension = candidate.dimension;
    return (
      restoration?.action === "cover-phase-translation" &&
      Number.isInteger(coordinateIndex) &&
      coordinateIndex >= 0 &&
      coordinateIndex < dimension &&
      restoration.coordinate === candidate.coordinates[coordinateIndex]?.name &&
      candidate.coordinates[coordinateIndex]?.period === 0 &&
      nearlyEqual(restoration.displacement, 4 * Math.PI) &&
      equivariance?.id === "isolated-affine-two-turn-cover" &&
      equivariance.verified === true &&
      equivariance.fieldGain === 0 &&
      equivariance.fieldCellCount === 1 &&
      equivariance.incidentChannelCount === 0 &&
      equivariance.mutatingStepCount === 0 &&
      section?.id === "u1-two-turn-returned-restored" &&
      section.verified === true &&
      section.winding === 2 &&
      candidate.universal?.winding === section.winding &&
      section.projection === "returned" &&
      section.sheet === "restored" &&
      restoration.derivativeRows === dimension &&
      restoration.derivativeColumns === dimension &&
      restoration.derivativeDigest === identityDerivativeDigest &&
      Array.isArray(restoration.derivative) &&
      restoration.derivative.length === dimension * dimension &&
      restoration.derivative.every((value, index) => {
        const row = Math.floor(index / dimension);
        const column = index % dimension;
        return value === (row === column ? 1 : 0);
      })
    );
  }
  function recurrenceMetrics(candidate) {
    const candidateRecurrence = candidate.recurrence;
    const dimension = candidate.dimension;
    if (
      !Number.isInteger(dimension) ||
      dimension <= 0 ||
      !Array.isArray(candidate.coordinates) ||
      candidate.coordinates.length !== dimension ||
      !Array.isArray(candidate.initialState) ||
      candidate.initialState.length !== dimension ||
      !Array.isArray(candidate.finalState) ||
      candidate.finalState.length !== dimension ||
      !candidate.initialState.every((value) => Number.isFinite(value)) ||
      !candidate.finalState.every((value) => Number.isFinite(value)) ||
      !candidate.coordinates.every(
        (coordinate) =>
          typeof coordinate?.name === "string" &&
          coordinate.name.length > 0 &&
          Number.isFinite(coordinate.period) &&
          coordinate.period >= 0,
      ) ||
      !Number.isFinite(candidateRecurrence.absoluteTolerance) ||
      candidateRecurrence.absoluteTolerance <= 0 ||
      !Number.isFinite(candidateRecurrence.relativeTolerance) ||
      candidateRecurrence.relativeTolerance < 0 ||
      !nearlyEqual(candidateRecurrence.absoluteTolerance, candidate.tolerances?.recurrenceAbsolute) ||
      !nearlyEqual(candidateRecurrence.relativeTolerance, candidate.tolerances?.recurrenceRelative) ||
      typeof candidateRecurrence.discreteStateVerified !== "boolean" ||
      candidate.discreteState?.verified !== candidateRecurrence.discreteStateVerified
    ) {
      return null;
    }

    const restored = candidate.finalState.slice();
    if (candidateRecurrence.kind === "relative") {
      const restoration = candidateRecurrence.restoration;
      if (
        candidateRecurrence.scope !== "relative" ||
        !hasCurrentRelativeRestoration(candidate, restoration)
      ) {
        return null;
      }
      restored[restoration.coordinateIndex] -= restoration.displacement;
    } else if (
      candidateRecurrence.kind !== "full" ||
      candidateRecurrence.scope !== "full" ||
      candidateRecurrence.restoration !== null
    ) {
      return null;
    }

    let residual = 0;
    let normalizedResidual = 0;
    for (let index = 0; index < dimension; index += 1) {
      const period = candidate.coordinates[index].period;
      let difference = restored[index] - candidate.initialState[index];
      if (period > 0) {
        difference %= period;
        if (difference > 0.5 * period) difference -= period;
        else if (difference < -0.5 * period) difference += period;
      }
      const coordinateResidual = Math.abs(difference);
      const allowed =
        candidateRecurrence.absoluteTolerance +
        candidateRecurrence.relativeTolerance *
          Math.max(Math.abs(restored[index]), Math.abs(candidate.initialState[index]));
      const normalized = coordinateResidual / allowed;
      residual = Math.max(residual, coordinateResidual);
      normalizedResidual = Math.max(normalizedResidual, normalized);
    }
    return {
      residual,
      normalizedResidual,
      verified: normalizedResidual <= 1 && candidateRecurrence.discreteStateVerified,
    };
  }
  function hasRetainedMonodromy(candidate) {
    return (
      digestPattern.test(candidate.monodromyDigest) &&
      Number.isInteger(candidate.dimension) &&
      candidate.dimension > 0 &&
      Array.isArray(candidate.monodromy) &&
      candidate.monodromy.length === candidate.dimension * candidate.dimension &&
      candidate.monodromy.every((value) => Number.isFinite(value))
    );
  }
  function hasAuthorizedRestoration(candidate) {
    const candidateRecurrence = candidate.recurrence;
    if (candidateRecurrence.kind === "full") {
      return (
        candidateRecurrence.scope === "full" &&
        candidateRecurrence.restorationDerivativeApplied === false &&
        candidateRecurrence.restoration === null
      );
    }
    if (candidateRecurrence.kind !== "relative") return false;
    const restoration = candidateRecurrence.restoration;
    return (
      candidateRecurrence.scope === "relative" &&
      candidateRecurrence.restorationDerivativeApplied === true &&
      hasCurrentRelativeRestoration(candidate, restoration)
    );
  }
  function hasBoundedPositiveSpectrum(candidate) {
    const tolerance = candidate.tolerances?.schur;
    return (
      Number.isFinite(tolerance) &&
      tolerance > 0 &&
      candidate.dimension === 13 &&
      candidate.classification === "marginal" &&
      Array.isArray(candidate.multipliers) &&
      candidate.multipliers.length === 13 &&
      candidate.multipliers.every(
        (multiplier) =>
          multiplier.mode === "physical" &&
          Number.isFinite(multiplier.real) &&
          Number.isFinite(multiplier.imag) &&
          Number.isFinite(multiplier.modulus) &&
          Math.abs(multiplier.real - 1) <= tolerance &&
          Math.abs(multiplier.imag) <= tolerance &&
          Math.abs(multiplier.modulus - 1) <= tolerance,
      ) &&
      Number.isFinite(candidate.spectrumDiagnostics?.spectralRadius) &&
      Math.abs(candidate.spectrumDiagnostics.spectralRadius - 1) <= tolerance
    );
  }
  const undeclaredHoldEnvelope =
    record.status === "held" &&
    record.reason === "undeclared-quotient" &&
    record.analysis === "tangent" &&
    recurrence.authorizesMonodromy === false &&
    recurrence.restorationDerivativeApplied === false &&
    recurrence.restoration === null &&
    typeof recurrence.discreteStateVerified === "boolean" &&
    record.discreteState?.verified === recurrence.discreteStateVerified &&
    Number.isFinite(recurrence.absoluteTolerance) &&
    recurrence.absoluteTolerance > 0 &&
    Number.isFinite(recurrence.relativeTolerance) &&
    recurrence.relativeTolerance >= 0 &&
    nearlyEqual(recurrence.absoluteTolerance, record.tolerances?.recurrenceAbsolute) &&
    nearlyEqual(recurrence.relativeTolerance, record.tolerances?.recurrenceRelative);
  const projectedHold =
    recurrence.scope === "projected" &&
    undeclaredHoldEnvelope &&
    ["full", "relative"].includes(recurrence.kind) &&
    Number.isFinite(recurrence.residual) &&
    recurrence.residual >= 0 &&
    Number.isFinite(recurrence.normalizedResidual) &&
    recurrence.normalizedResidual >= 0 &&
    recurrence.verified === false &&
    !(recurrence.normalizedResidual <= 1 && recurrence.discreteStateVerified);
  const unexecutableRelativeHold =
    recurrence.kind === "relative" &&
    recurrence.scope === "relative" &&
    undeclaredHoldEnvelope &&
    recurrence.residual === 0 &&
    recurrence.normalizedResidual === 0 &&
    recurrence.verified === false &&
    recurrence.discreteStateVerified === false;
  const nonAuthoritativeUndeclaredHold = projectedHold || unexecutableRelativeHold;
  if (recurrence.scope === "projected") {
    // A projected receipt does not serialize its include mask, so its scalar
    // residual cannot be independently reconstructed. It remains admissible
    // only as the runtime's explicit, nonauthorizing undeclared-quotient hold.
    if (!projectedHold) return false;
  } else if (unexecutableRelativeHold) {
    // The runtime can reject a requested relative quotient before it has an
    // executable restoration. Its zero metrics are explicit non-results, not
    // a recurrence claim, and are admitted only in this exact held shape.
  } else {
    const derivedRecurrence = recurrenceMetrics(record);
    if (
      !derivedRecurrence ||
      !nearlyEqual(recurrence.residual, derivedRecurrence.residual) ||
      !nearlyEqual(recurrence.normalizedResidual, derivedRecurrence.normalizedResidual) ||
      recurrence.verified !== derivedRecurrence.verified ||
      recurrence.authorizesMonodromy !== derivedRecurrence.verified
    ) {
      return false;
    }
  }
  const noSpectrum = record.multipliers === null && record.spectrumDiagnostics === null;
  if (record.status === "accepted") {
    return (
      record.reason === "none" &&
      record.analysis === "monodromy" &&
      recurrence.verified === true &&
      recurrence.authorizesMonodromy === true &&
      recurrence.discreteStateVerified === true &&
      hasAuthorizedRestoration(record) &&
      hasRetainedMonodromy(record) &&
      hasBoundedPositiveSpectrum(record) &&
      typeof record.backend === "string" &&
      record.backend.length > 0 &&
      record.spectrumDiagnostics !== null
    );
  }
  if (record.status !== "held" || record.classification !== "held") return false;
  const spectralHold = currentSpectralHoldReasons.includes(record.reason);
  if (spectralHold) {
    return (
      record.analysis === "monodromy" &&
      recurrence.verified === true &&
      recurrence.authorizesMonodromy === true &&
      recurrence.discreteStateVerified === true &&
      hasAuthorizedRestoration(record) &&
      hasRetainedMonodromy(record) &&
      record.backend === null &&
      noSpectrum
    );
  }
  return (
    currentTangentHoldReasons.includes(record.reason) &&
    record.analysis === "tangent" &&
    (nonAuthoritativeUndeclaredHold || recurrence.verified === false) &&
    recurrence.authorizesMonodromy === false &&
    record.monodromyDigest === null &&
    record.monodromy === null &&
    record.backend === null &&
    noSpectrum
  );
}

function entry({
  id,
  label,
  role,
  sourcePath,
  receiptPath,
  command,
  maturity,
  verdict,
  ceiling,
}) {
  const source = path.join(ROOT, sourcePath);
  const receiptFile = path.join(ROOT, receiptPath);
  const receipt = receiptRecords(receiptFile)[0];
  return {
    id,
    label,
    role,
    sourcePath,
    sourceFileSha256: sha256(source),
    receiptPath,
    receiptFileSha256: sha256(receiptFile),
    receiptRecordIndex: 0,
    receiptHref: path.relative(HERE, receiptFile),
    command,
    claim: { maturity, verdict, scope: sourcePath, ceiling },
    receipt: receiptProjection(receipt),
  };
}

export function buildSnapshot() {
  const snapshot = {
    schema: "cdc.ui.verified-replay.v1",
    snapshotKind: "verified replay snapshot",
    authoritativeDate: "2026-08-08",
    generatorPath: "ui/web/console/generate-verified-snapshot.mjs",
    toolchain: abiMetadata(),
    entries: [
      entry({
        id: "canonical-held",
        label: "Canonical operator · full-state hold",
        role: "canonical-source",
        sourcePath: "framework_loop.cdc",
        receiptPath: "ui/web/console/receipts/canonical-held.txt",
        command: "build/u2/cdc_native_runtime stability framework_loop.cdc",
        maturity: "adversarially-verified",
        verdict: "held",
        ceiling:
          "U1 accepted. U2 tangent executed. Full-state recurrence held; no spectrum was emitted.",
      }),
      entry({
        id: "relative-fixture",
        label: "Calibration fixture · positive relative recurrence",
        role: "positive-relative-calibration-fixture",
        sourcePath: "tests/fixtures/u2/u720_true_relative_marginal.cdc",
        receiptPath: "ui/web/console/receipts/relative-calibration.txt",
        command:
          "build/u2/cdc_native_runtime stability tests/fixtures/u2/u720_true_relative_marginal.cdc",
        maturity: "adversarially-verified",
        verdict: "accepted",
        ceiling:
          "Relative recurrence accepted. Identity monodromy produced thirteen physical unit multipliers; classification marginal.",
      }),
    ],
  };

  const [canonical, relative] = snapshot.entries.map((item) => item.receipt);
  for (const [label, record] of [
    ["canonical", canonical],
    ["relative", relative],
  ]) {
    if (!hasAuthoritativeMethodContract(record)) {
      throw new Error(`${label} snapshot omitted or drifted from the authoritative method contract`);
    }
    if (!hasDiscreteStateShape(record)) {
      throw new Error(`${label} snapshot omitted or malformed its discrete-state receipt`);
    }
  }
  const canonicalChanges = canonical.discreteState.initial.filter(
    (initial, index) => initial.mode !== canonical.discreteState.final[index].mode,
  );
  if (
    !validatesAnalysisBoundary(canonical) ||
    canonical.status !== "held" ||
    canonical.reason !== "recurrence-mode-mismatch" ||
    canonical.recurrence.authorizesMonodromy !== false ||
    canonical.recurrence.discreteStateVerified !== false ||
    canonical.discreteState.verified !== false ||
    canonical.discreteState.initialDigest === canonical.discreteState.finalDigest ||
    canonicalChanges.length !== 3 ||
    canonicalChanges.map((item) => item.cell).join(",") !== "agent.a,agent.b,agent.c" ||
    canonical.discreteState.final.slice(0, 3).map((item) => item.mode).join(",") !==
      "latched:+,latched:+,latched:0" ||
    canonical.monodromyDigest !== null ||
    canonical.multipliers !== null ||
    canonical.spectrumDiagnostics !== null
  ) {
    throw new Error(
      "canonical snapshot must expose the discrete-state change, hold before monodromy, and emit no spectrum",
    );
  }
  const schur = relative.spectrumDiagnostics?.schur;
  const schurResiduals = !schur
    ? []
    : [schur.reconstructionResidual, schur.orthogonalityResidual, schur.triangularResidual];
  if (
    !validatesAnalysisBoundary(relative) ||
    relative.status !== "accepted" ||
    relative.recurrence.verified !== true ||
    relative.recurrence.authorizesMonodromy !== true ||
    relative.recurrence.discreteStateVerified !== true ||
    relative.discreteState.verified !== true ||
    relative.discreteState.initialDigest !== relative.discreteState.finalDigest ||
    !relative.discreteState.initial.every(
      (initial, index) => initial.mode === relative.discreteState.final[index].mode,
    ) ||
    relative.dimension !== 13 ||
    !Array.isArray(relative.multipliers) ||
    relative.multipliers.length !== 13 ||
    !relative.multipliers.every((item) => item.mode === "physical" && item.modulus === 1) ||
    relative.spectrumDiagnostics?.spectralRadius !== 1 ||
    !schur ||
    !Number.isFinite(schur.validationTolerance) ||
    schur.validationTolerance <= 0 ||
    schurResiduals.length !== 3 ||
    !schurResiduals.every(
      (residual) => Number.isFinite(residual) && residual >= 0 && residual <= schur.validationTolerance,
    )
  ) {
    throw new Error(
      "relative calibration snapshot is not the discrete-state-verified, numerically validated 13-mode receipt",
    );
  }
  if (snapshot.toolchain.major !== 1 || snapshot.toolchain.minor !== 5) {
    throw new Error("web contract requires the checked ABI 1.5 source");
  }
  return snapshot;
}

function syncEmbeddedSnapshot(snapshot) {
  const htmlPath = path.join(HERE, "index.html");
  const html = fs.readFileSync(htmlPath, "utf8");
  const pattern =
    /(<script type="application\/json" id="u2-snapshot-data">\n)[\s\S]*?(\n<\/script>)/u;
  if (!pattern.test(html)) throw new Error("console is missing the U2 snapshot data block");
  const rendered = JSON.stringify(snapshot, null, 2);
  fs.writeFileSync(
    htmlPath,
    html.replace(pattern, (_whole, opening, closing) => `${opening}${rendered}${closing}`),
    "utf8",
  );
}

function main() {
  captureReceipts();
  const snapshot = buildSnapshot();
  fs.writeFileSync(OUTPUT, `${JSON.stringify(snapshot, null, 2)}\n`, "utf8");
  syncEmbeddedSnapshot(snapshot);
  console.log(`wrote ${path.relative(ROOT, OUTPUT)}`);
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main();

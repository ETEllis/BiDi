import Foundation
import Darwin

enum U2ReceiptParser {
    static let schema = "cdc.u2.stability.v1"
    private static let runtimeIdentity = "cdc-native-u2-v1"
    private static let sourceBoundEarlyHoldReasons: Set<String> = [
        "allocation-failed",
        "dimension-mismatch",
        "invalid-argument",
        "nondifferentiable-quantization",
        "nonfinite-variational-state",
        "state-manifest-mismatch",
        "unsupported-delay-state",
        "unsupported-primal-semantic"
    ]
    private static let tangentHoldReasons: Set<String> = [
        "allocation-failed",
        "dimension-mismatch",
        "invalid-argument",
        "nonfinite-variational-state",
        "recurrence-mode-mismatch",
        "recurrence-residual",
        "undeclared-quotient"
    ]
    private static let spectralHoldReasons: Set<String> = [
        "allocation-failed",
        "invalid-argument",
        "spectral-backend-unavailable",
        "spectral-backend-failed",
        "variational-validation-failed"
    ]

    private struct SchemaEnvelope: Decodable {
        let schema: String
    }

    static func parse(_ output: String) throws -> [CanonicalU2Receipt] {
        let lines = output.split(omittingEmptySubsequences: false) { character in
            character.isNewline
        }
        var receipts: [CanonicalU2Receipt] = []
        let decoder = JSONDecoder()

        for (offset, lineSlice) in lines.enumerated() {
            let line = String(lineSlice)
            guard line.hasPrefix("u2-json=") else { continue }
            let raw = String(line.dropFirst("u2-json=".count))
            let data = Data(raw.utf8)
            let envelope: SchemaEnvelope
            do {
                envelope = try decoder.decode(SchemaEnvelope.self, from: data)
            } catch {
                throw U2ReceiptFailure(
                    kind: .malformedJSON(line: offset + 1),
                    detail: "Canonical U2 JSON on line \(offset + 1) could not be decoded: \(error.localizedDescription)"
                )
            }
            guard envelope.schema == schema else {
                throw U2ReceiptFailure(
                    kind: .unsupportedSchema(envelope.schema),
                    detail: "Unsupported U2 receipt schema \(envelope.schema); expected \(schema)."
                )
            }
            let record: U2StabilityRecord
            do {
                record = try decoder.decode(U2StabilityRecord.self, from: data)
            } catch {
                throw U2ReceiptFailure(
                    kind: .malformedJSON(line: offset + 1),
                    detail: "Canonical U2 JSON on line \(offset + 1) could not be decoded: \(error.localizedDescription)"
                )
            }
            try validateRequiredFieldPresence(raw)
            try validate(record)
            receipts.append(CanonicalU2Receipt(record: record, rawJSON: raw, lineNumber: offset + 1))
        }

        guard !receipts.isEmpty else {
            throw U2ReceiptFailure(
                kind: .noCanonicalRecord,
                detail: "The runtime emitted no canonical u2-json record. No U2 claim can be displayed."
            )
        }

        let duplicate = Dictionary(grouping: receipts, by: { $0.record.orbit })
            .first(where: { $0.value.count > 1 })?.key
        if let duplicate {
            throw invalid("duplicate orbit id \(duplicate)")
        }
        let digests = Set(receipts.map { $0.record.sourceDigest })
        guard digests.count == 1 else {
            throw invalid("records from one invocation disagree on the canonical source digest")
        }
        return receipts
    }

    private static func validate(_ record: U2StabilityRecord) throws {
        guard ["accepted", "held"].contains(record.status) else {
            throw invalid("unknown status \(record.status)")
        }
        guard ["accepted", "held"].contains(record.universal.status) else {
            throw invalid("unknown universal status \(record.universal.status)")
        }
        guard ["monodromy", "tangent", "held"].contains(record.analysis) else {
            throw invalid("unknown analysis carrier \(record.analysis)")
        }
        guard ["stable", "marginal", "unstable", "held"].contains(record.classification) else {
            throw invalid("unknown classification \(record.classification)")
        }
        guard record.runtimeIdentity == runtimeIdentity else {
            throw invalid("runtime identity is not the executable v1 U2 producer")
        }
        guard record.sourceDigestSurface == "grammar-1-canonical" else {
            throw invalid("source digest does not bind the grammar-1 canonical surface")
        }
        try validateDigest(record.sourceDigest, field: "sourceDigest")
        try validateDigest(record.universal.resultDigest, field: "universal.resultDigest")
        guard record.determinism == "source-order,row-major,canonical-float,sorted-multipliers" else {
            throw invalid("determinism declaration contradicts the executable v1 contract")
        }
        guard record.tolerances.recurrenceAbsolute.isFinite,
              record.tolerances.recurrenceAbsolute > 0,
              record.tolerances.recurrenceRelative.isFinite,
              record.tolerances.recurrenceRelative >= 0,
              record.tolerances.neutral.isFinite,
              record.tolerances.neutral > 0,
              record.tolerances.schur.isFinite,
              record.tolerances.schur > 0 else {
            throw invalid("declared U2 tolerances are not finite executable bounds")
        }
        try validateMethodNames(record)
        guard record.events.map(\.time).elementsEqual(record.events.map(\.time).sorted()) else {
            throw invalid("event times are not nondecreasing")
        }
        guard (record.status == "accepted") == (record.reason == "none") else {
            throw invalid("U2 status and reason contradict one another")
        }
        guard (record.universal.status == "accepted") == (record.universal.reason == "none") else {
            throw invalid("universal status and reason contradict one another")
        }

        let spectralHold = record.status == "held" && spectralHoldReasons.contains(record.reason)
        switch record.analysis {
        case "monodromy":
            guard (record.status == "accepted" || spectralHold),
                  record.universal.status == "accepted" else {
                throw invalid("monodromy requires accepted U1 and either accepted U2 or a typed downstream spectral hold")
            }
        case "tangent":
            guard record.status == "held", !spectralHold,
                  record.universal.status == "accepted",
                  tangentHoldReasons.contains(record.reason) else {
                throw invalid("a tangent hold requires accepted U1 and a pre-monodromy U2 hold")
            }
        case "held":
            let primalHold = record.universal.status == "held" && record.reason == "primal-held"
            let sourceBoundHold = record.universal.status == "accepted" &&
                sourceBoundEarlyHoldReasons.contains(record.reason)
            guard record.status == "held", primalHold || sourceBoundHold else {
                throw invalid("held analysis is neither a typed primal hold nor a typed source-bound U2 hold")
            }
        default:
            break
        }

        if let recurrence = record.recurrence {
            guard ["full", "relative"].contains(recurrence.kind) else {
                throw invalid("unknown recurrence kind \(recurrence.kind)")
            }
            guard ["full", "relative", "projected"].contains(recurrence.scope) else {
                throw invalid("unknown recurrence scope \(recurrence.scope)")
            }
            guard recurrence.scope == "projected" || recurrence.scope == recurrence.kind else {
                throw invalid("recurrence kind \(recurrence.kind) contradicts scope \(recurrence.scope)")
            }
            guard !recurrence.authorizesMonodromy || recurrence.verified else {
                throw invalid("unverified recurrence authorized monodromy")
            }
            guard !recurrence.verified || recurrence.discreteStateVerified else {
                throw invalid("recurrence was verified without matching discrete state")
            }
            guard recurrence.residual.isFinite,
                  recurrence.residual >= 0,
                  recurrence.normalizedResidual.isFinite,
                  recurrence.normalizedResidual >= 0,
                  (!recurrence.verified || recurrence.normalizedResidual <= 1),
                  recurrence.absoluteTolerance == record.tolerances.recurrenceAbsolute,
                  recurrence.relativeTolerance == record.tolerances.recurrenceRelative else {
                throw invalid("recurrence measurements contradict the declared tolerance contract")
            }
            if recurrence.restorationDerivativeApplied {
                guard recurrence.kind == "relative", recurrence.verified, recurrence.restoration != nil else {
                    throw invalid("restoration derivative was applied without verified relative recurrence")
                }
            }
        }

        if record.status == "held" {
            guard record.classification == "held",
                  record.multipliers == nil,
                  record.spectrumDiagnostics == nil,
                  record.backend == nil else {
                throw invalid("held analysis exposed downstream spectrum data")
            }
            if spectralHold {
                guard let recurrence = record.recurrence,
                      recurrence.verified,
                      recurrence.discreteStateVerified,
                      recurrence.authorizesMonodromy else {
                    throw invalid("downstream spectral hold lacks verified recurrence and monodromy authorization")
                }
            } else if let recurrence = record.recurrence {
                guard !recurrence.verified, !recurrence.authorizesMonodromy else {
                    throw invalid("recurrence-stage hold claimed verification or monodromy authorization")
                }
            }
        }

        if record.analysis == "held" {
            guard record.status == "held",
                  record.methodDetail == nil,
                  record.finiteDifference == nil,
                  record.eventBudget == nil,
                  record.neutralModeRemoval == nil,
                  record.recurrence == nil,
                  record.discreteState == nil,
                  record.initialState == nil,
                  record.finalState == nil,
                  record.pathTangentDigest == nil,
                  record.pathTangent == nil,
                  record.events.isEmpty,
                  record.monodromyDigest == nil,
                  record.monodromy == nil,
                  record.multipliers == nil,
                  record.spectrumDiagnostics == nil,
                  record.backend == nil else {
                throw invalid("early hold fabricated tangent or downstream U2 state")
            }
            if record.universal.status == "held" {
                guard record.reason == "primal-held",
                      record.manifestDigest == nil,
                      record.dimension == nil,
                      record.coordinates == nil else {
                    throw invalid("primal hold fabricated a source-bound U2 manifest")
                }
            } else if record.reason == "state-manifest-mismatch" {
                guard record.manifestDigest == nil,
                      record.dimension == nil,
                      record.coordinates == nil else {
                    throw invalid("failed state manifest exposed an unverified layout")
                }
            } else {
                try validateLayoutMetadata(record)
            }
            return
        }

        guard let dimension = record.dimension, dimension > 0,
              let coordinates = record.coordinates,
              coordinates.count == dimension,
              let initialState = record.initialState,
              initialState.count == dimension,
              let finalState = record.finalState,
              finalState.count == dimension,
              record.pathTangent?.count == dimension * dimension,
              let pathTangentDigest = record.pathTangentDigest,
              record.manifestDigest != nil,
              let recurrence = record.recurrence,
              let discreteState = record.discreteState,
              record.methodDetail != nil,
              record.finiteDifference != nil,
              record.eventBudget != nil,
              record.neutralModeRemoval != nil else {
            throw invalid("tangent carrier dimensions or required fields are inconsistent")
        }
        try validateLayoutMetadata(record)
        try validateDeclaredMethod(record)
        try validateDiscreteState(discreteState, recurrence: recurrence)
        try validateDigest(pathTangentDigest, field: "pathTangentDigest")
        guard initialState.allSatisfy(\.isFinite),
              finalState.allSatisfy(\.isFinite),
              record.pathTangent?.allSatisfy(\.isFinite) == true,
              record.events.allSatisfy({ $0.time.isFinite }) else {
            throw invalid("tangent carrier contains non-finite state or event data")
        }

        if let restoration = recurrence.restoration {
            try validateRestoration(
                restoration,
                record: record,
                coordinates: coordinates,
                dimension: dimension
            )
        }
        try validateRecurrenceMeasurements(
            recurrence,
            record: record,
            coordinates: coordinates,
            initialState: initialState,
            finalState: finalState
        )

        if record.status == "accepted" {
            guard record.analysis == "monodromy",
                  recurrence.verified,
                  recurrence.discreteStateVerified,
                  recurrence.authorizesMonodromy,
                  recurrence.scope != "projected",
                  record.monodromy?.count == dimension * dimension,
                  record.monodromyDigest != nil,
                  record.multipliers?.count == dimension,
                  record.spectrumDiagnostics != nil,
                  record.backend != nil,
                  record.classification != "held" else {
                throw invalid("accepted analysis lacks verified recurrence or complete spectrum")
            }
            if recurrence.kind == "relative" {
                try validateAuthorizedRestoration(recurrence)
            } else {
                try validateAuthorizedRestoration(recurrence)
            }
            try validateSpectrum(record, dimension: dimension)
        } else if spectralHold {
            guard record.analysis == "monodromy",
                  recurrence.scope != "projected",
                  record.monodromy?.count == dimension * dimension,
                  record.monodromy?.allSatisfy(\.isFinite) == true,
                  record.monodromyDigest.map(isCanonicalDigest) == true else {
                throw invalid("downstream spectral hold did not retain the authorized monodromy artifact")
            }
            try validateAuthorizedRestoration(recurrence)
        } else {
            guard record.analysis == "tangent",
                  record.monodromy == nil,
                  record.monodromyDigest == nil,
                  record.spectrumDiagnostics == nil else {
                throw invalid("recurrence-stage hold exposed downstream monodromy or spectrum state")
            }
        }
    }

    private static func validateMethodNames(_ record: U2StabilityRecord) throws {
        guard record.methods.flow == "explicit-euler-map",
              record.methods.commit == "scheduled-fixed-mode-reset",
              record.methods.nest == "fixed-trit-overwrite",
              record.methods.guard == "unbound-no-saltation",
              record.methods.spectrum == "validated-real-schur" else {
            throw invalid("method names contradict the executable v1 contract")
        }
    }

    private static func validateLayoutMetadata(_ record: U2StabilityRecord) throws {
        guard let dimension = record.dimension,
              dimension > 0,
              let coordinates = record.coordinates,
              coordinates.count == dimension,
              let manifestDigest = record.manifestDigest,
              isCanonicalDigest(manifestDigest),
              Set(coordinates.map(\.name)).count == coordinates.count,
              coordinates.allSatisfy({ !$0.name.isEmpty && $0.period.isFinite && $0.period >= 0 }) else {
            throw invalid("source-bound state manifest is missing or dimensionally inconsistent")
        }
    }

    private static func validateRestoration(
        _ restoration: U2StabilityRecord.Recurrence.Restoration,
        record: U2StabilityRecord,
        coordinates: [U2StabilityRecord.Coordinate],
        dimension: Int
    ) throws {
        guard record.recurrence?.kind == "relative",
              restoration.action == "cover-phase-translation",
              restoration.coordinateIndex >= 0,
              restoration.coordinateIndex < dimension,
              coordinates[restoration.coordinateIndex].name == restoration.coordinate,
              coordinates[restoration.coordinateIndex].period == 0,
              restoration.displacement.isFinite,
              matchesRuntimeFloat(restoration.displacement, 4 * Double.pi),
              restoration.equivarianceWitness.id == "isolated-affine-two-turn-cover",
              restoration.equivarianceWitness.verified,
              restoration.equivarianceWitness.fieldGain == 0,
              restoration.equivarianceWitness.fieldCellCount == 1,
              restoration.equivarianceWitness.incidentChannelCount == 0,
              restoration.equivarianceWitness.mutatingStepCount == 0,
              restoration.sectionWitness.id == "u1-two-turn-returned-restored",
              restoration.sectionWitness.verified,
              restoration.sectionWitness.winding == 2,
              restoration.sectionWitness.winding == record.universal.winding,
              restoration.sectionWitness.projection == "returned",
              restoration.sectionWitness.sheet == "restored",
              restoration.derivativeRows == dimension,
              restoration.derivativeColumns == dimension,
              restoration.derivative.count == dimension * dimension,
              restoration.derivative.allSatisfy(\.isFinite),
              isIdentity(restoration.derivative, dimension: dimension),
              isCanonicalDigest(restoration.derivativeDigest) else {
            throw invalid("relative restoration contradicts the executable v1 cover-phase action")
        }
    }

    private static func validateRecurrenceMeasurements(
        _ recurrence: U2StabilityRecord.Recurrence,
        record: U2StabilityRecord,
        coordinates: [U2StabilityRecord.Coordinate],
        initialState: [Double],
        finalState: [Double]
    ) throws {
        if recurrence.scope == "projected" {
            // Schema v1 does not emit the include mask, so an exact independent
            // recomputation is impossible. The producer's own verification bit
            // must still agree with its reported normalized/discrete result.
            // Consumer admission remains fail-closed: only a false diagnostic
            // may be held, and it can never authorize monodromy.
            let producerVerified = recurrence.normalizedResidual <= 1 &&
                recurrence.discreteStateVerified
            guard recurrence.verified == producerVerified,
                  !producerVerified,
                  record.status == "held",
                  record.analysis == "tangent",
                  record.reason == "undeclared-quotient",
                  !recurrence.authorizesMonodromy,
                  !recurrence.restorationDerivativeApplied,
                  recurrence.restoration == nil else {
                throw invalid("projected recurrence cannot carry an authoritative U2 claim in schema v1")
            }
            return
        }

        if recurrence.kind == "relative", recurrence.restoration == nil {
            // The v1 executor can discover that a requested phase quotient is
            // not executable before recurrence_check runs. Its zeroed result
            // is a typed hold, never a measured or authorizing recurrence.
            guard record.status == "held",
                  record.analysis == "tangent",
                  record.reason == "undeclared-quotient",
                  recurrence.residual == 0,
                  recurrence.normalizedResidual == 0,
                  !recurrence.verified,
                  !recurrence.authorizesMonodromy,
                  !recurrence.discreteStateVerified,
                  !recurrence.restorationDerivativeApplied else {
                throw invalid("undeclared relative quotient fabricated an authoritative recurrence")
            }
            return
        }

        var candidate = finalState
        switch recurrence.kind {
        case "full":
            guard recurrence.scope == "full",
                  recurrence.restoration == nil,
                  !recurrence.restorationDerivativeApplied else {
                throw invalid("full recurrence exposed relative restoration state")
            }
        case "relative":
            guard recurrence.scope == "relative",
                  let restoration = recurrence.restoration else {
                throw invalid("nonprojected relative recurrence lacks its executable endpoint restoration")
            }
            candidate[restoration.coordinateIndex] -= restoration.displacement
            guard candidate[restoration.coordinateIndex].isFinite else {
                throw invalid("relative endpoint restoration produced a non-finite state")
            }
        default:
            throw invalid("unknown recurrence kind \(recurrence.kind)")
        }

        var expectedResidual = 0.0
        var expectedNormalizedResidual = 0.0
        for index in coordinates.indices {
            var difference = candidate[index] - initialState[index]
            let period = coordinates[index].period
            if period > 0 {
                difference = Darwin.fmod(difference, period)
                if difference > 0.5 * period {
                    difference -= period
                } else if difference < -0.5 * period {
                    difference += period
                }
            }
            let residual = Darwin.fabs(difference)
            let allowed = recurrence.absoluteTolerance + recurrence.relativeTolerance *
                Darwin.fmax(Darwin.fabs(candidate[index]), Darwin.fabs(initialState[index]))
            let normalized = residual / allowed
            guard residual.isFinite, allowed.isFinite, allowed > 0, normalized.isFinite else {
                throw invalid("recurrence recomputation escaped finite executable bounds")
            }
            expectedResidual = max(expectedResidual, residual)
            expectedNormalizedResidual = max(expectedNormalizedResidual, normalized)
        }

        guard matchesRuntimeFloat(recurrence.residual, expectedResidual),
              matchesRuntimeFloat(recurrence.normalizedResidual, expectedNormalizedResidual) else {
            throw invalid("reported recurrence residuals do not match the executable endpoint computation")
        }

        let expectedVerified = expectedNormalizedResidual <= 1 && recurrence.discreteStateVerified
        let expectedRestorationApplied = recurrence.kind == "relative" && expectedVerified
        let expectedAuthorization = expectedVerified
        guard recurrence.verified == expectedVerified,
              recurrence.restorationDerivativeApplied == expectedRestorationApplied,
              recurrence.authorizesMonodromy == expectedAuthorization else {
            throw invalid("recurrence verification or monodromy authorization contradicts recomputed residuals")
        }
    }

    private static func isIdentity(_ matrix: [Double], dimension: Int) -> Bool {
        for row in 0..<dimension {
            for column in 0..<dimension {
                let expected = row == column ? 1.0 : 0.0
                if matrix[row * dimension + column] != expected { return false }
            }
        }
        return true
    }

    private static func matchesRuntimeFloat(_ reported: Double, _ expected: Double) -> Bool {
        if reported == expected { return true }
        // The v1 machine receipt deliberately serializes finite reals with
        // fifteen significant decimal digits so adjacent libm results collapse
        // to one macOS/Linux artifact. Calculations and acceptance remain
        // binary64. Admit only that declared decimal boundary (plus the former
        // eight-ULP operation-order allowance), never the runtime tolerance.
        let ulpBudget = 8 * max(reported.ulp, expected.ulp)
        let scale = max(Darwin.fabs(reported), Darwin.fabs(expected))
        let serializationBudget = 5e-15 * scale
        return Darwin.fabs(reported - expected) <= max(ulpBudget, serializationBudget)
    }

    private static func validateAuthorizedRestoration(_ recurrence: U2StabilityRecord.Recurrence) throws {
        if recurrence.kind == "relative" {
            guard recurrence.scope == "relative",
                  recurrence.restorationDerivativeApplied,
                  recurrence.restoration != nil else {
                throw invalid("relative monodromy lacks a complete applied restoration witness")
            }
        } else {
            guard recurrence.scope == "full",
                  !recurrence.restorationDerivativeApplied,
                  recurrence.restoration == nil else {
                throw invalid("full monodromy exposed relative restoration state")
            }
        }
    }

    private static func validateDeclaredMethod(_ record: U2StabilityRecord) throws {
        guard record.methods.flow == "explicit-euler-map",
              record.methods.commit == "scheduled-fixed-mode-reset",
              record.methods.nest == "fixed-trit-overwrite",
              record.methods.guard == "unbound-no-saltation",
              record.methods.spectrum == "validated-real-schur",
              let detail = record.methodDetail,
              detail.flow.localMap == "explicit-euler-synchronous",
              detail.flow.jacobian == "analytic-exact",
              detail.flow.finiteDifferenceOracle == nil,
              detail.commit.kind == "scheduled-fixed-mode-reset",
              detail.commit.continuousDerivative == "identity-within-fixed-mode",
              detail.commit.saltation == "not-applicable",
              detail.nest.localMap == "fixed-trit-overwrite",
              detail.nest.jacobian == "analytic-exact",
              detail.nest.childPrior == "overwrite-parent-belief",
              detail.guard.binding == nil,
              detail.guard.eventLocalization == nil,
              detail.guard.saltation == nil,
              detail.spectrum.solver == "validated-real-schur",
              detail.spectrum.ordering == "sorted-multipliers" else {
            throw invalid("method detail contradicts the executable v1 method contract")
        }
        guard let finiteDifference = record.finiteDifference,
              finiteDifference.status == "not-run",
              finiteDifference.step == nil,
              finiteDifference.residual == nil else {
            throw invalid("finite-difference boundary must be explicit not-run with null measurements")
        }
        guard let eventBudget = record.eventBudget,
              eventBudget.status == "not-applicable",
              eventBudget.limit == nil,
              eventBudget.used == nil else {
            throw invalid("event-budget boundary must be explicit not-applicable with null counts")
        }
        guard let neutral = record.neutralModeRemoval,
              neutral.status == "not-requested",
              neutral.generatorDigest == nil,
              neutral.removedModes == nil else {
            throw invalid("neutral-mode boundary must be explicit not-requested with null artifacts")
        }
    }

    private static func validateDiscreteState(
        _ state: U2StabilityRecord.DiscreteState,
        recurrence: U2StabilityRecord.Recurrence
    ) throws {
        guard recurrence.discreteStateVerified == state.verified,
              isCanonicalDigest(state.initialDigest),
              isCanonicalDigest(state.finalDigest),
              state.initial.count == state.final.count else {
            throw invalid("discrete-state receipt disagrees with recurrence or is truncated")
        }
        let initialIndices = Set(state.initial.map(\.sourceIndex))
        let finalIndices = Set(state.final.map(\.sourceIndex))
        guard initialIndices.count == state.initial.count,
              finalIndices.count == state.final.count else {
            throw invalid("discrete-state receipt repeats a source index")
        }
        for entry in state.initial + state.final {
            guard entry.sourceIndex >= 0, !entry.cell.isEmpty, !entry.coordinate.isEmpty else {
                throw invalid("discrete-state entry lacks canonical identity")
            }
            if entry.hasLatch {
                guard let latch = entry.latch,
                      ["+", "0", "-"].contains(latch),
                      entry.mode == "latched:\(latch)" else {
                    throw invalid("latched discrete-state entry has contradictory mode data")
                }
            } else {
                guard entry.latch == nil, entry.mode == "unlatched" else {
                    throw invalid("unlatched discrete-state entry exposed a latch")
                }
            }
        }
        for (initial, final) in zip(state.initial, state.final) {
            guard initial.cell == final.cell,
                  initial.coordinate == final.coordinate,
                  initial.sourceIndex == final.sourceIndex else {
                throw invalid("discrete-state initial/final identity order changed")
            }
        }
        if state.verified {
            guard state.initialDigest == state.finalDigest, state.initial == state.final else {
                throw invalid("verified discrete state changed between endpoints")
            }
        }
    }

    private static func validateSpectrum(_ record: U2StabilityRecord, dimension: Int) throws {
        guard let monodromy = record.monodromy,
              monodromy.count == dimension * dimension,
              monodromy.allSatisfy(\.isFinite),
              record.monodromyDigest.map(isCanonicalDigest) == true,
              let multipliers = record.multipliers,
              multipliers.count == dimension,
              let diagnostics = record.spectrumDiagnostics,
              diagnostics.spectralRadius.isFinite,
              diagnostics.spectralRadius >= 0,
              diagnostics.schur.reconstructionResidual.isFinite,
              diagnostics.schur.orthogonalityResidual.isFinite,
              diagnostics.schur.triangularResidual.isFinite,
              diagnostics.schur.validationTolerance.isFinite,
              diagnostics.schur.validationTolerance == record.tolerances.schur,
              diagnostics.schur.reconstructionResidual <= diagnostics.schur.validationTolerance,
              diagnostics.schur.orthogonalityResidual <= diagnostics.schur.validationTolerance,
              diagnostics.schur.triangularResidual <= diagnostics.schur.validationTolerance else {
            throw invalid("accepted spectrum lacks finite, validated Schur diagnostics")
        }
        for multiplier in multipliers {
            guard multiplier.real.isFinite,
                  multiplier.imag.isFinite,
                  multiplier.modulus.isFinite,
                  multiplier.modulus >= 0,
                  multiplier.mode == "physical" else {
                throw invalid("spectrum contains an invalid multiplier")
            }
            let expectedModulus = hypot(multiplier.real, multiplier.imag)
            let modulusTolerance = max(record.tolerances.schur, 1e-12) * max(1, expectedModulus)
            guard abs(multiplier.modulus - expectedModulus) <= modulusTolerance else {
                throw invalid("multiplier modulus contradicts its real and imaginary components")
            }
        }
        let radius = multipliers.map(\.modulus).max() ?? 0
        let tolerance = max(record.tolerances.schur, 1e-12)
        guard abs(radius - diagnostics.spectralRadius) <= tolerance else {
            throw invalid("spectral radius contradicts the canonical multipliers")
        }
        let expectedClassification: String
        if multipliers.contains(where: { $0.modulus > 1 + record.tolerances.neutral }) {
            expectedClassification = "unstable"
        } else if multipliers.contains(where: { $0.modulus >= 1 - record.tolerances.neutral }) {
            expectedClassification = "marginal"
        } else {
            expectedClassification = "stable"
        }
        guard record.classification == expectedClassification else {
            throw invalid("classification contradicts the physical multiplier moduli")
        }
    }

    private static func validateRequiredFieldPresence(_ raw: String) throws {
        guard let object = try? JSONSerialization.jsonObject(with: Data(raw.utf8)),
              let receipt = object as? [String: Any] else {
            throw invalid("canonical JSON is not an object")
        }
        try require([
            "schema", "orbit", "status", "reason", "analysis", "runtimeIdentity",
            "sourceDigest", "sourceDigestSurface", "universal", "manifestDigest",
            "methods", "methodDetail", "tolerances", "finiteDifference", "eventBudget",
            "neutralModeRemoval", "determinism", "recurrence", "discreteState",
            "dimension", "coordinates", "initialState", "finalState", "pathTangentDigest",
            "pathTangent", "events", "monodromyDigest", "monodromy", "multipliers",
            "spectrumDiagnostics", "backend", "classification"
        ], in: receipt, context: "receipt")
        if let recurrence = receipt["recurrence"] as? [String: Any] {
            try require(["discreteStateVerified"], in: recurrence, context: "recurrence")
        }
        if let discrete = receipt["discreteState"] as? [String: Any] {
            try require(
                ["verified", "initialDigest", "finalDigest", "initial", "final"],
                in: discrete,
                context: "discreteState"
            )
        }
    }

    private static func require(
        _ keys: [String],
        in object: [String: Any],
        context: String
    ) throws {
        if let missing = keys.first(where: { object[$0] == nil }) {
            throw invalid("missing required \(context) field \(missing)")
        }
    }

    private static func validateDigest(_ digest: String, field: String) throws {
        guard isCanonicalDigest(digest) else {
            throw invalid("\(field) is not an exact blake3 digest")
        }
    }

    private static func isCanonicalDigest(_ digest: String) -> Bool {
        let prefix = "blake3:"
        guard digest.hasPrefix(prefix) else { return false }
        let hexadecimal = digest.dropFirst(prefix.count)
        return hexadecimal.count == 64 && hexadecimal.allSatisfy {
            ($0 >= "0" && $0 <= "9") || ($0 >= "a" && $0 <= "f")
        }
    }

    private static func invalid(_ detail: String) -> U2ReceiptFailure {
        U2ReceiptFailure(kind: .invalidShape(detail), detail: "Canonical U2 receipt rejected: \(detail).")
    }
}

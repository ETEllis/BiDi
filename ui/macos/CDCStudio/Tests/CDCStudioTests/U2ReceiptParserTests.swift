import Foundation
import XCTest
@testable import CDCStudio

final class U2ReceiptParserTests: XCTestCase {
    func testAcceptedRelativeReceiptDecodesAndBuildsCompleteOrbit() throws {
        let receipts = try U2ReceiptParser.parse("notice\nu2-json=\(acceptedJSON)\ndone\n")
        XCTAssertEqual(receipts.count, 1)
        let record = try XCTUnwrap(receipts.first?.record)
        XCTAssertEqual(record.status, "accepted")
        XCTAssertEqual(record.recurrence?.restoration?.action, "cover-phase-translation")
        XCTAssertEqual(record.multipliers?.count, 1)

        let orbit = EvidenceOrbit(receipt: receipts[0])
        XCTAssertEqual(orbit.stages.map(\.kind), EvidenceStageKind.allCases)
        XCTAssertTrue(orbit.claimCeiling.contains("Monodromy analyzed as marginal"))
    }

    func testTangentHoldHasNoSpectrumAndKeepsRecurrenceReason() throws {
        let receipt = try XCTUnwrap(U2ReceiptParser.parse("u2-json=\(tangentHoldJSON)").first)
        XCTAssertEqual(receipt.record.analysis, "tangent")
        XCTAssertNil(receipt.record.monodromy)
        XCTAssertNil(receipt.record.multipliers)

        let orbit = EvidenceOrbit(receipt: receipt)
        XCTAssertTrue(orbit.claimCeiling.contains("recurrence-mode-mismatch"))
        XCTAssertEqual(orbit.stages.last?.state.label, "unavailable")
    }

    func testPrimalHoldCannotFabricateU2State() throws {
        let receipt = try XCTUnwrap(U2ReceiptParser.parse("u2-json=\(primalHoldJSON)").first)
        XCTAssertEqual(receipt.record.analysis, "held")
        XCTAssertNil(receipt.record.dimension)
        XCTAssertTrue(EvidenceOrbit(receipt: receipt).claimCeiling.contains("cone-not-reciprocal"))
    }

    func testNoCanonicalRecordFailsClosed() {
        XCTAssertThrowsError(try U2ReceiptParser.parse("human prose only")) { error in
            XCTAssertEqual((error as? U2ReceiptFailure)?.kind, .noCanonicalRecord)
        }
    }

    func testMalformedJSONFailsClosed() {
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json={not-json}")) { error in
            guard case .malformedJSON = (error as? U2ReceiptFailure)?.kind else {
                return XCTFail("wrong parser failure: \(error)")
            }
        }
    }

    func testUnsupportedSchemaFailsClosed() {
        let changed = acceptedJSON.replacingOccurrences(of: "cdc.u2.stability.v1", with: "cdc.u2.stability.v2")
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(changed)")) { error in
            XCTAssertEqual((error as? U2ReceiptFailure)?.kind, .unsupportedSchema("cdc.u2.stability.v2"))
        }
    }

    func testHeldReceiptCannotExposeSpectrum() {
        let invalid = tangentHoldJSON
            .replacingOccurrences(of: "\"multipliers\":null", with: "\"multipliers\":[{\"real\":1,\"imag\":0,\"modulus\":1,\"mode\":\"physical\"}]")
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    func testDuplicateOrbitAndDigestDisagreementFailClosed() {
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(acceptedJSON)\nu2-json=\(acceptedJSON)"))
        let second = mutate(tangentHoldJSON) {
            $0["sourceDigest"] = "blake3:\(String(repeating: "9", count: 64))"
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(acceptedJSON)\nu2-json=\(second)"))
    }

    func testAcceptedU2CannotContradictHeldUniversalState() {
        let invalid = acceptedJSON.replacingOccurrences(
            of: "\"id\":\"u1\",\"status\":\"accepted\",\"reason\":\"none\"",
            with: "\"id\":\"u1\",\"status\":\"held\",\"reason\":\"cone-not-reciprocal\""
        )
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    func testTangentCarrierRequiresAcceptedUniversalState() {
        let invalid = tangentHoldJSON.replacingOccurrences(
            of: "\"id\":\"u1\",\"status\":\"accepted\",\"reason\":\"none\"",
            with: "\"id\":\"u1\",\"status\":\"held\",\"reason\":\"cone-not-reciprocal\""
        )
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    func testPrimalHoldCannotClaimAcceptedUniversalState() {
        let invalid = primalHoldJSON.replacingOccurrences(
            of: "\"id\":\"u1\",\"status\":\"held\",\"reason\":\"cone-not-reciprocal\"",
            with: "\"id\":\"u1\",\"status\":\"accepted\",\"reason\":\"none\""
        )
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    func testRecurrenceVocabularyAndKindScopeContradictionsFailClosed() {
        let unknownKind = tangentHoldJSON.replacingOccurrences(of: "\"kind\":\"full\"", with: "\"kind\":\"orbifold\"")
        let unknownScope = tangentHoldJSON.replacingOccurrences(of: "\"scope\":\"full\"", with: "\"scope\":\"ambient\"")
        let crossed = tangentHoldJSON.replacingOccurrences(of: "\"kind\":\"full\",\"scope\":\"full\"", with: "\"kind\":\"relative\",\"scope\":\"full\"")

        for invalid in [unknownKind, unknownScope, crossed] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testHeldRecurrenceCannotClaimVerificationOrAuthorization() {
        let verified = tangentHoldJSON.replacingOccurrences(of: "\"verified\":false,\"authorizesMonodromy\":false", with: "\"verified\":true,\"authorizesMonodromy\":false")
        let authorized = tangentHoldJSON.replacingOccurrences(of: "\"verified\":false,\"authorizesMonodromy\":false", with: "\"verified\":false,\"authorizesMonodromy\":true")

        for invalid in [verified, authorized] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testDownstreamSpectralHoldRetainsVerifiedMonodromyWithoutSpectrum() throws {
        let receipt = try XCTUnwrap(U2ReceiptParser.parse("u2-json=\(spectralHoldJSON)").first)
        XCTAssertEqual(receipt.record.analysis, "monodromy")
        XCTAssertTrue(receipt.record.recurrence?.verified == true)
        XCTAssertTrue(receipt.record.recurrence?.authorizesMonodromy == true)
        XCTAssertNotNil(receipt.record.monodromy)
        XCTAssertNil(receipt.record.multipliers)
        XCTAssertNil(receipt.record.spectrumDiagnostics)

        let orbit = EvidenceOrbit(receipt: receipt)
        XCTAssertEqual(orbit.stages[3].state.label, "analyzed")
        XCTAssertEqual(orbit.stages[4].state.label, "held")
        XCTAssertTrue(orbit.claimCeiling.contains("monodromy retained"))
    }

    func testSpectralHoldStageContradictionsFailClosed() {
        let missingMonodromy = mutate(spectralHoldJSON) {
            $0["monodromy"] = NSNull()
            $0["monodromyDigest"] = NSNull()
        }
        let tangentCarrier = mutate(spectralHoldJSON) { $0["analysis"] = "tangent" }
        let leakedSpectrum = mutate(spectralHoldJSON) {
            $0["multipliers"] = [["real": 1, "imag": 0, "modulus": 1, "mode": "physical"]]
        }

        for invalid in [missingMonodromy, tangentCarrier, leakedSpectrum] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testExpandedV1MissingAndTruncatedFieldsFailClosed() {
        let missingMethodDetail = mutate(acceptedJSON) { $0.removeValue(forKey: "methodDetail") }
        let missingDiagnostics = mutate(acceptedJSON) { $0.removeValue(forKey: "spectrumDiagnostics") }
        let missingDiscreteFlag = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence.removeValue(forKey: "discreteStateVerified")
            receipt["recurrence"] = recurrence
        }
        let truncatedDiscreteState = mutate(acceptedJSON) { receipt in
            var discrete = receipt["discreteState"] as! [String: Any]
            discrete.removeValue(forKey: "final")
            receipt["discreteState"] = discrete
        }
        let syntacticallyTruncated = String(acceptedJSON.dropLast())

        for invalid in [missingMethodDetail, missingDiagnostics, missingDiscreteFlag, truncatedDiscreteState, syntacticallyTruncated] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testExpandedV1SemanticContradictionsFailClosed() {
        let discreteDisagreement = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["discreteStateVerified"] = false
            receipt["recurrence"] = recurrence
        }
        let inventedFiniteDifference = mutate(acceptedJSON) { receipt in
            var finiteDifference = receipt["finiteDifference"] as! [String: Any]
            finiteDifference["status"] = "verified"
            finiteDifference["step"] = 1e-6
            finiteDifference["residual"] = 0
            receipt["finiteDifference"] = finiteDifference
        }
        let falseSpectralRadius = mutate(acceptedJSON) { receipt in
            var diagnostics = receipt["spectrumDiagnostics"] as! [String: Any]
            diagnostics["spectralRadius"] = 2
            receipt["spectrumDiagnostics"] = diagnostics
        }
        let changedVerifiedDiscreteEntry = mutate(acceptedJSON) { receipt in
            var discrete = receipt["discreteState"] as! [String: Any]
            var final = discrete["final"] as! [[String: Any]]
            final[0]["hasLatch"] = true
            final[0]["latch"] = "+"
            final[0]["mode"] = "latched:+"
            discrete["final"] = final
            receipt["discreteState"] = discrete
        }

        for invalid in [discreteDisagreement, inventedFiniteDifference, falseSpectralRadius, changedVerifiedDiscreteEntry] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testTypedSourceBoundEarlyHoldsAdmitOnlyRuntimeEmittedShapes() throws {
        for reason in ["unsupported-delay-state", "nondifferentiable-quantization"] {
            let receipt = try XCTUnwrap(U2ReceiptParser.parse(
                "u2-json=\(sourceBoundEarlyHoldJSON(reason: reason))"
            ).first)
            XCTAssertEqual(receipt.record.analysis, "held")
            XCTAssertEqual(receipt.record.universal.status, "accepted")
            XCTAssertEqual(receipt.record.dimension, 1)
            XCTAssertNil(receipt.record.pathTangent)
        }

        let manifestFailure = try XCTUnwrap(U2ReceiptParser.parse(
            "u2-json=\(sourceBoundEarlyHoldJSON(reason: "state-manifest-mismatch", retainsLayout: false))"
        ).first)
        XCTAssertNil(manifestFailure.record.manifestDigest)
        XCTAssertNil(manifestFailure.record.dimension)
        XCTAssertNil(manifestFailure.record.coordinates)
    }

    func testSourceBoundEarlyHoldContradictionsFailClosed() {
        let bogusReason = sourceBoundEarlyHoldJSON(reason: "invented-early-hold")
        let leakedTangent = mutate(sourceBoundEarlyHoldJSON(reason: "unsupported-delay-state")) {
            $0["initialState"] = [0.0]
        }
        let missingBoundLayout = sourceBoundEarlyHoldJSON(
            reason: "unsupported-delay-state",
            retainsLayout: false
        )
        let falseManifestFailure = sourceBoundEarlyHoldJSON(
            reason: "state-manifest-mismatch",
            retainsLayout: true
        )

        for invalid in [bogusReason, leakedTangent, missingBoundLayout, falseManifestFailure] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testDisplayTruthIdentityReasonAndSpectrumContradictionsFailClosed() {
        let wrongRuntime = mutate(acceptedJSON) { $0["runtimeIdentity"] = "cdc-native-u2-v2" }
        let bogusTangentReason = mutate(tangentHoldJSON) { $0["reason"] = "invented-hold" }
        let retiredSpectrumReason = mutate(spectralHoldJSON) { $0["reason"] = "spectrum-residual" }
        let falseModulus = mutate(acceptedJSON) { receipt in
            var multipliers = receipt["multipliers"] as! [[String: Any]]
            multipliers[0]["modulus"] = 0.5
            receipt["multipliers"] = multipliers
        }
        let falseClassification = mutate(acceptedJSON) { $0["classification"] = "stable" }
        let inventedGauge = mutate(acceptedJSON) { receipt in
            var multipliers = receipt["multipliers"] as! [[String: Any]]
            multipliers[0]["mode"] = "gauge"
            receipt["multipliers"] = multipliers
        }
        let missingRelativeRestoration = mutate(spectralHoldJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["restorationDerivativeApplied"] = false
            recurrence["restoration"] = NSNull()
            receipt["recurrence"] = recurrence
        }

        for invalid in [
            wrongRuntime, bogusTangentReason, retiredSpectrumReason, falseModulus,
            falseClassification, inventedGauge, missingRelativeRestoration
        ] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testReservedSpectralReasonIsNotAdmittedByCurrentV1() {
        let invalid = mutate(spectralHoldJSON) { $0["reason"] = "neutral-mode-unverified" }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    func testVariationalValidationFailureIsAdmittedOnlyAsDownstreamSpectralHold() throws {
        let forgedEarly = sourceBoundEarlyHoldJSON(reason: "variational-validation-failed")
        let forgedTangent = mutate(tangentHoldJSON) { $0["reason"] = "variational-validation-failed" }
        for invalid in [forgedEarly, forgedTangent] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }

        let spectral = mutate(spectralHoldJSON) { $0["reason"] = "variational-validation-failed" }
        XCTAssertEqual(
            try XCTUnwrap(U2ReceiptParser.parse("u2-json=\(spectral)").first).record.analysis,
            "monodromy"
        )
    }

    func testEveryShapeDigestRequiresExactLowercaseBlake3Form() {
        let invalidSource = mutate(acceptedJSON) { $0["sourceDigest"] = "sha256:deadbeef" }
        let invalidUniversal = mutate(acceptedJSON) { receipt in
            var universal = receipt["universal"] as! [String: Any]
            universal["resultDigest"] = "blake3:\(String(repeating: "A", count: 64))"
            receipt["universal"] = universal
        }
        let invalidManifest = mutate(acceptedJSON) { $0["manifestDigest"] = "blake3:1234" }
        let invalidPath = mutate(acceptedJSON) { $0["pathTangentDigest"] = "path" }
        let invalidMonodromy = mutate(acceptedJSON) { $0["monodromyDigest"] = "" }
        let invalidRestoration = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            var restoration = recurrence["restoration"] as! [String: Any]
            restoration["derivativeDigest"] = "blake3:\(String(repeating: "g", count: 64))"
            recurrence["restoration"] = restoration
            receipt["recurrence"] = recurrence
        }
        let invalidDiscreteInitial = mutate(acceptedJSON) { receipt in
            var discrete = receipt["discreteState"] as! [String: Any]
            discrete["initialDigest"] = "not-a-digest"
            receipt["discreteState"] = discrete
        }
        let invalidDiscreteFinal = mutate(tangentHoldJSON) { receipt in
            var discrete = receipt["discreteState"] as! [String: Any]
            discrete["finalDigest"] = "blake3:\(String(repeating: "0", count: 63))"
            receipt["discreteState"] = discrete
        }

        for invalid in [
            invalidSource, invalidUniversal, invalidManifest, invalidPath,
            invalidMonodromy, invalidRestoration, invalidDiscreteInitial,
            invalidDiscreteFinal
        ] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testDeterminismMustEqualCurrentV1Declaration() {
        let invalid = mutate(acceptedJSON) {
            $0["determinism"] = "row-major,source-order,canonical-float,sorted-multipliers"
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    func testRecurrenceResidualsAreNonnegativeAndVerifiedNormalizedResidualIsBounded() {
        let negativeResidual = mutate(tangentHoldJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["residual"] = -0.1
            receipt["recurrence"] = recurrence
        }
        let negativeNormalized = mutate(tangentHoldJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["normalizedResidual"] = -0.1
            receipt["recurrence"] = recurrence
        }
        let verifiedOutOfBound = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["normalizedResidual"] = 1.0000001
            receipt["recurrence"] = recurrence
        }

        for invalid in [negativeResidual, negativeNormalized, verifiedOutOfBound] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testRecurrenceReceiptMustMatchExecutableEndpointComputation() {
        let forgedRawResidual = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["residual"] = 1_000_000_000.0
            recurrence["normalizedResidual"] = 0.0
            receipt["recurrence"] = recurrence
        }
        let mismatchedPair = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["residual"] = 0.0
            recurrence["normalizedResidual"] = 0.5
            receipt["recurrence"] = recurrence
        }
        let mutatedEndpoint = mutate(acceptedJSON) { receipt in
            receipt["finalState"] = [12.816370614359172]
        }
        let mutatedPeriod = mutate(tangentHoldJSON) { receipt in
            var coordinates = receipt["coordinates"] as! [[String: Any]]
            coordinates[0]["period"] = 10.0
            receipt["coordinates"] = coordinates
        }
        let mutatedRestoration = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            var restoration = recurrence["restoration"] as! [String: Any]
            restoration["displacement"] = 12.0
            recurrence["restoration"] = restoration
            receipt["recurrence"] = recurrence
        }
        let inventedRestorationAction = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            var restoration = recurrence["restoration"] as! [String: Any]
            restoration["action"] = "generic-translation"
            recurrence["restoration"] = restoration
            receipt["recurrence"] = recurrence
        }

        for invalid in [
            forgedRawResidual, mismatchedPair, mutatedEndpoint, mutatedPeriod,
            mutatedRestoration, inventedRestorationAction
        ] {
            XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
        }
    }

    func testRecurrenceRecomputationUsesAbsolutePlusRelativeTolerance() throws {
        let normalized = 0.5 / (0.1 + 0.001 * 1000.5)
        let legitimate = mutate(tangentHoldJSON) { receipt in
            var tolerances = receipt["tolerances"] as! [String: Any]
            tolerances["recurrenceAbsolute"] = 0.1
            tolerances["recurrenceRelative"] = 0.001
            receipt["tolerances"] = tolerances
            receipt["initialState"] = [1000.0]
            receipt["finalState"] = [1000.5]
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["residual"] = 0.5
            recurrence["absoluteTolerance"] = 0.1
            recurrence["relativeTolerance"] = 0.001
            recurrence["normalizedResidual"] = normalized
            receipt["recurrence"] = recurrence
        }
        XCTAssertNoThrow(try U2ReceiptParser.parse("u2-json=\(legitimate)"))

        let forgedNormalized = mutate(legitimate) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["normalizedResidual"] = normalized + 0.01
            receipt["recurrence"] = recurrence
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(forgedNormalized)"))
    }

    func testProjectedV1ReceiptRemainsHeldAndNonAuthoritativeWithoutEmittedMask() throws {
        let projected = mutate(tangentHoldJSON) { receipt in
            receipt["reason"] = "undeclared-quotient"
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["scope"] = "projected"
            receipt["recurrence"] = recurrence
        }
        let record = try XCTUnwrap(U2ReceiptParser.parse("u2-json=\(projected)").first).record
        XCTAssertEqual(record.status, "held")
        XCTAssertFalse(record.recurrence?.verified == true)
        XCTAssertFalse(record.recurrence?.authorizesMonodromy == true)

        let forgedAuthority = mutate(projected) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["verified"] = true
            recurrence["discreteStateVerified"] = true
            recurrence["authorizesMonodromy"] = true
            receipt["recurrence"] = recurrence
            var discrete = receipt["discreteState"] as! [String: Any]
            discrete["verified"] = true
            discrete["finalDigest"] = discrete["initialDigest"]
            discrete["final"] = discrete["initial"]
            receipt["discreteState"] = discrete
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(forgedAuthority)"))

        let forgedProducerMismatch = mutate(projected) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["normalizedResidual"] = 0.0
            recurrence["discreteStateVerified"] = true
            recurrence["verified"] = false
            receipt["recurrence"] = recurrence
            var discrete = receipt["discreteState"] as! [String: Any]
            discrete["verified"] = true
            discrete["finalDigest"] = discrete["initialDigest"]
            discrete["final"] = discrete["initial"]
            receipt["discreteState"] = discrete
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(forgedProducerMismatch)"))
    }

    func testNonExecutableRelativeQuotientPlaceholderRemainsTypedNonAuthoritativeHold() throws {
        let undeclared = mutate(tangentHoldJSON) { receipt in
            receipt["reason"] = "undeclared-quotient"
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["kind"] = "relative"
            recurrence["scope"] = "relative"
            recurrence["residual"] = 0.0
            recurrence["normalizedResidual"] = 0.0
            recurrence["restoration"] = NSNull()
            receipt["recurrence"] = recurrence
        }
        let record = try XCTUnwrap(U2ReceiptParser.parse("u2-json=\(undeclared)").first).record
        XCTAssertEqual(record.reason, "undeclared-quotient")
        XCTAssertEqual(record.recurrence?.kind, "relative")
        XCTAssertNil(record.recurrence?.restoration)
        XCTAssertFalse(record.recurrence?.verified == true)
        XCTAssertFalse(record.recurrence?.authorizesMonodromy == true)

        let forgedMeasurement = mutate(undeclared) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            recurrence["residual"] = 0.25
            receipt["recurrence"] = recurrence
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(forgedMeasurement)"))
    }

    func testRestorationCoordinateMustMatchManifestCoordinateIndex() {
        let invalid = mutate(acceptedJSON) { receipt in
            var recurrence = receipt["recurrence"] as! [String: Any]
            var restoration = recurrence["restoration"] as! [String: Any]
            restoration["coordinate"] = "invented.coordinate"
            recurrence["restoration"] = restoration
            receipt["recurrence"] = recurrence
        }
        XCTAssertThrowsError(try U2ReceiptParser.parse("u2-json=\(invalid)"))
    }

    private var acceptedJSON: String {
        canonicalizeFixtureDigests(
        """
        {"schema":"cdc.u2.stability.v1","orbit":"relative","status":"accepted","reason":"none","analysis":"monodromy","runtimeIdentity":"cdc-native-u2-v1","sourceDigest":"digest-a","sourceDigestSurface":"grammar-1-canonical","universal":{"id":"u1","status":"accepted","reason":"none","resultDigest":"u1-digest","receptiveAngle":"0.375000","radiantAngle":"-0.250000","holonomy":"0.125000","winding":2},"manifestDigest":"manifest","methods":{"flow":"explicit-euler-map","commit":"scheduled-fixed-mode-reset","nest":"fixed-trit-overwrite","guard":"unbound-no-saltation","spectrum":"validated-real-schur"},"methodDetail":{"flow":{"localMap":"explicit-euler-synchronous","jacobian":"analytic-exact","finiteDifferenceOracle":null},"commit":{"kind":"scheduled-fixed-mode-reset","continuousDerivative":"identity-within-fixed-mode","saltation":"not-applicable"},"nest":{"localMap":"fixed-trit-overwrite","jacobian":"analytic-exact","childPrior":"overwrite-parent-belief"},"guard":{"binding":null,"eventLocalization":null,"saltation":null},"spectrum":{"solver":"validated-real-schur","ordering":"sorted-multipliers"}},"tolerances":{"recurrenceAbsolute":1e-6,"recurrenceRelative":0,"neutral":1e-9,"schur":1e-10},"finiteDifference":{"status":"not-run","step":null,"residual":null},"eventBudget":{"status":"not-applicable","limit":null,"used":null},"neutralModeRemoval":{"status":"not-requested","generatorDigest":null,"removedModes":null},"determinism":"source-order,row-major,canonical-float,sorted-multipliers","recurrence":{"kind":"relative","scope":"relative","residual":0,"absoluteTolerance":1e-6,"relativeTolerance":0,"normalizedResidual":0,"verified":true,"authorizesMonodromy":true,"discreteStateVerified":true,"restorationDerivativeApplied":true,"restoration":{"action":"cover-phase-translation","coordinateIndex":0,"coordinate":"cover.phase.theta","displacement":12.566370614359172,"equivarianceWitness":{"id":"isolated-affine-two-turn-cover","verified":true,"fieldGain":0,"fieldCellCount":1,"incidentChannelCount":0,"mutatingStepCount":0},"sectionWitness":{"id":"u1-two-turn-returned-restored","verified":true,"winding":2,"projection":"returned","sheet":"restored"},"derivativeRows":1,"derivativeColumns":1,"derivativeDigest":"rho","derivative":[1]}},"discreteState":{"verified":true,"initialDigest":"discrete-a","finalDigest":"discrete-a","initial":[{"cell":"cover.phase","coordinate":"cover.phase.theta","sourceIndex":0,"hasLatch":false,"latch":null,"mode":"unlatched"}],"final":[{"cell":"cover.phase","coordinate":"cover.phase.theta","sourceIndex":0,"hasLatch":false,"latch":null,"mode":"unlatched"}]},"dimension":1,"coordinates":[{"name":"cover.phase.theta","period":0}],"initialState":[0],"finalState":[12.566370614359172],"pathTangentDigest":"path","pathTangent":[1],"events":[{"kind":"flow","time":1,"transverse":null},{"kind":"endpoint-restoration","time":2,"transverse":null}],"monodromyDigest":"mono","monodromy":[1],"multipliers":[{"real":1,"imag":0,"modulus":1,"mode":"physical"}],"spectrumDiagnostics":{"spectralRadius":1,"schur":{"reconstructionResidual":0,"orthogonalityResidual":0,"triangularResidual":0,"validationTolerance":1e-10}},"backend":"lapack-dgees","classification":"marginal"}
        """
        )
    }

    private var tangentHoldJSON: String {
        canonicalizeFixtureDigests(
        """
        {"schema":"cdc.u2.stability.v1","orbit":"full-hold","status":"held","reason":"recurrence-mode-mismatch","analysis":"tangent","runtimeIdentity":"cdc-native-u2-v1","sourceDigest":"digest-a","sourceDigestSurface":"grammar-1-canonical","universal":{"id":"u1","status":"accepted","reason":"none","resultDigest":"u1-digest","receptiveAngle":"0.375000","radiantAngle":"-0.250000","holonomy":"0.125000","winding":2},"manifestDigest":"manifest","methods":{"flow":"explicit-euler-map","commit":"scheduled-fixed-mode-reset","nest":"fixed-trit-overwrite","guard":"unbound-no-saltation","spectrum":"validated-real-schur"},"methodDetail":{"flow":{"localMap":"explicit-euler-synchronous","jacobian":"analytic-exact","finiteDifferenceOracle":null},"commit":{"kind":"scheduled-fixed-mode-reset","continuousDerivative":"identity-within-fixed-mode","saltation":"not-applicable"},"nest":{"localMap":"fixed-trit-overwrite","jacobian":"analytic-exact","childPrior":"overwrite-parent-belief"},"guard":{"binding":null,"eventLocalization":null,"saltation":null},"spectrum":{"solver":"validated-real-schur","ordering":"sorted-multipliers"}},"tolerances":{"recurrenceAbsolute":1e-6,"recurrenceRelative":0,"neutral":1e-9,"schur":1e-10},"finiteDifference":{"status":"not-run","step":null,"residual":null},"eventBudget":{"status":"not-applicable","limit":null,"used":null},"neutralModeRemoval":{"status":"not-requested","generatorDigest":null,"removedModes":null},"determinism":"source-order,row-major,canonical-float,sorted-multipliers","recurrence":{"kind":"full","scope":"full","residual":12.5,"absoluteTolerance":1e-6,"relativeTolerance":0,"normalizedResidual":12500000,"verified":false,"authorizesMonodromy":false,"discreteStateVerified":false,"restorationDerivativeApplied":false,"restoration":null},"discreteState":{"verified":false,"initialDigest":"discrete-a","finalDigest":"discrete-b","initial":[{"cell":"cover.phase","coordinate":"cover.phase.theta","sourceIndex":0,"hasLatch":false,"latch":null,"mode":"unlatched"}],"final":[{"cell":"cover.phase","coordinate":"cover.phase.theta","sourceIndex":0,"hasLatch":true,"latch":"+","mode":"latched:+"}]},"dimension":1,"coordinates":[{"name":"cover.phase.theta","period":0}],"initialState":[0],"finalState":[12.5],"pathTangentDigest":"path","pathTangent":[1],"events":[{"kind":"flow","time":1,"transverse":null}],"monodromyDigest":null,"monodromy":null,"multipliers":null,"spectrumDiagnostics":null,"backend":null,"classification":"held"}
        """
        )
    }

    private var primalHoldJSON: String {
        canonicalizeFixtureDigests(
        """
        {"schema":"cdc.u2.stability.v1","orbit":"primal-hold","status":"held","reason":"primal-held","analysis":"held","runtimeIdentity":"cdc-native-u2-v1","sourceDigest":"digest-a","sourceDigestSurface":"grammar-1-canonical","universal":{"id":"u1","status":"held","reason":"cone-not-reciprocal","resultDigest":"u1-digest","receptiveAngle":"0.375000","radiantAngle":"-0.250000","holonomy":"0.125000","winding":0},"manifestDigest":null,"methods":{"flow":"explicit-euler-map","commit":"scheduled-fixed-mode-reset","nest":"fixed-trit-overwrite","guard":"unbound-no-saltation","spectrum":"validated-real-schur"},"methodDetail":null,"tolerances":{"recurrenceAbsolute":1e-6,"recurrenceRelative":0,"neutral":1e-9,"schur":1e-10},"finiteDifference":null,"eventBudget":null,"neutralModeRemoval":null,"determinism":"source-order,row-major,canonical-float,sorted-multipliers","recurrence":null,"discreteState":null,"dimension":null,"coordinates":null,"initialState":null,"finalState":null,"pathTangentDigest":null,"pathTangent":null,"events":[],"monodromyDigest":null,"monodromy":null,"multipliers":null,"spectrumDiagnostics":null,"backend":null,"classification":"held"}
        """
        )
    }

    private var spectralHoldJSON: String {
        return mutate(acceptedJSON) { receipt in
            receipt["status"] = "held"
            receipt["reason"] = "spectral-backend-unavailable"
            receipt["analysis"] = "monodromy"
            receipt["multipliers"] = NSNull()
            receipt["spectrumDiagnostics"] = NSNull()
            receipt["backend"] = NSNull()
            receipt["classification"] = "held"
        }
    }

    private func sourceBoundEarlyHoldJSON(reason: String, retainsLayout: Bool = true) -> String {
        mutate(acceptedJSON) { receipt in
            receipt["status"] = "held"
            receipt["reason"] = reason
            receipt["analysis"] = "held"
            receipt["methodDetail"] = NSNull()
            receipt["finiteDifference"] = NSNull()
            receipt["eventBudget"] = NSNull()
            receipt["neutralModeRemoval"] = NSNull()
            receipt["recurrence"] = NSNull()
            receipt["discreteState"] = NSNull()
            receipt["initialState"] = NSNull()
            receipt["finalState"] = NSNull()
            receipt["pathTangentDigest"] = NSNull()
            receipt["pathTangent"] = NSNull()
            receipt["events"] = []
            receipt["monodromyDigest"] = NSNull()
            receipt["monodromy"] = NSNull()
            receipt["multipliers"] = NSNull()
            receipt["spectrumDiagnostics"] = NSNull()
            receipt["backend"] = NSNull()
            receipt["classification"] = "held"
            if !retainsLayout {
                receipt["manifestDigest"] = NSNull()
                receipt["dimension"] = NSNull()
                receipt["coordinates"] = NSNull()
            }
        }
    }

    private func mutate(
        _ json: String,
        _ transform: (inout [String: Any]) -> Void
    ) -> String {
        var receipt = try! JSONSerialization.jsonObject(with: Data(json.utf8)) as! [String: Any]
        transform(&receipt)
        let data = try! JSONSerialization.data(withJSONObject: receipt, options: [.sortedKeys])
        return String(decoding: data, as: UTF8.self)
    }

    private func canonicalizeFixtureDigests(_ json: String) -> String {
        let replacements = [
            ("\"digest-a\"", "\"blake3:\(String(repeating: "a", count: 64))\""),
            ("\"u1-digest\"", "\"blake3:\(String(repeating: "b", count: 64))\""),
            ("\"manifest\"", "\"blake3:\(String(repeating: "c", count: 64))\""),
            ("\"rho\"", "\"blake3:\(String(repeating: "d", count: 64))\""),
            ("\"discrete-a\"", "\"blake3:\(String(repeating: "e", count: 64))\""),
            ("\"discrete-b\"", "\"blake3:\(String(repeating: "f", count: 64))\""),
            ("\"path\"", "\"blake3:\(String(repeating: "1", count: 64))\""),
            ("\"mono\"", "\"blake3:\(String(repeating: "2", count: 64))\"")
        ]
        return replacements.reduce(json) { result, replacement in
            result.replacingOccurrences(of: replacement.0, with: replacement.1)
        }
    }
}

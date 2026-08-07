import Foundation
import XCTest
@testable import CDCStudio

final class LiveU2IntegrationTests: XCTestCase {
    func testRealRuntimeCoversCanonicalHoldRelativeAcceptanceAndPrimalHold() throws {
        let testFile = URL(fileURLWithPath: #filePath)
        let root = try XCTUnwrap(
            RepositoryDiscovery.findAncestor(startingAt: testFile),
            "could not locate the authoritative CDC checkout"
        )
        let unified = root.appendingPathComponent("build/cdc")
        let integration = root.appendingPathComponent("build/u2/cdc_native_runtime")
        let binary: URL
        if FileManager.default.isExecutableFile(atPath: unified.path) {
            binary = unified
        } else if FileManager.default.isExecutableFile(atPath: integration.path) {
            binary = integration
        } else {
            throw XCTSkip("build/cdc and build/u2/cdc_native_runtime are unavailable; run ./scripts/verify_u2.sh")
        }

        let canonical = try run(binary, source: "framework_loop.cdc", root: root)
        let canonicalReceipts = try U2ReceiptParser.parse(canonical)
        XCTAssertEqual(canonicalReceipts.first?.record.orbit, "loop-u720-full")
        XCTAssertEqual(canonicalReceipts.first?.record.universal.status, "accepted")
        XCTAssertEqual(canonicalReceipts.first?.record.analysis, "tangent")
        XCTAssertEqual(canonicalReceipts.first?.record.reason, "recurrence-mode-mismatch")
        XCTAssertEqual(canonicalReceipts.first?.record.finiteDifference?.status, "not-run")
        XCTAssertEqual(canonicalReceipts.first?.record.eventBudget?.status, "not-applicable")
        XCTAssertEqual(canonicalReceipts.first?.record.neutralModeRemoval?.status, "not-requested")
        XCTAssertEqual(canonicalReceipts.first?.record.recurrence?.discreteStateVerified, false)
        XCTAssertNotNil(canonicalReceipts.first?.record.discreteState)
        XCTAssertNil(canonicalReceipts.first?.record.multipliers)

        let positive = try run(binary, source: "tests/fixtures/u2/u720_true_relative_marginal.cdc", root: root)
        let positiveReceipt = try XCTUnwrap(U2ReceiptParser.parse(positive).first)
        XCTAssertEqual(positiveReceipt.record.status, "accepted")
        XCTAssertEqual(positiveReceipt.record.recurrence?.scope, "relative")
        XCTAssertEqual(positiveReceipt.record.dimension, 13)
        XCTAssertEqual(positiveReceipt.record.classification, "marginal")
        XCTAssertEqual(positiveReceipt.record.multipliers?.count, 13)
        XCTAssertEqual(positiveReceipt.record.recurrence?.discreteStateVerified, true)
        XCTAssertEqual(positiveReceipt.record.discreteState?.verified, true)
        XCTAssertNotNil(positiveReceipt.record.methodDetail)
        XCTAssertNotNil(positiveReceipt.record.spectrumDiagnostics)

        let primal = try run(binary, source: "tests/fixtures/u2/u720_primal_hold.cdc", root: root)
        let primalReceipt = try XCTUnwrap(U2ReceiptParser.parse(primal).first)
        XCTAssertEqual(primalReceipt.record.analysis, "held")
        XCTAssertEqual(primalReceipt.record.universal.reason, "cone-not-reciprocal")
        XCTAssertNil(primalReceipt.record.methodDetail)
        XCTAssertNil(primalReceipt.record.discreteState)
        XCTAssertNil(primalReceipt.record.dimension)
        XCTAssertNil(primalReceipt.record.monodromy)
    }

    func testRealRuntimeSourceBoundEarlyHoldsPreserveOnlyVerifiedManifestMetadata() throws {
        let testFile = URL(fileURLWithPath: #filePath)
        let root = try XCTUnwrap(
            RepositoryDiscovery.findAncestor(startingAt: testFile),
            "could not locate the authoritative CDC checkout"
        )
        let binary = try authoritativeRuntime(root: root)
        let scratch = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-early-holds-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: scratch, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: scratch) }

        let positive = try String(
            contentsOf: root.appendingPathComponent("tests/fixtures/u2/u720_true_relative_marginal.cdc"),
            encoding: .utf8
        )
        let delayRange = try XCTUnwrap(positive.range(of: "delay=0.0"))
        let delayed = positive.replacingCharacters(in: delayRange, with: "delay=0.25")
        let delayURL = scratch.appendingPathComponent("unsupported-delay.cdc")
        try delayed.write(to: delayURL, atomically: true, encoding: .utf8)
        let delayReceipt = try XCTUnwrap(U2ReceiptParser.parse(
            try run(binary, source: delayURL.path, root: root)
        ).first)
        try assertSourceBoundEarlyHold(delayReceipt, reason: "unsupported-delay-state")

        let loop = try String(contentsOf: root.appendingPathComponent("framework_loop.cdc"), encoding: .utf8)
        let nondifferentiable = loop
            .replacingOccurrences(of: "field loop-field dt=0.125 gain=1.0 deadband=0.5", with: "field loop-field dt=0.125 gain=1.0 deadband=6.123233995736766e-17")
            .split(separator: "\n", omittingEmptySubsequences: false)
            .map { line -> String in
                let text = String(line)
                let prefixes = [
                    "commit loop-act-", "trace loop-record", "bridge loop-key",
                    "council loop-council", "universal loop-u720"
                ]
                guard prefixes.contains(where: text.hasPrefix),
                      let expectations = text.range(of: " expect-") else { return text }
                return String(text[..<expectations.lowerBound])
            }
            .joined(separator: "\n")
        let nondifferentiableURL = scratch.appendingPathComponent("nondifferentiable-quantization.cdc")
        try nondifferentiable.write(to: nondifferentiableURL, atomically: true, encoding: .utf8)
        let nondifferentiableReceipt = try XCTUnwrap(U2ReceiptParser.parse(
            try run(binary, source: nondifferentiableURL.path, root: root)
        ).first)
        try assertSourceBoundEarlyHold(
            nondifferentiableReceipt,
            reason: "nondifferentiable-quantization"
        )
    }

    private func authoritativeRuntime(root: URL) throws -> URL {
        for candidate in ["build/cdc", "build/u2/cdc_native_runtime"] {
            let url = root.appendingPathComponent(candidate)
            if FileManager.default.isExecutableFile(atPath: url.path) { return url }
        }
        throw XCTSkip("build/cdc and build/u2/cdc_native_runtime are unavailable; run ./scripts/verify_u2.sh")
    }

    private func assertSourceBoundEarlyHold(
        _ receipt: CanonicalU2Receipt,
        reason: String,
        file: StaticString = #filePath,
        line: UInt = #line
    ) throws {
        XCTAssertEqual(receipt.record.status, "held", file: file, line: line)
        XCTAssertEqual(receipt.record.reason, reason, file: file, line: line)
        XCTAssertEqual(receipt.record.analysis, "held", file: file, line: line)
        XCTAssertEqual(receipt.record.universal.status, "accepted", file: file, line: line)
        XCTAssertNotNil(receipt.record.manifestDigest, file: file, line: line)
        XCTAssertEqual(receipt.record.coordinates?.count, receipt.record.dimension, file: file, line: line)
        XCTAssertNil(receipt.record.methodDetail, file: file, line: line)
        XCTAssertNil(receipt.record.recurrence, file: file, line: line)
        XCTAssertNil(receipt.record.initialState, file: file, line: line)
        XCTAssertNil(receipt.record.pathTangent, file: file, line: line)
        XCTAssertNil(receipt.record.monodromy, file: file, line: line)
        XCTAssertNil(receipt.record.multipliers, file: file, line: line)
        let orbit = EvidenceOrbit(receipt: receipt)
        XCTAssertEqual(orbit.stages[2].state, .held(reason), file: file, line: line)
    }

    private func run(_ binary: URL, source: String, root: URL) throws -> String {
        let scratch = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-live-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: scratch, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: scratch) }
        let stdoutURL = scratch.appendingPathComponent("stdout")
        let stderrURL = scratch.appendingPathComponent("stderr")
        FileManager.default.createFile(atPath: stdoutURL.path, contents: nil)
        FileManager.default.createFile(atPath: stderrURL.path, contents: nil)
        let stdout = try FileHandle(forWritingTo: stdoutURL)
        let stderr = try FileHandle(forWritingTo: stderrURL)
        defer {
            try? stdout.close()
            try? stderr.close()
        }

        let process = Process()
        process.executableURL = binary
        process.arguments = ["stability", source]
        process.currentDirectoryURL = root
        process.standardOutput = stdout
        process.standardError = stderr
        try process.run()
        process.waitUntilExit()
        try stdout.synchronize()
        try stderr.synchronize()
        let output = try String(contentsOf: stdoutURL, encoding: .utf8)
        let diagnostics = try String(contentsOf: stderrURL, encoding: .utf8)
        XCTAssertEqual(process.terminationStatus, 0, diagnostics)
        return output
    }
}

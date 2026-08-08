import XCTest
@testable import CDCStudio

final class ToolchainServiceTests: XCTestCase {
    private func makeRuntime(scriptBody: String) throws -> (root: URL, binary: URL) {
        let root = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-runtime-\(UUID().uuidString)")
        let build = root.appendingPathComponent("build")
        try FileManager.default.createDirectory(at: build, withIntermediateDirectories: true)
        let binary = build.appendingPathComponent("cdc")
        try scriptBody.write(to: binary, atomically: true, encoding: .utf8)
        try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: binary.path)
        return (root, binary)
    }

    private func makeNativeRuntime(cSource: String) throws -> (root: URL, binary: URL) {
        let root = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-native-runtime-\(UUID().uuidString)")
        let build = root.appendingPathComponent("build")
        try FileManager.default.createDirectory(at: build, withIntermediateDirectories: true)
        let source = root.appendingPathComponent("runtime.c")
        let binary = build.appendingPathComponent("cdc")
        try cSource.write(to: source, atomically: true, encoding: .utf8)
        let diagnostics = Pipe()
        let compiler = Process()
        compiler.executableURL = URL(fileURLWithPath: "/usr/bin/clang")
        compiler.arguments = [source.path, "-o", binary.path]
        compiler.standardError = diagnostics
        try compiler.run()
        compiler.waitUntilExit()
        guard compiler.terminationStatus == 0 else {
            let detail = String(decoding: diagnostics.fileHandleForReading.readDataToEndOfFile(), as: UTF8.self)
            throw NSError(domain: "ToolchainServiceTests", code: Int(compiler.terminationStatus), userInfo: [
                NSLocalizedDescriptionKey: detail
            ])
        }
        return (root, binary)
    }

    private func makeFloodingRuntime(bytesPerStream: Int) throws -> (root: URL, binary: URL) {
        let chunks = max(1, bytesPerStream / 1024)
        return try makeRuntime(scriptBody: """
        #!/bin/sh
        i=0
        line=$(printf 'x%.0s' $(seq 1 1023))
        while [ "$i" -lt \(chunks) ]; do
          echo "$line"
          echo "$line" >&2
          i=$((i + 1))
        done
        exit 0
        """)
    }

    private func execute(
        _ runtime: (root: URL, binary: URL),
        timeout: TimeInterval,
        streamByteLimit: Int = ToolchainProcessRunner.defaultStreamByteLimit
    ) async -> ToolchainProcessRunner.Outcome {
        let runner = ToolchainProcessRunner(streamByteLimit: streamByteLimit)
        let ticket = runner.reserve()
        return await runner.execute(
            binary: runtime.binary,
            arguments: [],
            workingDirectory: runtime.root,
            timeout: timeout,
            ticket: ticket
        )
    }

    @MainActor
    func testFloodingChildOnBothStreamsTerminatesAndCapturesBoth() async throws {
        let bytesPerStream = 512 * 1024
        let runtime = try makeFloodingRuntime(bytesPerStream: bytesPerStream)
        defer { try? FileManager.default.removeItem(at: runtime.root) }
        let outcome = await execute(runtime, timeout: 60)

        XCTAssertEqual(outcome.exitCode, 0)
        XCTAssertGreaterThan(outcome.stdout.utf8.count, bytesPerStream / 2, "stdout was truncated")
        XCTAssertGreaterThan(outcome.stderr.utf8.count, bytesPerStream / 2, "stderr was truncated")
    }

    @MainActor
    func testWedgedChildIsTerminatedByTimeout() async throws {
        let runtime = try makeRuntime(scriptBody: "#!/bin/sh\nexec sleep 600\n")
        defer { try? FileManager.default.removeItem(at: runtime.root) }
        let started = Date()
        let outcome = await execute(runtime, timeout: 2)

        XCTAssertLessThan(Date().timeIntervalSince(started), 30, "timeout did not fire")
        XCTAssertEqual(outcome.exitCode, -2)
        XCTAssertTrue(outcome.timedOut)
        XCTAssertTrue(outcome.stderr.contains("timed out"))
    }

    func testChildClosingBothPipesCannotBypassProcessTimeout() async throws {
        let runtime = try makeRuntime(scriptBody: "#!/bin/sh\nexec 1>&-\nexec 2>&-\nexec sleep 600\n")
        defer { try? FileManager.default.removeItem(at: runtime.root) }

        let started = Date()
        let outcome = await execute(runtime, timeout: 1)

        XCTAssertLessThan(Date().timeIntervalSince(started), 10, "EOF incorrectly ended process supervision")
        XCTAssertEqual(outcome.exitCode, -2)
        XCTAssertTrue(outcome.timedOut)
        XCTAssertEqual(outcome.stdoutCapture, .complete)
        XCTAssertEqual(outcome.stderrCapture, .complete)
    }

    func testPerStreamCaptureCapsAreTypedAndFailClosed() async throws {
        let limit = 64 * 1024
        let runtime = try makeFloodingRuntime(bytesPerStream: limit * 4)
        defer { try? FileManager.default.removeItem(at: runtime.root) }

        let outcome = await execute(runtime, timeout: 30, streamByteLimit: limit)

        XCTAssertEqual(outcome.exitCode, -6)
        XCTAssertEqual(outcome.stdout.utf8.count, limit)
        XCTAssertEqual(outcome.stdoutCapture, .truncated(limit: limit))
        XCTAssertEqual(outcome.stderrCapture, .truncated(limit: limit))
        XCTAssertTrue(outcome.stderr.contains("capture limit"))
    }

    func testCancellationReservedBeforeLaunchCannotBeResetOrLost() async throws {
        let runtime = try makeRuntime(scriptBody: "#!/bin/sh\ntouch launched\n")
        defer { try? FileManager.default.removeItem(at: runtime.root) }
        let runner = ToolchainProcessRunner()
        let ticket = runner.reserve()
        runner.cancel(ticket)

        let outcome = await runner.execute(
            binary: runtime.binary,
            arguments: [],
            workingDirectory: runtime.root,
            timeout: 5,
            ticket: ticket
        )

        XCTAssertEqual(outcome.exitCode, -3)
        XCTAssertTrue(outcome.cancelled)
        XCTAssertFalse(FileManager.default.fileExists(atPath: runtime.root.appendingPathComponent("launched").path))
    }

    @MainActor
    func testOperatorCancellationTerminatesActiveChild() async throws {
        let runtime = try makeRuntime(scriptBody: "#!/bin/sh\ntrap '' TERM\nexec sleep 600\n")
        defer { try? FileManager.default.removeItem(at: runtime.root) }
        let runner = ToolchainProcessRunner()
        let ticket = runner.reserve()

        let task = Task {
            await runner.execute(
                binary: runtime.binary,
                arguments: [],
                workingDirectory: runtime.root,
                timeout: 30,
                ticket: ticket
            )
        }
        try await Task.sleep(for: .milliseconds(100))
        runner.cancel(ticket)
        let outcome = await task.value

        XCTAssertTrue(outcome.cancelled)
        XCTAssertEqual(outcome.exitCode, -3)
        XCTAssertTrue(outcome.stderr.contains("cancelled by operator"))
    }

    @MainActor
    func testConcurrentInvocationFailsFastInsteadOfCorruptingRunningState() async throws {
        let runtime = try makeNativeRuntime(cSource: "#include <unistd.h>\nint main(void) { sleep(2); return 0; }\n")
        defer { try? FileManager.default.removeItem(at: runtime.root) }
        let service = ToolchainService(repositoryURL: runtime.root)

        let first = Task { await service.run([], timeout: 10) }
        try await Task.sleep(for: .milliseconds(100))
        let secondResult = await service.run([], timeout: 10)
        let second = try XCTUnwrap(secondResult)
        service.cancelCurrentRun()
        _ = await first.value

        XCTAssertEqual(second.exitCode, -4)
        XCTAssertTrue(second.stderr.contains("already active"))
    }

    func testDyadicRenderingIsSixBitsZeroPadded() {
        for (index, expected) in [(0, "000000"), (1, "000001"), (43, "101011"), (63, "111111")] {
            let bits = String(index, radix: 2)
            let padded = String(repeating: "0", count: max(0, 6 - bits.count)) + bits
            XCTAssertEqual(padded, expected)
        }
    }
}

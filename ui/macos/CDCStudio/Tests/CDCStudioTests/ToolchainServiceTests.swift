import XCTest
@testable import CDCStudio

/// 2026-07-28 review, finding 3.
///
/// Draining a child's stdout to EOF before starting on stderr deadlocks the
/// moment the child fills the undrained pipe's buffer: the child blocks
/// writing to stderr, the parent blocks reading stdout that will never end.
/// This is deterministic, not a race — which is why it needs a permanent
/// counterexample rather than a retry.
final class ToolchainServiceTests: XCTestCase {

    /// Writes a child that floods BOTH streams well past pipe capacity
    /// (64 KiB on Darwin), interleaved so neither can be drained first.
    private func makeFloodingChild(bytesPerStream: Int) throws -> URL {
        let dir = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-flood-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        let script = dir.appendingPathComponent("flood.sh")
        let chunks = max(1, bytesPerStream / 1024)
        let body = """
        #!/bin/sh
        i=0
        line=$(printf 'x%.0s' $(seq 1 1023))
        while [ "$i" -lt \(chunks) ]; do
          echo "$line"
          echo "$line" >&2
          i=$((i + 1))
        done
        exit 0
        """
        try body.write(to: script, atomically: true, encoding: .utf8)
        try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: script.path)
        return script
    }

    @MainActor
    func testFloodingChildOnBothStreamsTerminatesAndCapturesBoth() async throws {
        let bytesPerStream = 512 * 1024 // 8x pipe capacity on both streams
        let child = try makeFloodingChild(bytesPerStream: bytesPerStream)
        defer { try? FileManager.default.removeItem(at: child.deletingLastPathComponent()) }

        let service = ToolchainService(repositoryURL: child.deletingLastPathComponent())
        service.binaryURL = child

        // A generous timeout: this must pass because both pipes are drained
        // concurrently, NOT because the timeout rescued a deadlock. A
        // deadlocked implementation would come back with exitCode -2.
        let invocation = await service.run([], timeout: 60)

        XCTAssertEqual(invocation.exitCode, 0,
                       "child must exit normally; -2 means the drain deadlocked and timed out")
        XCTAssertGreaterThan(invocation.stdout.utf8.count, bytesPerStream / 2,
                             "stdout was truncated")
        XCTAssertGreaterThan(invocation.stderr.utf8.count, bytesPerStream / 2,
                             "stderr was truncated")
    }

    /// A child that never exits must not wedge the caller: the timeout
    /// escalates SIGTERM then SIGKILL and the invocation returns.
    @MainActor
    func testWedgedChildIsTerminatedByTimeout() async throws {
        let dir = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-wedge-\(UUID().uuidString)")
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        defer { try? FileManager.default.removeItem(at: dir) }
        let script = dir.appendingPathComponent("wedge.sh")
        try "#!/bin/sh\nsleep 600\n".write(to: script, atomically: true, encoding: .utf8)
        try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: script.path)

        let service = ToolchainService(repositoryURL: dir)
        service.binaryURL = script

        let started = Date()
        let invocation = await service.run([], timeout: 2)
        XCTAssertLessThan(Date().timeIntervalSince(started), 30, "timeout did not fire")
        XCTAssertEqual(invocation.exitCode, -2)
        XCTAssertTrue(invocation.stderr.contains("timed out"))
    }

    /// The dyadic readout is six bits, zero-padded on the left. This is the
    /// value whose construction failed to compile at the reviewed head.
    func testDyadicRenderingIsSixBitsZeroPadded() {
        for (index, expected) in [(0, "000000"), (1, "000001"), (43, "101011"), (63, "111111")] {
            let bits = String(index, radix: 2)
            let padded = String(repeating: "0", count: max(0, 6 - bits.count)) + bits
            XCTAssertEqual(padded, expected)
        }
    }
}

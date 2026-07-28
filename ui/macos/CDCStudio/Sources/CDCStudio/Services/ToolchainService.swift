import Foundation

/// Runs the real `cdc` binary. The app is a surface over the shipped toolchain,
/// never a reimplementation of it: every number the UI shows was produced by
/// the same executable the verification gate runs.
@MainActor
final class ToolchainService: ObservableObject {

    struct Invocation: Identifiable, Equatable {
        let id = UUID()
        let command: String
        let stdout: String
        let stderr: String
        let exitCode: Int32
        var succeeded: Bool { exitCode == 0 }
    }

    @Published private(set) var history: [Invocation] = []
    @Published private(set) var isRunning = false
    @Published var repositoryURL: URL
    @Published var binaryURL: URL

    init(repositoryURL: URL? = nil) {
        let repo = repositoryURL ?? FileManager.default.homeDirectoryForCurrentUser
        self.repositoryURL = repo
        self.binaryURL = repo.appendingPathComponent("build/cdc")
    }

    var binaryAvailable: Bool {
        FileManager.default.isExecutableFile(atPath: binaryURL.path)
    }

    /// Runs `cdc <arguments>` and returns the typed invocation record.
    /// Failure is a value, not an exception: a nonzero exit is ordinary
    /// information in this product, and the caller decides how to read it.
    @discardableResult
    func run(_ arguments: [String], timeout: TimeInterval = 120) async -> Invocation {
        isRunning = true
        defer { isRunning = false }

        let outcome = await Self.execute(
            binary: binaryURL,
            arguments: arguments,
            workingDirectory: repositoryURL,
            timeout: timeout
        )
        let invocation = Invocation(
            command: "cdc " + arguments.joined(separator: " "),
            stdout: outcome.stdout,
            stderr: outcome.stderr,
            exitCode: outcome.exitCode
        )

        history.insert(invocation, at: 0)
        if history.count > 50 { history.removeLast(history.count - 50) }
        return invocation
    }

    struct Outcome: Sendable {
        let stdout: String
        let stderr: String
        let exitCode: Int32
    }

    /// Launches the child off the main actor and drains stdout and stderr
    /// CONCURRENTLY.
    ///
    /// 2026-07-28 review, finding 3: reading one stream to EOF before
    /// starting the other deadlocks deterministically. A child that fills
    /// the undrained pipe's buffer (64 KiB on Darwin) blocks on write; the
    /// parent is blocked reading the other stream, which the child can no
    /// longer reach the end of. Neither side can proceed. Both handles are
    /// therefore drained on their own queues, and a timeout escalates
    /// SIGTERM then SIGKILL so a wedged child can never wedge the UI.
    nonisolated private static func execute(
        binary: URL,
        arguments: [String],
        workingDirectory: URL,
        timeout: TimeInterval
    ) async -> Outcome {
        await withCheckedContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async {
                continuation.resume(returning: runBlocking(
                    binary: binary,
                    arguments: arguments,
                    workingDirectory: workingDirectory,
                    timeout: timeout
                ))
            }
        }
    }

    nonisolated private static func runBlocking(
        binary: URL,
        arguments: [String],
        workingDirectory: URL,
        timeout: TimeInterval
    ) -> Outcome {
        let process = Process()
        process.executableURL = binary
        process.arguments = arguments
        process.currentDirectoryURL = workingDirectory

        let outPipe = Pipe()
        let errPipe = Pipe()
        process.standardOutput = outPipe
        process.standardError = errPipe

        do {
            try process.run()
        } catch {
            return Outcome(
                stdout: "",
                stderr: "could not launch \(binary.path): \(error.localizedDescription)",
                exitCode: -1
            )
        }

        let lock = NSLock()
        var outData = Data()
        var errData = Data()
        let group = DispatchGroup()

        func drain(_ handle: FileHandle, into sink: @escaping (Data) -> Void) {
            group.enter()
            DispatchQueue.global(qos: .userInitiated).async {
                let data = handle.readDataToEndOfFile()
                lock.lock()
                sink(data)
                lock.unlock()
                group.leave()
            }
        }
        drain(outPipe.fileHandleForReading) { outData = $0 }
        drain(errPipe.fileHandleForReading) { errData = $0 }

        var timedOut = false
        if group.wait(timeout: .now() + timeout) == .timedOut {
            timedOut = true
            process.terminate()
            if group.wait(timeout: .now() + 5) == .timedOut {
                kill(process.processIdentifier, SIGKILL)
                group.wait()
            }
        }
        process.waitUntilExit()

        lock.lock()
        let out = String(decoding: outData, as: UTF8.self)
        var err = String(decoding: errData, as: UTF8.self)
        lock.unlock()
        if timedOut {
            err += "\ncdc studio: timed out after \(Int(timeout))s; child terminated\n"
        }
        return Outcome(
            stdout: out,
            stderr: err,
            exitCode: timedOut ? -2 : process.terminationStatus
        )
    }

    func sources() -> [URL] {
        let contents = (try? FileManager.default.contentsOfDirectory(
            at: repositoryURL,
            includingPropertiesForKeys: nil
        )) ?? []
        return contents
            .filter { $0.pathExtension == "cdc" }
            .sorted { $0.lastPathComponent < $1.lastPathComponent }
    }

    func replay() -> ReplayRecord? {
        try? ReplayRecord.load(from: repositoryURL.appendingPathComponent("demo/replay.json"))
    }
}

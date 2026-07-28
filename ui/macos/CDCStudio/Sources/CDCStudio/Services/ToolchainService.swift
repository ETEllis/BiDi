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
    func run(_ arguments: [String]) async -> Invocation {
        isRunning = true
        defer { isRunning = false }

        let process = Process()
        process.executableURL = binaryURL
        process.arguments = arguments
        process.currentDirectoryURL = repositoryURL

        let outPipe = Pipe()
        let errPipe = Pipe()
        process.standardOutput = outPipe
        process.standardError = errPipe

        var invocation: Invocation
        do {
            try process.run()
            let outData = outPipe.fileHandleForReading.readDataToEndOfFile()
            let errData = errPipe.fileHandleForReading.readDataToEndOfFile()
            process.waitUntilExit()
            invocation = Invocation(
                command: "cdc " + arguments.joined(separator: " "),
                stdout: String(decoding: outData, as: UTF8.self),
                stderr: String(decoding: errData, as: UTF8.self),
                exitCode: process.terminationStatus
            )
        } catch {
            invocation = Invocation(
                command: "cdc " + arguments.joined(separator: " "),
                stdout: "",
                stderr: "could not launch \(binaryURL.path): \(error.localizedDescription)",
                exitCode: -1
            )
        }

        history.insert(invocation, at: 0)
        if history.count > 50 { history.removeLast(history.count - 50) }
        return invocation
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

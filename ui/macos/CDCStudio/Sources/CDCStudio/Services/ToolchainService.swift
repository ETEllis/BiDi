import Foundation

enum ToolchainStreamCapture: Equatable, Sendable {
    case complete
    case truncated(limit: Int)
    case failed(String)

    var isComplete: Bool {
        if case .complete = self { return true }
        return false
    }
}

@MainActor
final class ToolchainService: ObservableObject {
    typealias SnapshotLoader = @Sendable (URL) async throws -> RepositorySnapshot

    struct Invocation: Identifiable, Equatable, Sendable {
        let id: UUID
        let arguments: [String]
        let command: String
        let stdout: String
        let stderr: String
        let exitCode: Int32
        let startedAt: Date
        let finishedAt: Date
        let timedOut: Bool
        let cancelled: Bool
        let stdoutCapture: ToolchainStreamCapture
        let stderrCapture: ToolchainStreamCapture
        let contextGeneration: ContextGeneration

        init(
            id: UUID,
            arguments: [String],
            command: String,
            stdout: String,
            stderr: String,
            exitCode: Int32,
            startedAt: Date,
            finishedAt: Date,
            timedOut: Bool,
            cancelled: Bool,
            stdoutCapture: ToolchainStreamCapture = .complete,
            stderrCapture: ToolchainStreamCapture = .complete,
            contextGeneration: ContextGeneration
        ) {
            self.id = id
            self.arguments = arguments
            self.command = command
            self.stdout = stdout
            self.stderr = stderr
            self.exitCode = exitCode
            self.startedAt = startedAt
            self.finishedAt = finishedAt
            self.timedOut = timedOut
            self.cancelled = cancelled
            self.stdoutCapture = stdoutCapture
            self.stderrCapture = stderrCapture
            self.contextGeneration = contextGeneration
        }

        var succeeded: Bool {
            exitCode == 0 && !timedOut && !cancelled &&
                stdoutCapture.isComplete && stderrCapture.isComplete
        }
        var elapsed: TimeInterval { finishedAt.timeIntervalSince(startedAt) }
    }

    enum StabilityState: Equatable {
        case idle
        case disabled(String)
        case running(SourceDocument)
        case held(source: SourceDocument, receipts: [CanonicalU2Receipt], invocation: Invocation)
        case analyzed(source: SourceDocument, receipts: [CanonicalU2Receipt], invocation: Invocation)
        case malformed(source: SourceDocument, failure: U2ReceiptFailure, invocation: Invocation)
        case failed(source: SourceDocument, invocation: Invocation)
    }

    @Published private(set) var history: [Invocation] = []
    @Published private(set) var isRunning = false
    @Published private(set) var repositoryState: RepositoryState = .locating
    @Published private(set) var runtimeState: RuntimeState = .unknown
    @Published private(set) var selectedSource: SourceDocument?
    @Published private(set) var stabilityState: StabilityState = .idle
    @Published private(set) var contextGeneration: ContextGeneration = .initial
    @Published var selectedOrbitID: String?
    @Published var isRepositoryPickerPresented = false

    private(set) var repositoryURL: URL?
    // Internal setter is intentional: production configuration derives this
    // from the selected repository, while @testable integration fixtures can
    // inject an isolated executable without weakening the public UI boundary.
    var binaryURL: URL?

    private let explicitRepositoryURL: URL?
    private let environment: [String: String]
    private let bookmarks: RepositoryBookmarkStore
    private let snapshotLoader: SnapshotLoader
    private let processRunner = ToolchainProcessRunner()
    private var securityScopedURL: URL?
    private var bootstrapped = false
    private var operationSequence: UInt64 = 0
    private var activeOperation: ActiveOperation?

    private struct ActiveOperation {
        enum Kind { case probe, command }
        let id: UInt64
        let kind: Kind
        let ticket: ToolchainProcessRunner.Ticket
    }

    private static let historyLimit = 12

    init(
        repositoryURL: URL? = nil,
        environment: [String: String] = ProcessInfo.processInfo.environment,
        bookmarkStore: RepositoryBookmarkStore = RepositoryBookmarkStore(),
        snapshotLoader: SnapshotLoader? = nil
    ) {
        self.explicitRepositoryURL = repositoryURL
        self.environment = environment
        self.bookmarks = bookmarkStore
        self.snapshotLoader = snapshotLoader ?? { url in
            try await Task.detached(priority: .userInitiated) {
                try RepositoryDiscovery.snapshot(at: url)
            }.value
        }
        self.repositoryURL = repositoryURL
        self.binaryURL = repositoryURL?.appendingPathComponent("build/cdc")
    }

    deinit {
        securityScopedURL?.stopAccessingSecurityScopedResource()
    }

    var snapshot: RepositorySnapshot? { repositoryState.snapshot }
    var sources: [SourceDocument] { snapshot?.sources ?? [] }
    var projectSources: [SourceDocument] { snapshot?.projectSources ?? [] }
    var fixtureSources: [SourceDocument] { snapshot?.fixtures ?? [] }

    var binaryAvailable: Bool {
        guard let binaryURL, let repositoryURL else { return false }
        return (try? RuntimeExecutableValidator.inspect(
            binary: binaryURL,
            repository: repositoryURL
        )) != nil
    }

    func bootstrap() async {
        guard !bootstrapped else { return }
        bootstrapped = true
        repositoryState = .locating

        if let explicitRepositoryURL {
            await configureRepository(explicitRepositoryURL, persist: false)
            return
        }
        if let path = environment["CDC_REPOSITORY"], !path.isEmpty {
            await configureRepository(URL(fileURLWithPath: path), persist: false)
            return
        }
        if let restored = bookmarks.restore() {
            await configureRepository(restored, persist: false)
            return
        }

        let working = URL(fileURLWithPath: FileManager.default.currentDirectoryPath)
        if let discovered = RepositoryDiscovery.findAncestor(startingAt: working) {
            await configureRepository(discovered, persist: false)
            return
        }
        repositoryState = .unconfigured
        runtimeState = .unknown
        stabilityState = .disabled("Choose a CDC repository first.")
    }

    func chooseRepository(_ url: URL) async {
        await configureRepository(url, persist: true)
    }

    func refreshRepository() async {
        guard let repositoryURL else {
            invalidateRepositoryContext()
            repositoryState = .unconfigured
            runtimeState = .unknown
            selectedSource = nil
            stabilityState = .disabled("Choose a CDC repository first.")
            return
        }
        await configureRepository(repositoryURL, persist: false)
    }

    func forgetRepository() {
        invalidateRepositoryContext()
        securityScopedURL?.stopAccessingSecurityScopedResource()
        securityScopedURL = nil
        bookmarks.clear()
        repositoryURL = nil
        binaryURL = nil
        selectedSource = nil
        selectedOrbitID = nil
        repositoryState = .unconfigured
        runtimeState = .unknown
        stabilityState = .disabled("Choose a CDC repository first.")
    }

    func selectSource(_ source: SourceDocument) {
        guard sources.contains(source) else { return }
        guard selectedSource != source else { return }
        let mustReprobe = runtimeState == .probing
        invalidateSourceContext()
        selectedSource = source
        selectedOrbitID = nil
        stabilityState = runtimeState.isReady
            ? .idle
            : .disabled(runtimeUnavailableReason)
        if mustReprobe {
            let generation = contextGeneration
            Task { await probeRuntime(generation: generation) }
        }
    }

    func analyzeSelectedSource() async {
        guard let source = selectedSource else {
            stabilityState = .disabled("Select a source before analysis.")
            return
        }
        guard runtimeState.isReady else {
            stabilityState = .disabled(runtimeUnavailableReason)
            return
        }

        let generation = contextGeneration
        stabilityState = .running(source)
        guard let invocation = await run(["stability", source.relativePath]),
              contextGeneration == generation else { return }

        guard invocation.succeeded else {
            stabilityState = .failed(source: source, invocation: invocation)
            return
        }
        do {
            let receipts = try U2ReceiptParser.parse(invocation.stdout)
            selectedOrbitID = receipts.first?.record.orbit
            if receipts.contains(where: { $0.record.accepted }) {
                stabilityState = .analyzed(source: source, receipts: receipts, invocation: invocation)
            } else {
                stabilityState = .held(source: source, receipts: receipts, invocation: invocation)
            }
        } catch let failure as U2ReceiptFailure {
            stabilityState = .malformed(source: source, failure: failure, invocation: invocation)
        } catch {
            let failure = U2ReceiptFailure(
                kind: .invalidShape("unexpected parser failure"),
                detail: error.localizedDescription
            )
            stabilityState = .malformed(source: source, failure: failure, invocation: invocation)
        }
    }

    func cancelCurrentRun() {
        guard let activeOperation else { return }
        processRunner.cancel(activeOperation.ticket)
    }

    @discardableResult
    func run(_ arguments: [String], timeout: TimeInterval = 120) async -> Invocation? {
        let generation = contextGeneration
        if arguments.first == "persist" {
            guard arguments.count == 2,
                  let selectedSource,
                  selectedSource.authorizesPersistenceEffects,
                  arguments[1] == selectedSource.relativePath else {
                let now = Date()
                return Invocation(
                    id: UUID(), arguments: arguments,
                    command: Self.commandString(arguments), stdout: "",
                    stderr: "cdc studio: persistence effects require the exact repository source framework_persistence.cdc\n",
                    exitCode: -5, startedAt: now, finishedAt: now,
                    timedOut: false, cancelled: false,
                    contextGeneration: generation
                )
            }
        }
        guard !isRunning else {
            let now = Date()
            return Invocation(
                id: UUID(), arguments: arguments,
                command: Self.commandString(arguments), stdout: "",
                stderr: "cdc studio: another toolchain invocation is already active\n",
                exitCode: -4, startedAt: now, finishedAt: now,
                timedOut: false, cancelled: false,
                contextGeneration: generation
            )
        }
        guard let binaryURL, let repositoryURL else {
            let now = Date()
            return Invocation(
                id: UUID(), arguments: arguments,
                command: Self.commandString(arguments), stdout: "",
                stderr: "cdc studio: no repository runtime is configured\n",
                exitCode: -1, startedAt: now, finishedAt: now,
                timedOut: false, cancelled: false,
                contextGeneration: generation
            )
        }
        let preparedRuntime: PreparedRuntimeExecutable
        do {
            preparedRuntime = try RuntimeExecutableValidator.prepare(
                binary: binaryURL,
                repository: repositoryURL
            )
        } catch {
            let now = Date()
            return Invocation(
                id: UUID(), arguments: arguments,
                command: Self.commandString(arguments), stdout: "",
                stderr: "cdc studio: ABI-compatible runtime preparation failed: \(error.localizedDescription)\n",
                exitCode: -7, startedAt: now, finishedAt: now,
                timedOut: false, cancelled: false,
                contextGeneration: generation
            )
        }
        defer { preparedRuntime.cleanup() }

        let operation = beginOperation(.command)
        let invocation = await perform(
            binary: preparedRuntime.executableURL,
            arguments: arguments,
            workingDirectory: repositoryURL,
            timeout: timeout,
            ticket: operation.ticket,
            contextGeneration: generation
        )
        finishOperation(operation.id)
        guard contextGeneration == generation else { return nil }
        history.insert(invocation, at: 0)
        if history.count > Self.historyLimit {
            history.removeLast(history.count - Self.historyLimit)
        }
        return invocation
    }

    func replay() -> ReplayRecord? {
        guard let snapshot,
              let replayURL = snapshot.artifact("demo/replay.json") else { return nil }
        return try? ReplayRecord.load(from: replayURL)
    }

    private func configureRepository(_ url: URL, persist: Bool) async {
        let previousPath = selectedSource?.relativePath
        let generation = invalidateRepositoryContext()
        let standardized = url.standardizedFileURL
        repositoryState = .loading(path: standardized.path)
        runtimeState = .unknown
        selectedSource = nil
        selectedOrbitID = nil
        stabilityState = .disabled("Repository validation is in progress.")

        securityScopedURL?.stopAccessingSecurityScopedResource()
        securityScopedURL = nil
        if standardized.startAccessingSecurityScopedResource() {
            securityScopedURL = standardized
        }

        do {
            let snapshot = try await snapshotLoader(standardized)
            guard contextGeneration == generation else { return }
            repositoryURL = snapshot.root
            binaryURL = snapshot.binaryURL
            repositoryState = snapshot.sources.isEmpty ? .empty(snapshot) : .ready(snapshot)

            selectedSource = snapshot.sources.first(where: { $0.relativePath == previousPath })
                ?? snapshot.projectSources.first(where: { $0.relativePath == "framework_loop.cdc" })
                ?? snapshot.projectSources.first
                ?? snapshot.fixtures.first

            if persist { try? bookmarks.save(snapshot.root) }
            await probeRuntime(generation: generation)
        } catch let scan as RepositoryScanError {
            guard contextGeneration == generation else { return }
            repositoryURL = standardized
            binaryURL = standardized.appendingPathComponent("build/cdc")
            selectedSource = nil
            repositoryState = .unavailable(scan.problem)
            runtimeState = .unknown
            stabilityState = .disabled(scan.problem.title)
        } catch {
            guard contextGeneration == generation else { return }
            repositoryURL = standardized
            selectedSource = nil
            repositoryState = .unavailable(.scanFailed(path: standardized.path, reason: error.localizedDescription))
            runtimeState = .unknown
            stabilityState = .disabled("Repository validation failed.")
        }
    }

    private func probeRuntime(generation: ContextGeneration) async {
        guard contextGeneration == generation else { return }
        guard let binaryURL, let repositoryURL else {
            runtimeState = .unknown
            return
        }
        guard FileManager.default.fileExists(atPath: binaryURL.path) else {
            runtimeState = .missing(path: binaryURL.path)
            stabilityState = .disabled("Build the CDC runtime before analysis.")
            return
        }
        runtimeState = .probing
        guard await waitUntilIdle(generation: generation),
              contextGeneration == generation else { return }

        let preparedRuntime: PreparedRuntimeExecutable
        do {
            preparedRuntime = try RuntimeExecutableValidator.prepare(
                binary: binaryURL,
                repository: repositoryURL
            )
        } catch let validation as RuntimeExecutableValidationError {
            if case let .notExecutable(path) = validation {
                runtimeState = .permissionDenied(path: path)
                stabilityState = .disabled("The CDC runtime is not executable.")
            } else {
                runtimeState = .failed(message: validation.localizedDescription)
                stabilityState = .disabled("The CDC runtime is not an ABI-compatible native executable.")
            }
            return
        } catch {
            runtimeState = .failed(message: error.localizedDescription)
            stabilityState = .disabled("The CDC runtime is not an ABI-compatible native executable.")
            return
        }
        defer { preparedRuntime.cleanup() }

        let operation = beginOperation(.probe)
        let invocation = await perform(
            binary: preparedRuntime.executableURL,
            arguments: ["version"],
            workingDirectory: repositoryURL,
            timeout: 10,
            ticket: operation.ticket,
            contextGeneration: generation
        )
        finishOperation(operation.id)
        guard contextGeneration == generation else { return }

        guard invocation.succeeded else {
            runtimeState = .failed(message: invocation.stderr.isEmpty ? "Runtime probe failed." : invocation.stderr)
            stabilityState = .disabled("The CDC runtime probe failed.")
            return
        }
        guard let identity = RuntimeIdentity.parse(invocation.stdout) else {
            runtimeState = .incompatible(raw: invocation.stdout)
            stabilityState = .disabled("The runtime did not report a supported identity.")
            return
        }
        guard identity.isSupported else {
            runtimeState = .incompatible(raw: identity.raw)
            stabilityState = .disabled("CDC Studio requires ABI 1.5 and grammar 1.")
            return
        }
        runtimeState = .ready(identity)
        stabilityState = selectedSource == nil ? .disabled("Select a source before analysis.") : .idle
    }

    private func perform(
        binary: URL,
        arguments: [String],
        workingDirectory: URL,
        timeout: TimeInterval,
        ticket: ToolchainProcessRunner.Ticket,
        contextGeneration: ContextGeneration
    ) async -> Invocation {
        let started = Date()
        let outcome = await processRunner.execute(
            binary: binary,
            arguments: arguments,
            workingDirectory: workingDirectory,
            timeout: timeout,
            ticket: ticket
        )
        return Invocation(
            id: UUID(), arguments: arguments,
            command: Self.commandString(arguments),
            stdout: outcome.stdout, stderr: outcome.stderr,
            exitCode: outcome.exitCode, startedAt: started,
            finishedAt: Date(), timedOut: outcome.timedOut,
            cancelled: outcome.cancelled,
            stdoutCapture: outcome.stdoutCapture,
            stderrCapture: outcome.stderrCapture,
            contextGeneration: contextGeneration
        )
    }

    @discardableResult
    private func invalidateRepositoryContext() -> ContextGeneration {
        contextGeneration = ContextGeneration(
            repository: contextGeneration.repository &+ 1,
            source: contextGeneration.source &+ 1
        )
        selectedOrbitID = nil
        if let activeOperation {
            processRunner.cancel(activeOperation.ticket)
        }
        return contextGeneration
    }

    private func invalidateSourceContext() {
        contextGeneration = ContextGeneration(
            repository: contextGeneration.repository,
            source: contextGeneration.source &+ 1
        )
        selectedOrbitID = nil
        if let activeOperation {
            processRunner.cancel(activeOperation.ticket)
        }
    }

    private func beginOperation(_ kind: ActiveOperation.Kind) -> ActiveOperation {
        operationSequence &+= 1
        let operation = ActiveOperation(
            id: operationSequence,
            kind: kind,
            ticket: processRunner.reserve()
        )
        activeOperation = operation
        isRunning = true
        return operation
    }

    private func finishOperation(_ id: UInt64) {
        guard activeOperation?.id == id else { return }
        activeOperation = nil
        isRunning = false
    }

    private func waitUntilIdle(generation: ContextGeneration) async -> Bool {
        while isRunning {
            guard contextGeneration == generation else { return false }
            try? await Task.sleep(for: .milliseconds(20))
        }
        return contextGeneration == generation
    }

    private var runtimeUnavailableReason: String {
        switch runtimeState {
        case .ready: return ""
        case .probing: return "Runtime ABI compatibility is being checked."
        case let .missing(path): return "Runtime not built at \(path)."
        case let .permissionDenied(path): return "Runtime is not executable at \(path)."
        case let .incompatible(raw): return "Unsupported runtime identity: \(raw)"
        case let .failed(message): return message
        case .unknown: return "Choose and validate a CDC repository first."
        }
    }

    private static func commandString(_ arguments: [String]) -> String {
        "cdc " + arguments.map { argument in
            argument.contains(where: { $0.isWhitespace }) ? "\"\(argument)\"" : argument
        }.joined(separator: " ")
    }
}

final class ToolchainProcessRunner: @unchecked Sendable {
    struct Ticket: Hashable, Sendable {
        fileprivate let rawValue: UInt64
    }

    struct Outcome: Sendable {
        let stdout: String
        let stderr: String
        let exitCode: Int32
        let timedOut: Bool
        let cancelled: Bool
        let stdoutCapture: ToolchainStreamCapture
        let stderrCapture: ToolchainStreamCapture
    }

    static let defaultStreamByteLimit = 1_048_576

    private struct ActiveProcess {
        let ticket: Ticket
        let process: Process
    }

    private let stateLock = NSLock()
    private let streamByteLimit: Int
    private var ticketSequence: UInt64 = 0
    private var knownTickets: Set<Ticket> = []
    private var cancelledTickets: Set<Ticket> = []
    private var activeProcess: ActiveProcess?

    init(streamByteLimit: Int = ToolchainProcessRunner.defaultStreamByteLimit) {
        precondition(streamByteLimit > 0)
        self.streamByteLimit = streamByteLimit
    }

    func reserve() -> Ticket {
        stateLock.lock()
        ticketSequence &+= 1
        let ticket = Ticket(rawValue: ticketSequence)
        knownTickets.insert(ticket)
        stateLock.unlock()
        return ticket
    }

    func execute(
        binary: URL,
        arguments: [String],
        workingDirectory: URL,
        timeout: TimeInterval,
        ticket: Ticket
    ) async -> Outcome {
        await withCheckedContinuation { continuation in
            DispatchQueue.global(qos: .userInitiated).async {
                continuation.resume(returning: self.runBlocking(
                    binary: binary,
                    arguments: arguments,
                    workingDirectory: workingDirectory,
                    timeout: timeout,
                    ticket: ticket
                ))
            }
        }
    }

    func cancel(_ ticket: Ticket) {
        stateLock.lock()
        guard knownTickets.contains(ticket) else {
            stateLock.unlock()
            return
        }
        cancelledTickets.insert(ticket)
        let process = activeProcess?.ticket == ticket ? activeProcess?.process : nil
        stateLock.unlock()

        if process?.isRunning == true {
            process?.terminate()
        }
    }

    private func runBlocking(
        binary: URL,
        arguments: [String],
        workingDirectory: URL,
        timeout: TimeInterval,
        ticket: Ticket
    ) -> Outcome {
        let process = Process()
        process.executableURL = binary
        process.arguments = arguments
        process.currentDirectoryURL = workingDirectory

        let outPipe = Pipe()
        let errPipe = Pipe()
        process.standardOutput = outPipe
        process.standardError = errPipe
        let termination = DispatchSemaphore(value: 0)
        process.terminationHandler = { _ in termination.signal() }

        stateLock.lock()
        guard knownTickets.contains(ticket), !cancelledTickets.contains(ticket) else {
            knownTickets.remove(ticket)
            cancelledTickets.remove(ticket)
            stateLock.unlock()
            return cancelledBeforeLaunch()
        }
        activeProcess = ActiveProcess(ticket: ticket, process: process)
        do {
            try process.run()
        } catch {
            activeProcess = nil
            knownTickets.remove(ticket)
            cancelledTickets.remove(ticket)
            stateLock.unlock()
            return Outcome(
                stdout: "",
                stderr: "could not launch \(binary.path): \(error.localizedDescription)",
                exitCode: -1,
                timedOut: false,
                cancelled: false,
                stdoutCapture: .complete,
                stderrCapture: .complete
            )
        }
        stateLock.unlock()

        let drainGroup = DispatchGroup()
        let stdoutCollector = BoundedPipeCollector(
            handle: outPipe.fileHandleForReading,
            limit: streamByteLimit,
            group: drainGroup
        )
        let stderrCollector = BoundedPipeCollector(
            handle: errPipe.fileHandleForReading,
            limit: streamByteLimit,
            group: drainGroup
        )
        stdoutCollector.start()
        stderrCollector.start()

        var timedOut = false
        let deadline = ProcessInfo.processInfo.systemUptime + max(0, timeout)
        while true {
            let remaining = max(0, deadline - ProcessInfo.processInfo.systemUptime)
            let waitSlice = min(0.02, remaining)
            if termination.wait(timeout: .now() + waitSlice) == .success {
                break
            }
            if isCancelled(ticket) {
                terminateAndWait(process, termination: termination)
                break
            }
            if ProcessInfo.processInfo.systemUptime >= deadline {
                timedOut = true
                terminateAndWait(process, termination: termination)
                break
            }
        }
        process.waitUntilExit()

        if drainGroup.wait(timeout: .now() + 1) == .timedOut {
            stdoutCollector.stop(reason: "stdout pipe did not close after process termination")
            stderrCollector.stop(reason: "stderr pipe did not close after process termination")
            _ = drainGroup.wait(timeout: .now() + 1)
        }

        let stdoutSnapshot = stdoutCollector.snapshot()
        let stderrSnapshot = stderrCollector.snapshot()
        let cancelled = isCancelled(ticket)
        clear(ticket, process: process)

        let out = String(decoding: stdoutSnapshot.data, as: UTF8.self)
        var err = String(decoding: stderrSnapshot.data, as: UTF8.self)
        if timedOut { err += "\ncdc studio: timed out after \(Int(timeout))s; child terminated\n" }
        if cancelled { err += "\ncdc studio: invocation cancelled by operator\n" }
        err += captureDiagnostic("stdout", state: stdoutSnapshot.state)
        err += captureDiagnostic("stderr", state: stderrSnapshot.state)
        let captureComplete = stdoutSnapshot.state.isComplete && stderrSnapshot.state.isComplete
        return Outcome(
            stdout: out,
            stderr: err,
            exitCode: timedOut ? -2 : (cancelled ? -3 : (captureComplete ? process.terminationStatus : -6)),
            timedOut: timedOut,
            cancelled: cancelled,
            stdoutCapture: stdoutSnapshot.state,
            stderrCapture: stderrSnapshot.state
        )
    }

    private func cancelledBeforeLaunch() -> Outcome {
        Outcome(
            stdout: "",
            stderr: "cdc studio: invocation cancelled by operator before launch\n",
            exitCode: -3,
            timedOut: false,
            cancelled: true,
            stdoutCapture: .complete,
            stderrCapture: .complete
        )
    }

    private func isCancelled(_ ticket: Ticket) -> Bool {
        stateLock.lock()
        let cancelled = cancelledTickets.contains(ticket)
        stateLock.unlock()
        return cancelled
    }

    private func clear(_ ticket: Ticket, process: Process) {
        stateLock.lock()
        if activeProcess?.ticket == ticket && activeProcess?.process === process {
            activeProcess = nil
        }
        knownTickets.remove(ticket)
        cancelledTickets.remove(ticket)
        stateLock.unlock()
    }

    private func terminateAndWait(_ process: Process, termination: DispatchSemaphore) {
        if process.isRunning { process.terminate() }
        if termination.wait(timeout: .now() + 2) == .timedOut, process.isRunning {
            kill(process.processIdentifier, SIGKILL)
            _ = termination.wait(timeout: .now() + 2)
        }
    }

    private func captureDiagnostic(_ stream: String, state: ToolchainStreamCapture) -> String {
        switch state {
        case .complete:
            return ""
        case let .truncated(limit):
            return "\ncdc studio: \(stream) exceeded \(limit)-byte capture limit; invocation output is incomplete\n"
        case let .failed(reason):
            return "\ncdc studio: \(stream) capture failed: \(reason)\n"
        }
    }
}

private final class BoundedPipeCollector: @unchecked Sendable {
    struct Snapshot: Sendable {
        let data: Data
        let state: ToolchainStreamCapture
    }

    private let handle: FileHandle
    private let limit: Int
    private let group: DispatchGroup
    private let lock = NSLock()
    private var data = Data()
    private var truncated = false
    private var failure: String?
    private var finished = false

    init(handle: FileHandle, limit: Int, group: DispatchGroup) {
        self.handle = handle
        self.limit = limit
        self.group = group
    }

    func start() {
        group.enter()
        handle.readabilityHandler = { [weak self] readable in
            guard let self else { return }
            let chunk = readable.availableData
            if chunk.isEmpty {
                self.finish()
            } else {
                self.append(chunk)
            }
        }
    }

    func stop(reason: String) {
        finish(failure: reason)
    }

    func snapshot() -> Snapshot {
        lock.lock()
        let retained = data
        let state: ToolchainStreamCapture
        if let failure {
            state = .failed(failure)
        } else if truncated {
            state = .truncated(limit: limit)
        } else {
            state = .complete
        }
        lock.unlock()
        return Snapshot(data: retained, state: state)
    }

    private func append(_ chunk: Data) {
        lock.lock()
        let remaining = max(0, limit - data.count)
        if remaining > 0 {
            data.append(chunk.prefix(remaining))
        }
        if chunk.count > remaining {
            truncated = true
        }
        lock.unlock()
    }

    private func finish(failure: String? = nil) {
        lock.lock()
        guard !finished else {
            lock.unlock()
            return
        }
        finished = true
        if let failure { self.failure = failure }
        lock.unlock()

        handle.readabilityHandler = nil
        try? handle.close()
        group.leave()
    }
}

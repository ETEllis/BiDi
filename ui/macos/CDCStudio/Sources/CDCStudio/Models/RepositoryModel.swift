import Foundation
import Darwin

struct RuntimeIdentity: Equatable, Sendable {
    let abiMajor: Int
    let abiMinor: Int
    let grammar: Int
    let raw: String

    var displayName: String { "ABI \(abiMajor).\(abiMinor) · grammar \(grammar)" }
    var isSupported: Bool { abiMajor == 1 && abiMinor == 5 && grammar == 1 }

    static func parse(_ output: String) -> RuntimeIdentity? {
        let raw = output.trimmingCharacters(in: .whitespacesAndNewlines)
        let components = raw.split(separator: " ", omittingEmptySubsequences: false)
        guard components.count == 3,
              components[0] == "cdc",
              components[1].hasPrefix("abi="),
              components[2].hasPrefix("grammar="),
              !raw.contains(where: \.isNewline) else {
            return nil
        }
        let abi = components[1].dropFirst("abi=".count).split(
            separator: ".",
            omittingEmptySubsequences: false
        )
        guard abi.count == 2,
              let major = Int(abi[0]),
              let minor = Int(abi[1]),
              let grammar = Int(components[2].dropFirst("grammar=".count)) else { return nil }
        return RuntimeIdentity(
            abiMajor: major,
            abiMinor: minor,
            grammar: grammar,
            raw: raw
        )
    }
}

enum RuntimeExecutableValidationError: Error, Equatable, LocalizedError {
    case wrongLocation(expected: String, actual: String)
    case symbolicLink(String)
    case notRegularFile(String)
    case notExecutable(String)
    case unsupportedFormat(String)
    case invalidSize(String, Int64)
    case unreadable(String, String)
    case preparationFailed(String)

    var errorDescription: String? {
        switch self {
        case let .wrongLocation(expected, actual):
            return "Runtime must be the exact repository executable at \(expected), not \(actual)."
        case let .symbolicLink(path):
            return "Runtime symlinks are not admitted: \(path)."
        case let .notRegularFile(path):
            return "Runtime is not a regular file: \(path)."
        case let .notExecutable(path):
            return "Runtime is not executable: \(path)."
        case let .unsupportedFormat(path):
            return "Runtime is not a native Mach-O executable: \(path)."
        case let .invalidSize(path, size):
            return "Runtime size is outside the admitted compatibility bound at \(path): \(size) bytes."
        case let .unreadable(path, reason):
            return "Runtime could not be inspected at \(path): \(reason)"
        case let .preparationFailed(reason):
            return "A private executable image could not be prepared: \(reason)"
        }
    }
}

struct PreparedRuntimeExecutable: Sendable {
    let executableURL: URL
    private let directoryURL: URL

    fileprivate init(executableURL: URL, directoryURL: URL) {
        self.executableURL = executableURL
        self.directoryURL = directoryURL
    }

    func cleanup() {
        try? FileManager.default.removeItem(at: directoryURL)
    }
}

struct RuntimeExecutableValidator {
    private static let maximumExecutableSize: Int64 = 128 * 1024 * 1024
    private static let machOMagic: Set<Data> = [
        Data([0xcf, 0xfa, 0xed, 0xfe]),
        Data([0xfe, 0xed, 0xfa, 0xcf]),
        Data([0xca, 0xfe, 0xba, 0xbe]),
        Data([0xbe, 0xba, 0xfe, 0xca])
    ]

    static func inspect(binary: URL, repository: URL) throws -> URL {
        let opened = try openCandidate(binary: binary, repository: repository)
        defer { Darwin.close(opened.descriptor) }
        let prefix = try readExactly(
            descriptor: opened.descriptor,
            count: 4,
            path: opened.candidate.path
        )
        try validateMachO(prefix: prefix, path: opened.candidate.path)
        return opened.candidate
    }

    static func prepare(binary: URL, repository: URL) throws -> PreparedRuntimeExecutable {
        let opened = try openCandidate(binary: binary, repository: repository)
        defer { Darwin.close(opened.descriptor) }
        let bytes = try readExactly(
            descriptor: opened.descriptor,
            count: opened.size,
            path: opened.candidate.path
        )
        try validateMachO(prefix: Data(bytes.prefix(4)), path: opened.candidate.path)
        return try materialize(bytes)
    }

    private static func openCandidate(
        binary: URL,
        repository: URL
    ) throws -> (descriptor: Int32, candidate: URL, size: Int) {
        let repositoryPath = repository.standardizedFileURL
        let expected = repositoryPath.appendingPathComponent("build/cdc").standardizedFileURL
        let candidate = binary.standardizedFileURL
        guard candidate.path == expected.path else {
            throw RuntimeExecutableValidationError.wrongLocation(
                expected: expected.path,
                actual: candidate.path
            )
        }

        let repositoryDescriptor = Darwin.open(
            repositoryPath.path,
            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC
        )
        guard repositoryDescriptor >= 0 else {
            throw unreadable(candidate.path)
        }
        defer { Darwin.close(repositoryDescriptor) }

        let buildDescriptor = Darwin.openat(
            repositoryDescriptor,
            "build",
            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC
        )
        guard buildDescriptor >= 0 else {
            throw unreadable(candidate.path)
        }
        defer { Darwin.close(buildDescriptor) }

        let descriptor = Darwin.openat(
            buildDescriptor,
            "cdc",
            O_RDONLY | O_NOFOLLOW | O_CLOEXEC
        )
        guard descriptor >= 0 else {
            if errno == ELOOP {
                throw RuntimeExecutableValidationError.symbolicLink(candidate.path)
            }
            throw unreadable(candidate.path)
        }

        var metadata = stat()
        guard Darwin.fstat(descriptor, &metadata) == 0 else {
            let error = unreadable(candidate.path)
            Darwin.close(descriptor)
            throw error
        }
        guard (metadata.st_mode & S_IFMT) == S_IFREG else {
            Darwin.close(descriptor)
            throw RuntimeExecutableValidationError.notRegularFile(candidate.path)
        }
        guard (metadata.st_mode & 0o111) != 0 else {
            Darwin.close(descriptor)
            throw RuntimeExecutableValidationError.notExecutable(candidate.path)
        }
        guard metadata.st_size >= 4, metadata.st_size <= maximumExecutableSize else {
            Darwin.close(descriptor)
            throw RuntimeExecutableValidationError.invalidSize(candidate.path, metadata.st_size)
        }
        return (descriptor, candidate, Int(metadata.st_size))
    }

    private static func readExactly(descriptor: Int32, count: Int, path: String) throws -> Data {
        var bytes = [UInt8](repeating: 0, count: count)
        var offset = 0
        while offset < count {
            let amount = bytes.withUnsafeMutableBytes { buffer -> Int in
                guard let baseAddress = buffer.baseAddress else { return 0 }
                return Darwin.read(
                    descriptor,
                    baseAddress.advanced(by: offset),
                    count - offset
                )
            }
            if amount < 0 {
                if errno == EINTR { continue }
                throw unreadable(path)
            }
            guard amount > 0 else {
                throw RuntimeExecutableValidationError.unreadable(path, "file changed during capture")
            }
            offset += amount
        }
        return Data(bytes)
    }

    private static func validateMachO(prefix: Data, path: String) throws {
        guard machOMagic.contains(prefix) else {
            throw RuntimeExecutableValidationError.unsupportedFormat(path)
        }
    }

    private static func materialize(_ bytes: Data) throws -> PreparedRuntimeExecutable {
        let templatePath = URL(fileURLWithPath: NSTemporaryDirectory(), isDirectory: true)
            .appendingPathComponent("cdc-studio-runtime.XXXXXX", isDirectory: true)
            .path
        var template = Array(templatePath.utf8CString)
        var creationError: Int32 = 0
        let directoryPath: String? = template.withUnsafeMutableBufferPointer { buffer in
            guard let address = buffer.baseAddress, Darwin.mkdtemp(address) != nil else {
                creationError = errno
                return nil
            }
            return String(cString: address)
        }
        guard let directoryPath else {
            throw RuntimeExecutableValidationError.preparationFailed(errorText(creationError))
        }

        let directoryURL = URL(fileURLWithPath: directoryPath, isDirectory: true)
        let executableURL = directoryURL.appendingPathComponent("cdc", isDirectory: false)
        do {
            let descriptor = Darwin.open(
                executableURL.path,
                O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                mode_t(0o700)
            )
            guard descriptor >= 0 else {
                throw RuntimeExecutableValidationError.preparationFailed(errorText(errno))
            }
            defer { Darwin.close(descriptor) }

            try bytes.withUnsafeBytes { buffer in
                var offset = 0
                while offset < buffer.count {
                    guard let baseAddress = buffer.baseAddress else {
                        throw RuntimeExecutableValidationError.preparationFailed("captured executable is empty")
                    }
                    let amount = Darwin.write(
                        descriptor,
                        baseAddress.advanced(by: offset),
                        buffer.count - offset
                    )
                    if amount < 0 {
                        if errno == EINTR { continue }
                        throw RuntimeExecutableValidationError.preparationFailed(errorText(errno))
                    }
                    guard amount > 0 else {
                        throw RuntimeExecutableValidationError.preparationFailed("short executable write")
                    }
                    offset += amount
                }
            }
            guard Darwin.fchmod(descriptor, mode_t(0o700)) == 0,
                  Darwin.fsync(descriptor) == 0 else {
                throw RuntimeExecutableValidationError.preparationFailed(errorText(errno))
            }
            return PreparedRuntimeExecutable(
                executableURL: executableURL,
                directoryURL: directoryURL
            )
        } catch {
            try? FileManager.default.removeItem(at: directoryURL)
            throw error
        }
    }

    private static func unreadable(_ path: String) -> RuntimeExecutableValidationError {
        .unreadable(path, errorText(errno))
    }

    private static func errorText(_ code: Int32) -> String {
        String(cString: strerror(code))
    }
}

struct SourceDocument: Identifiable, Hashable, Sendable {
    enum Kind: String, Sendable {
        case project
        case fixture
    }

    let url: URL
    let relativePath: String
    let kind: Kind

    // A relative path is only unique inside one checkout. Repository swaps can
    // legitimately produce the same relative path, so view identity binds to
    // the canonical file location while async work is additionally guarded by
    // ContextGeneration.
    var id: String { url.standardizedFileURL.path }
    var displayName: String { url.deletingPathExtension().lastPathComponent }
    var authorizesPersistenceEffects: Bool {
        relativePath == "framework_persistence.cdc"
    }
    var directoryLabel: String {
        let directory = (relativePath as NSString).deletingLastPathComponent
        return directory == "." || directory.isEmpty ? "repository root" : directory
    }
}

/// A process-lifetime monotonic identity for every repository/source context.
/// Neither component is reset, including A -> B -> A transitions, so a late
/// response can never become current again merely because a path matches.
struct ContextGeneration: Equatable, Sendable {
    let repository: UInt64
    let source: UInt64

    static let initial = ContextGeneration(repository: 0, source: 0)
}

struct RepositorySnapshot: Equatable, Sendable {
    let root: URL
    let sources: [SourceDocument]

    var projectSources: [SourceDocument] { sources.filter { $0.kind == .project } }
    var fixtures: [SourceDocument] { sources.filter { $0.kind == .fixture } }
    var binaryURL: URL { root.appendingPathComponent("build/cdc") }

    func artifact(_ relativePath: String) -> URL? {
        let candidate = root.appendingPathComponent(relativePath).standardizedFileURL
        return FileManager.default.fileExists(atPath: candidate.path) ? candidate : nil
    }
}

enum RepositoryProblem: Equatable, Sendable {
    case invalid(path: String, reason: String)
    case permissionDenied(path: String)
    case offline(path: String)
    case scanFailed(path: String, reason: String)

    var title: String {
        switch self {
        case .invalid: return "Not a CDC repository"
        case .permissionDenied: return "Permission denied"
        case .offline: return "Repository unavailable"
        case .scanFailed: return "Could not read repository"
        }
    }

    var detail: String {
        switch self {
        case let .invalid(path, reason): return "\(path)\n\(reason)"
        case let .permissionDenied(path): return "CDC Studio cannot read \(path). Choose it again to reauthorize access."
        case let .offline(path): return "\(path) is no longer reachable. Reconnect its volume or choose another repository."
        case let .scanFailed(path, reason): return "\(path)\n\(reason)"
        }
    }
}

enum RepositoryState: Equatable, Sendable {
    case locating
    case unconfigured
    case loading(path: String)
    case ready(RepositorySnapshot)
    case empty(RepositorySnapshot)
    case unavailable(RepositoryProblem)

    var snapshot: RepositorySnapshot? {
        switch self {
        case let .ready(snapshot), let .empty(snapshot): return snapshot
        default: return nil
        }
    }
}

enum RuntimeState: Equatable, Sendable {
    case unknown
    case probing
    case missing(path: String)
    case permissionDenied(path: String)
    case incompatible(raw: String)
    case ready(RuntimeIdentity)
    case failed(message: String)

    var isReady: Bool {
        if case .ready = self { return true }
        return false
    }
}

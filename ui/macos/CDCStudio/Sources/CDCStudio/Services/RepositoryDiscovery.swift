import Foundation

struct RepositoryDiscovery {
    private static let requiredMarkers = [
        "kernel.cdc",
        "runtime/toolchain/main.c",
        "ui/macos/CDCStudio/Package.swift"
    ]

    private static let excludedDirectories: Set<String> = [
        ".git", ".build", ".ui-proof", "build", "DerivedData", "node_modules"
    ]

    static func findAncestor(startingAt start: URL) -> URL? {
        var candidate = start.standardizedFileURL
        var isDirectory: ObjCBool = false
        if FileManager.default.fileExists(atPath: candidate.path, isDirectory: &isDirectory),
           !isDirectory.boolValue {
            candidate.deleteLastPathComponent()
        }

        while candidate.path != "/" {
            if isRepository(candidate) { return candidate }
            candidate.deleteLastPathComponent()
        }
        return isRepository(candidate) ? candidate : nil
    }

    static func isRepository(_ url: URL) -> Bool {
        requiredMarkers.allSatisfy {
            FileManager.default.fileExists(atPath: url.appendingPathComponent($0).path)
        }
    }

    static func snapshot(at root: URL) throws -> RepositorySnapshot {
        let canonicalRoot = root.standardizedFileURL.resolvingSymlinksInPath()
        guard FileManager.default.fileExists(atPath: canonicalRoot.path) else {
            throw RepositoryScanError.offline(canonicalRoot.path)
        }
        guard FileManager.default.isReadableFile(atPath: canonicalRoot.path) else {
            throw RepositoryScanError.permissionDenied(canonicalRoot.path)
        }
        guard isRepository(canonicalRoot) else {
            throw RepositoryScanError.invalid(
                canonicalRoot.path,
                "Expected kernel.cdc, runtime/toolchain/main.c, and ui/macos/CDCStudio/Package.swift."
            )
        }

        let keys: [URLResourceKey] = [.isDirectoryKey, .isRegularFileKey, .isSymbolicLinkKey]
        var traversalFailure: (path: String, reason: String)?
        guard let enumerator = FileManager.default.enumerator(
            at: canonicalRoot,
            includingPropertiesForKeys: keys,
            options: [.skipsHiddenFiles],
            errorHandler: { url, error in
                traversalFailure = (url.path, error.localizedDescription)
                return false
            }
        ) else {
            throw RepositoryScanError.unreadable(canonicalRoot.path, "Could not enumerate repository contents.")
        }

        var documents: [SourceDocument] = []
        for case let url as URL in enumerator {
            let relative = relativePath(for: url, within: canonicalRoot)
            let components = relative.split(separator: "/").map(String.init)
            if let first = components.first, excludedDirectories.contains(first) {
                enumerator.skipDescendants()
                continue
            }

            let values: URLResourceValues
            do {
                values = try url.resourceValues(forKeys: Set(keys))
            } catch {
                throw RepositoryScanError.unreadable(url.path, error.localizedDescription)
            }
            if values.isDirectory == true {
                if excludedDirectories.contains(url.lastPathComponent) {
                    enumerator.skipDescendants()
                }
                continue
            }
            guard url.pathExtension.lowercased() == "cdc",
                  values.isRegularFile == true || values.isSymbolicLink == true else {
                continue
            }

            let resolved = url.standardizedFileURL.resolvingSymlinksInPath()
            guard contains(resolved, in: canonicalRoot) else { continue }
            let kind: SourceDocument.Kind = relative.hasPrefix("tests/fixtures/") ? .fixture : .project
            documents.append(SourceDocument(url: resolved, relativePath: relative, kind: kind))
        }
        if let traversalFailure {
            throw RepositoryScanError.unreadable(traversalFailure.path, traversalFailure.reason)
        }

        documents.sort {
            if $0.kind != $1.kind { return $0.kind == .project }
            return $0.relativePath.localizedStandardCompare($1.relativePath) == .orderedAscending
        }
        return RepositorySnapshot(root: canonicalRoot, sources: documents)
    }

    static func relativePath(for url: URL, within root: URL) -> String {
        let rootPath = root.standardizedFileURL.path
        let path = url.standardizedFileURL.path
        guard path != rootPath else { return "." }
        let prefix = rootPath.hasSuffix("/") ? rootPath : rootPath + "/"
        return path.hasPrefix(prefix) ? String(path.dropFirst(prefix.count)) : path
    }

    static func contains(_ url: URL, in root: URL) -> Bool {
        let rootPath = root.standardizedFileURL.resolvingSymlinksInPath().path
        let path = url.standardizedFileURL.resolvingSymlinksInPath().path
        return path == rootPath || path.hasPrefix(rootPath + "/")
    }
}

enum RepositoryScanError: Error, Equatable {
    case invalid(String, String)
    case permissionDenied(String)
    case offline(String)
    case unreadable(String, String)

    var problem: RepositoryProblem {
        switch self {
        case let .invalid(path, reason): return .invalid(path: path, reason: reason)
        case let .permissionDenied(path): return .permissionDenied(path: path)
        case let .offline(path): return .offline(path: path)
        case let .unreadable(path, reason): return .scanFailed(path: path, reason: reason)
        }
    }
}

final class RepositoryBookmarkStore {
    private let defaults: UserDefaults
    private let key = "CDCStudio.repositoryBookmark.v1"

    init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
    }

    func save(_ url: URL) throws {
        let data = try url.bookmarkData(
            options: [.withSecurityScope],
            includingResourceValuesForKeys: nil,
            relativeTo: nil
        )
        defaults.set(data, forKey: key)
    }

    func restore() -> URL? {
        guard let data = defaults.data(forKey: key) else { return nil }
        var stale = false
        guard let url = try? URL(
            resolvingBookmarkData: data,
            options: [.withSecurityScope],
            relativeTo: nil,
            bookmarkDataIsStale: &stale
        ) else {
            return nil
        }
        if stale { try? save(url) }
        return url
    }

    func clear() {
        defaults.removeObject(forKey: key)
    }
}

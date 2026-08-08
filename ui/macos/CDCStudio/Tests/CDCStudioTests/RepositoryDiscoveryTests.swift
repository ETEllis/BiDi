import XCTest
@testable import CDCStudio

final class RepositoryDiscoveryTests: XCTestCase {
    private func makeRepository() throws -> URL {
        let root = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("cdc-studio-repository-\(UUID().uuidString)")
        for directory in ["runtime/toolchain", "ui/macos/CDCStudio", "tests/fixtures/u2", "build"] {
            try FileManager.default.createDirectory(
                at: root.appendingPathComponent(directory),
                withIntermediateDirectories: true
            )
        }
        try "cell root".write(to: root.appendingPathComponent("kernel.cdc"), atomically: true, encoding: .utf8)
        try "int main(void){}".write(to: root.appendingPathComponent("runtime/toolchain/main.c"), atomically: true, encoding: .utf8)
        try "// swift-tools-version: 5.9".write(to: root.appendingPathComponent("ui/macos/CDCStudio/Package.swift"), atomically: true, encoding: .utf8)
        try "universal main".write(to: root.appendingPathComponent("framework_loop.cdc"), atomically: true, encoding: .utf8)
        try "spectrum case".write(to: root.appendingPathComponent("tests/fixtures/u2/case.cdc"), atomically: true, encoding: .utf8)
        return root
    }

    private func installRuntime(_ body: String, in root: URL) throws {
        let binary = root.appendingPathComponent("build/cdc")
        try body.write(to: binary, atomically: true, encoding: .utf8)
        try FileManager.default.setAttributes([.posixPermissions: 0o755], ofItemAtPath: binary.path)
    }

    private func installNativeRuntime(_ sourceText: String, in root: URL) throws {
        let source = root.appendingPathComponent("build/runtime-test.c")
        let binary = root.appendingPathComponent("build/cdc")
        try sourceText.write(to: source, atomically: true, encoding: .utf8)
        let diagnostics = Pipe()
        let compiler = Process()
        compiler.executableURL = URL(fileURLWithPath: "/usr/bin/clang")
        compiler.arguments = [source.path, "-o", binary.path]
        compiler.standardError = diagnostics
        try compiler.run()
        compiler.waitUntilExit()
        guard compiler.terminationStatus == 0 else {
            let detail = String(decoding: diagnostics.fileHandleForReading.readDataToEndOfFile(), as: UTF8.self)
            throw NSError(domain: "RepositoryDiscoveryTests", code: Int(compiler.terminationStatus), userInfo: [
                NSLocalizedDescriptionKey: detail
            ])
        }
    }

    func testAncestorDiscoveryAndRecursiveSourceClassification() throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }

        let nested = root.appendingPathComponent("ui/macos/CDCStudio")
        XCTAssertEqual(
            RepositoryDiscovery.findAncestor(startingAt: nested)?.standardizedFileURL.path,
            root.standardizedFileURL.path
        )

        let snapshot = try RepositoryDiscovery.snapshot(at: root)
        XCTAssertEqual(snapshot.projectSources.map(\.relativePath), ["framework_loop.cdc", "kernel.cdc"])
        XCTAssertEqual(snapshot.fixtures.map(\.relativePath), ["tests/fixtures/u2/case.cdc"])
    }

    func testGeneratedAndProofDirectoriesAreExcluded() throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        for directory in ["build", ".ui-proof", ".build"] {
            let url = root.appendingPathComponent(directory).appendingPathComponent("ignored.cdc")
            try FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
            try "ignored".write(to: url, atomically: true, encoding: .utf8)
        }

        let snapshot = try RepositoryDiscovery.snapshot(at: root)
        XCTAssertFalse(snapshot.sources.contains { $0.relativePath.contains("ignored") })
    }

    func testTraversalPermissionErrorFailsClosedInsteadOfReturningPartialSnapshot() throws {
        let root = try makeRepository()
        let blocked = root.appendingPathComponent("blocked")
        defer {
            try? FileManager.default.setAttributes([.posixPermissions: 0o700], ofItemAtPath: blocked.path)
            try? FileManager.default.removeItem(at: root)
        }
        try FileManager.default.createDirectory(at: blocked, withIntermediateDirectories: true)
        try "hidden source".write(
            to: blocked.appendingPathComponent("hidden.cdc"),
            atomically: true,
            encoding: .utf8
        )
        try FileManager.default.setAttributes([.posixPermissions: 0o000], ofItemAtPath: blocked.path)

        XCTAssertThrowsError(try RepositoryDiscovery.snapshot(at: root)) { error in
            guard case .unreadable = error as? RepositoryScanError else {
                return XCTFail("expected typed unreadable traversal failure, got \(error)")
            }
        }
    }

    func testSymlinkEscapingRepositoryIsRejected() throws {
        let root = try makeRepository()
        let outside = URL(fileURLWithPath: NSTemporaryDirectory())
            .appendingPathComponent("outside-\(UUID().uuidString).cdc")
        defer {
            try? FileManager.default.removeItem(at: root)
            try? FileManager.default.removeItem(at: outside)
        }
        try "outside".write(to: outside, atomically: true, encoding: .utf8)
        try FileManager.default.createSymbolicLink(
            at: root.appendingPathComponent("escaped.cdc"),
            withDestinationURL: outside
        )

        let snapshot = try RepositoryDiscovery.snapshot(at: root)
        XCTAssertFalse(snapshot.sources.contains { $0.relativePath == "escaped.cdc" })
    }

    @MainActor
    func testExplicitRepositorySelectsFrameworkLoopAndVerifiesIdentity() async throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        try installNativeRuntime(
            "#include <stdio.h>\nint main(void) { puts(\"cdc abi=1.5 grammar=1\"); return 0; }\n",
            in: root
        )

        let service = ToolchainService(repositoryURL: root, environment: [:])
        await service.bootstrap()

        XCTAssertEqual(service.selectedSource?.relativePath, "framework_loop.cdc")
        guard case let .ready(identity) = service.runtimeState else {
            return XCTFail("runtime identity was not admitted")
        }
        XCTAssertEqual(identity.abiMajor, 1)
        XCTAssertEqual(identity.abiMinor, 5)
        XCTAssertEqual(identity.grammar, 1)
    }

    @MainActor
    func testInvalidExplicitPathDoesNotFallBackToHome() async {
        let invalid = URL(fileURLWithPath: "/tmp/not-a-cdc-repository-\(UUID().uuidString)")
        let service = ToolchainService(repositoryURL: invalid, environment: [:])

        await service.bootstrap()

        guard case let .unavailable(problem) = service.repositoryState else {
            return XCTFail("invalid explicit path must remain an honest unavailable state")
        }
        XCTAssertTrue(problem.detail.contains(invalid.path))
        XCTAssertNotEqual(service.repositoryURL, FileManager.default.homeDirectoryForCurrentUser)
    }

    func testRuntimeIdentityParserRejectsStaleABI() {
        XCTAssertEqual(RuntimeIdentity.parse("cdc abi=1.5 grammar=1")?.isSupported, true)
        XCTAssertEqual(RuntimeIdentity.parse("cdc abi=1.2 grammar=1")?.isSupported, false)
        XCTAssertNil(RuntimeIdentity.parse("runtime linked"))
        XCTAssertNil(RuntimeIdentity.parse("prefix cdc abi=1.5 grammar=1"))
        XCTAssertNil(RuntimeIdentity.parse("cdc abi=1.5 grammar=1 trailing"))
        XCTAssertNil(RuntimeIdentity.parse("cdc abi=1.5 grammar=1\nspoof"))
    }

    func testRuntimeCompatibilityRejectsShellShimSymlinkAndNonRegularTargets() throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        let binary = root.appendingPathComponent("build/cdc")

        try installRuntime("#!/bin/sh\necho 'cdc abi=1.5 grammar=1'\n", in: root)
        XCTAssertThrowsError(try RuntimeExecutableValidator.inspect(binary: binary, repository: root)) {
            XCTAssertEqual($0 as? RuntimeExecutableValidationError, .unsupportedFormat(binary.path))
        }

        try FileManager.default.removeItem(at: binary)
        try FileManager.default.createSymbolicLink(
            at: binary,
            withDestinationURL: URL(fileURLWithPath: "/bin/echo")
        )
        XCTAssertThrowsError(try RuntimeExecutableValidator.inspect(binary: binary, repository: root)) {
            XCTAssertEqual($0 as? RuntimeExecutableValidationError, .symbolicLink(binary.path))
        }

        try FileManager.default.removeItem(at: binary)
        try FileManager.default.createDirectory(at: binary, withIntermediateDirectories: false)
        XCTAssertThrowsError(try RuntimeExecutableValidator.inspect(binary: binary, repository: root)) {
            XCTAssertEqual($0 as? RuntimeExecutableValidationError, .notRegularFile(binary.path))
        }
    }

    func testPreparedRuntimeExecutesCapturedBytesAfterRepositoryPathReplacement() async throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        try installNativeRuntime(
            "#include <stdio.h>\nint main(void) { puts(\"captured-benign-runtime\"); return 0; }\n",
            in: root
        )

        let binary = root.appendingPathComponent("build/cdc")
        let prepared = try RuntimeExecutableValidator.prepare(binary: binary, repository: root)
        let preparedDirectory = prepared.executableURL.deletingLastPathComponent()
        defer { prepared.cleanup() }
        XCTAssertNotEqual(prepared.executableURL.standardizedFileURL.path, binary.standardizedFileURL.path)
        let permissions = try XCTUnwrap(
            FileManager.default.attributesOfItem(atPath: prepared.executableURL.path)[.posixPermissions]
                as? NSNumber
        )
        XCTAssertEqual(permissions.intValue & 0o777, 0o700)
        let directoryPermissions = try XCTUnwrap(
            FileManager.default.attributesOfItem(atPath: preparedDirectory.path)[.posixPermissions]
                as? NSNumber
        )
        XCTAssertEqual(directoryPermissions.intValue & 0o777, 0o700)

        let marker = root.appendingPathComponent("build/replacement-executed")
        try installNativeRuntime(
            """
            #include <stdio.h>
            int main(void) {
                FILE *file = fopen("build/replacement-executed", "w");
                if (file) fclose(file);
                puts("malicious-replacement-runtime");
                return 0;
            }
            """,
            in: root
        )

        let runner = ToolchainProcessRunner()
        let outcome = await runner.execute(
            binary: prepared.executableURL,
            arguments: [],
            workingDirectory: root,
            timeout: 5,
            ticket: runner.reserve()
        )
        XCTAssertEqual(outcome.exitCode, 0)
        XCTAssertEqual(outcome.stdout.trimmingCharacters(in: .whitespacesAndNewlines), "captured-benign-runtime")
        XCTAssertFalse(FileManager.default.fileExists(atPath: marker.path))

        prepared.cleanup()
        XCTAssertFalse(FileManager.default.fileExists(atPath: preparedDirectory.path))
    }

    func testPersistenceAuthorizationRequiresExactRepositoryRelativePath() {
        let root = URL(fileURLWithPath: "/tmp/cdc-persistence-authorization")
        let admitted = SourceDocument(
            url: root.appendingPathComponent("framework_persistence.cdc"),
            relativePath: "framework_persistence.cdc",
            kind: .project
        )
        let nested = SourceDocument(
            url: root.appendingPathComponent("examples/framework_persistence.cdc"),
            relativePath: "examples/framework_persistence.cdc",
            kind: .project
        )
        let fixture = SourceDocument(
            url: root.appendingPathComponent("tests/fixtures/framework_persistence.cdc"),
            relativePath: "tests/fixtures/framework_persistence.cdc",
            kind: .fixture
        )

        XCTAssertTrue(admitted.authorizesPersistenceEffects)
        XCTAssertFalse(nested.authorizesPersistenceEffects)
        XCTAssertFalse(fixture.authorizesPersistenceEffects)
    }

    @MainActor
    func testToolchainRejectsNestedPersistenceBeforeLaunchingRuntime() async throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        let exactURL = root.appendingPathComponent("framework_persistence.cdc")
        let nestedURL = root.appendingPathComponent("tests/fixtures/u2/framework_persistence.cdc")
        try "persist exact".write(to: exactURL, atomically: true, encoding: .utf8)
        try "persist fixture".write(to: nestedURL, atomically: true, encoding: .utf8)
        try installNativeRuntime(
            """
            #include <stdio.h>
            #include <string.h>
            int main(int argc, char **argv) {
                if (argc > 1 && strcmp(argv[1], "version") == 0) {
                    puts("cdc abi=1.5 grammar=1");
                    return 0;
                }
                if (argc > 1 && strcmp(argv[1], "persist") == 0) {
                    FILE *marker = fopen("build/persisted", "w");
                    if (!marker) return 2;
                    fclose(marker);
                    return 0;
                }
                return 1;
            }
            """,
            in: root
        )

        let service = ToolchainService(repositoryURL: root, environment: [:])
        await service.bootstrap()
        let nested = try XCTUnwrap(service.sources.first { $0.relativePath == "tests/fixtures/u2/framework_persistence.cdc" })
        let exact = try XCTUnwrap(service.sources.first { $0.relativePath == "framework_persistence.cdc" })
        let marker = root.appendingPathComponent("build/persisted")

        service.selectSource(nested)
        let rejectedResult = await service.run(["persist", nested.relativePath])
        let rejected = try XCTUnwrap(rejectedResult)
        XCTAssertEqual(rejected.exitCode, -5)
        XCTAssertFalse(FileManager.default.fileExists(atPath: marker.path))

        service.selectSource(exact)
        let admittedResult = await service.run(["persist", exact.relativePath])
        let admitted = try XCTUnwrap(admittedResult)
        XCTAssertTrue(admitted.succeeded)
        XCTAssertTrue(FileManager.default.fileExists(atPath: marker.path))
    }

    @MainActor
    func testDiscoveryRejectsLateRepositoryResponsesAcrossAtoBtoA() async throws {
        let repositoryA = try makeRepository()
        let repositoryB = try makeRepository()
        defer {
            try? FileManager.default.removeItem(at: repositoryA)
            try? FileManager.default.removeItem(at: repositoryB)
        }
        let runtime = "#include <stdio.h>\nint main(void) { puts(\"cdc abi=1.5 grammar=1\"); return 0; }\n"
        try installNativeRuntime(runtime, in: repositoryA)
        try installNativeRuntime(runtime, in: repositoryB)

        let service = ToolchainService(environment: [:], snapshotLoader: { url in
            if url.standardizedFileURL == repositoryA.standardizedFileURL {
                try await Task.sleep(for: .milliseconds(240))
            } else {
                try await Task.sleep(for: .milliseconds(80))
            }
            return try RepositoryDiscovery.snapshot(at: url)
        })

        let firstA = Task { await service.chooseRepository(repositoryA) }
        try await Task.sleep(for: .milliseconds(20))
        let middleB = Task { await service.chooseRepository(repositoryB) }
        try await Task.sleep(for: .milliseconds(20))
        await service.chooseRepository(repositoryA)
        await firstA.value
        await middleB.value

        XCTAssertEqual(service.contextGeneration.repository, 3)
        XCTAssertEqual(service.contextGeneration.source, 3)
        XCTAssertEqual(service.repositoryURL?.standardizedFileURL, repositoryA.standardizedFileURL)
        guard case let .ready(identity) = service.runtimeState else {
            return XCTFail("the final A context did not own the runtime result")
        }
        XCTAssertEqual(identity.raw, "cdc abi=1.5 grammar=1")
    }

    @MainActor
    func testProbeRejectsLateResponsesAcrossRepositoryAtoBtoA() async throws {
        let repositoryA = try makeRepository()
        let repositoryB = try makeRepository()
        defer {
            try? FileManager.default.removeItem(at: repositoryA)
            try? FileManager.default.removeItem(at: repositoryB)
        }
        try "delay".write(to: repositoryA.appendingPathComponent("build/delay-once"), atomically: true, encoding: .utf8)
        try "delay".write(to: repositoryB.appendingPathComponent("build/delay-once"), atomically: true, encoding: .utf8)
        let delayedRuntime = """
        #include <signal.h>
        #include <stdio.h>
        #include <unistd.h>
        int main(void) {
            if (access("build/delay-once", F_OK) == 0) {
                unlink("build/delay-once");
                signal(SIGTERM, SIG_IGN);
                sleep(1);
            }
            puts("cdc abi=1.5 grammar=1");
            return 0;
        }
        """
        try installNativeRuntime(delayedRuntime, in: repositoryA)
        try installNativeRuntime(delayedRuntime, in: repositoryB)

        let service = ToolchainService(environment: [:])
        let firstA = Task { await service.chooseRepository(repositoryA) }
        try await waitUntil {
            service.isRunning &&
            service.repositoryURL?.standardizedFileURL == repositoryA.standardizedFileURL &&
            !FileManager.default.fileExists(atPath: repositoryA.appendingPathComponent("build/delay-once").path)
        }

        let middleB = Task { await service.chooseRepository(repositoryB) }
        try await waitUntil {
            service.isRunning &&
            service.repositoryURL?.standardizedFileURL == repositoryB.standardizedFileURL &&
            !FileManager.default.fileExists(atPath: repositoryB.appendingPathComponent("build/delay-once").path)
        }

        await service.chooseRepository(repositoryA)
        await firstA.value
        await middleB.value

        XCTAssertEqual(service.contextGeneration.repository, 3)
        XCTAssertEqual(service.repositoryURL?.standardizedFileURL, repositoryA.standardizedFileURL)
        guard case let .ready(identity) = service.runtimeState else {
            return XCTFail("the final A probe did not become authoritative")
        }
        XCTAssertEqual(identity.raw, "cdc abi=1.5 grammar=1")
    }

    @MainActor
    func testAnalyzeRejectsLateResponseAcrossSourceAtoBtoA() async throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        try "alternate".write(to: root.appendingPathComponent("alternate.cdc"), atomically: true, encoding: .utf8)
        try installNativeRuntime(
            """
            #include <signal.h>
            #include <stdio.h>
            #include <string.h>
            #include <unistd.h>
            int main(int argc, char **argv) {
                if (argc > 1 && strcmp(argv[1], "version") == 0) {
                    puts("cdc abi=1.5 grammar=1");
                    return 0;
                }
                signal(SIGTERM, SIG_IGN);
                sleep(1);
                puts("u2-json={}");
                return 0;
            }
            """,
            in: root
        )

        let service = ToolchainService(repositoryURL: root, environment: [:])
        await service.bootstrap()
        let sourceA = try XCTUnwrap(service.sources.first { $0.relativePath == "framework_loop.cdc" })
        let sourceB = try XCTUnwrap(service.sources.first { $0.relativePath == "alternate.cdc" })
        let initialGeneration = service.contextGeneration

        let analysis = Task { await service.analyzeSelectedSource() }
        try await waitUntil { service.isRunning }
        service.selectSource(sourceB)
        service.selectSource(sourceA)
        await analysis.value

        XCTAssertEqual(service.selectedSource, sourceA)
        XCTAssertEqual(service.contextGeneration.repository, initialGeneration.repository)
        XCTAssertEqual(service.contextGeneration.source, initialGeneration.source + 2)
        XCTAssertNil(service.selectedOrbitID)
        XCTAssertTrue(service.history.isEmpty, "stale invocation must not enter history")
        guard case .idle = service.stabilityState else {
            return XCTFail("late A response overwrote the final A context: \(service.stabilityState)")
        }
    }

    @MainActor
    func testProbeRejectsLateResponseAcrossSourceAtoBtoA() async throws {
        let root = try makeRepository()
        defer { try? FileManager.default.removeItem(at: root) }
        try "alternate".write(to: root.appendingPathComponent("alternate.cdc"), atomically: true, encoding: .utf8)
        try "delay".write(to: root.appendingPathComponent("build/delay-once"), atomically: true, encoding: .utf8)
        try installNativeRuntime(
            """
            #include <signal.h>
            #include <stdio.h>
            #include <unistd.h>
            int main(void) {
                if (access("build/delay-once", F_OK) == 0) {
                    unlink("build/delay-once");
                    signal(SIGTERM, SIG_IGN);
                    sleep(1);
                    puts("cdc abi=1.2 grammar=1");
                    return 0;
                }
                puts("cdc abi=1.5 grammar=1");
                return 0;
            }
            """,
            in: root
        )

        let service = ToolchainService(repositoryURL: root, environment: [:])
        let bootstrap = Task { await service.bootstrap() }
        try await waitUntil {
            service.isRunning &&
            !FileManager.default.fileExists(atPath: root.appendingPathComponent("build/delay-once").path)
        }
        let sourceA = try XCTUnwrap(service.sources.first { $0.relativePath == "framework_loop.cdc" })
        let sourceB = try XCTUnwrap(service.sources.first { $0.relativePath == "alternate.cdc" })
        let initialGeneration = service.contextGeneration

        service.selectSource(sourceB)
        service.selectSource(sourceA)
        await bootstrap.value
        try await waitUntil {
            if case .ready = service.runtimeState { return true }
            return false
        }

        XCTAssertEqual(service.contextGeneration.repository, initialGeneration.repository)
        XCTAssertEqual(service.contextGeneration.source, initialGeneration.source + 2)
        XCTAssertEqual(service.selectedSource, sourceA)
        guard case let .ready(identity) = service.runtimeState else {
            return XCTFail("final source context did not receive a fresh runtime probe")
        }
        XCTAssertEqual(identity.raw, "cdc abi=1.5 grammar=1")
    }

    @MainActor
    private func waitUntil(
        timeout: Duration = .seconds(5),
        _ condition: @escaping @MainActor () -> Bool
    ) async throws {
        let clock = ContinuousClock()
        let deadline = clock.now.advanced(by: timeout)
        while !condition() {
            if clock.now >= deadline {
                throw NSError(domain: "RepositoryDiscoveryTests", code: 1, userInfo: [
                    NSLocalizedDescriptionKey: "timed out waiting for asynchronous context transition"
                ])
            }
            try await Task.sleep(for: .milliseconds(20))
        }
    }
}

import Foundation

/// Typed model of the runtime's replay record (`cdc replay …`, tracked at
/// `demo/replay.json`). The app never invents data: every field here is emitted
/// by the native runtime, and unknown fields are absent rather than defaulted.
struct ReplayRecord: Codable, Equatable {
    struct Flow: Codable, Equatable {
        let thetaCouncilB: String
        let channelWeight: String
    }

    struct CommitRecord: Codable, Equatable {
        let trits: String
        let balance: String
        let status: String
        let reason: String
    }

    struct Nest: Codable, Equatable {
        let up: String
        let parentBelief: String
        let childPrior: String
    }

    struct Trace: Codable, Equatable {
        let trits: String
        let events: String
    }

    struct Bridge: Codable, Equatable {
        let dyadic: String
        let triadic: String
        let index: String
    }

    struct Universal: Codable, Equatable {
        let `operator`: String
        let frame: String
        let receptiveAngle: String
        let radiantAngle: String
        let holonomy: String
        let halfProjection: String
        let halfSheet: String
        let fullProjection: String
        let fullSheet: String
        let recordCoordinate: String
        let decisionCoordinate: String

        var windingRestored: Bool { fullSheet == "restored" }
        var coordinatesAgree: Bool { recordCoordinate == decisionCoordinate }
        var holonomyValue: Double { Double(holonomy) ?? 0 }
    }

    let flow: Flow
    let commit: CommitRecord
    let hold: CommitRecord
    let nest: Nest
    let trace: Trace
    let bridge: Bridge
    let universal: Universal

    static func load(from url: URL) throws -> ReplayRecord {
        let data = try Data(contentsOf: url)
        return try JSONDecoder().decode(ReplayRecord.self, from: data)
    }
}

/// One row of the typed test report. Counts are carried separately and are
/// never summed into a single "passed" number (Amendment A7).
struct TestTally: Equatable {
    var runs = 0
    var commit = 0
    var hold = 0
    var expectedHold = 0
    var unexpectedHold = 0
    var nest = 0
    var fail = 0

    var gateGreen: Bool { fail == 0 && unexpectedHold == 0 && runs > 0 }

    /// Parses the runtime's own report line so the UI and the gate can never
    /// disagree about what happened.
    static func parse(_ line: String) -> TestTally? {
        guard line.contains("cdc test") else { return nil }
        func value(_ key: String) -> Int {
            guard let range = line.range(of: "\(key)=") else { return 0 }
            let rest = line[range.upperBound...]
            let digits = rest.prefix { $0.isNumber }
            return Int(digits) ?? 0
        }
        var tally = TestTally()
        tally.runs = value("runs")
        tally.commit = value("commit")
        tally.hold = value("hold")
        tally.expectedHold = value("expected")
        tally.unexpectedHold = value("unexpected")
        tally.nest = value("nest")
        tally.fail = value("fail")
        return tally
    }
}

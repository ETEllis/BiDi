import Foundation

struct U2StabilityRecord: Decodable, Equatable, Identifiable, Sendable {
    struct Universal: Decodable, Equatable, Sendable {
        let id: String
        let status: String
        let reason: String
        let resultDigest: String
        let receptiveAngle: String
        let radiantAngle: String
        let holonomy: String
        let winding: Int
    }

    struct Methods: Decodable, Equatable, Sendable {
        let flow: String
        let commit: String
        let nest: String
        let `guard`: String
        let spectrum: String
    }

    struct MethodDetail: Decodable, Equatable, Sendable {
        struct Flow: Decodable, Equatable, Sendable {
            let localMap: String
            let jacobian: String
            let finiteDifferenceOracle: String?
        }

        struct Commit: Decodable, Equatable, Sendable {
            let kind: String
            let continuousDerivative: String
            let saltation: String
        }

        struct Nest: Decodable, Equatable, Sendable {
            let localMap: String
            let jacobian: String
            let childPrior: String
        }

        struct Guard: Decodable, Equatable, Sendable {
            let binding: String?
            let eventLocalization: String?
            let saltation: String?
        }

        struct Spectrum: Decodable, Equatable, Sendable {
            let solver: String
            let ordering: String
        }

        let flow: Flow
        let commit: Commit
        let nest: Nest
        let `guard`: Guard
        let spectrum: Spectrum
    }

    struct Tolerances: Decodable, Equatable, Sendable {
        let recurrenceAbsolute: Double
        let recurrenceRelative: Double
        let neutral: Double
        let schur: Double
    }

    struct FiniteDifference: Decodable, Equatable, Sendable {
        let status: String
        let step: Double?
        let residual: Double?
    }

    struct EventBudget: Decodable, Equatable, Sendable {
        let status: String
        let limit: Int?
        let used: Int?
    }

    struct NeutralModeRemoval: Decodable, Equatable, Sendable {
        let status: String
        let generatorDigest: String?
        let removedModes: [Int]?
    }

    struct Recurrence: Decodable, Equatable, Sendable {
        struct Restoration: Decodable, Equatable, Sendable {
            struct EquivarianceWitness: Decodable, Equatable, Sendable {
                let id: String
                let verified: Bool
                let fieldGain: Double
                let fieldCellCount: Int
                let incidentChannelCount: Int
                let mutatingStepCount: Int
            }

            struct SectionWitness: Decodable, Equatable, Sendable {
                let id: String
                let verified: Bool
                let winding: Int
                let projection: String
                let sheet: String
            }

            let action: String
            let coordinateIndex: Int
            let coordinate: String
            let displacement: Double
            let equivarianceWitness: EquivarianceWitness
            let sectionWitness: SectionWitness
            let derivativeRows: Int
            let derivativeColumns: Int
            let derivativeDigest: String
            let derivative: [Double]
        }

        let kind: String
        let scope: String
        let residual: Double
        let absoluteTolerance: Double
        let relativeTolerance: Double
        let normalizedResidual: Double
        let verified: Bool
        let authorizesMonodromy: Bool
        let discreteStateVerified: Bool
        let restorationDerivativeApplied: Bool
        let restoration: Restoration?
    }

    struct DiscreteState: Decodable, Equatable, Sendable {
        struct Entry: Decodable, Equatable, Sendable {
            let cell: String
            let coordinate: String
            let sourceIndex: Int
            let hasLatch: Bool
            let latch: String?
            let mode: String
        }

        let verified: Bool
        let initialDigest: String
        let finalDigest: String
        let initial: [Entry]
        let final: [Entry]
    }

    struct Coordinate: Decodable, Equatable, Sendable {
        let name: String
        let period: Double
    }

    struct Event: Decodable, Equatable, Sendable {
        let kind: String
        let time: Double
        let transverse: Bool?
    }

    struct Multiplier: Decodable, Equatable, Sendable {
        let real: Double
        let imag: Double
        let modulus: Double
        let mode: String
    }

    struct SpectrumDiagnostics: Decodable, Equatable, Sendable {
        struct Schur: Decodable, Equatable, Sendable {
            let reconstructionResidual: Double
            let orthogonalityResidual: Double
            let triangularResidual: Double
            let validationTolerance: Double
        }

        let spectralRadius: Double
        let schur: Schur
    }

    let schema: String
    let orbit: String
    let status: String
    let reason: String
    let analysis: String
    let runtimeIdentity: String
    let sourceDigest: String
    let sourceDigestSurface: String
    let universal: Universal
    let manifestDigest: String?
    let methods: Methods
    let methodDetail: MethodDetail?
    let tolerances: Tolerances
    let finiteDifference: FiniteDifference?
    let eventBudget: EventBudget?
    let neutralModeRemoval: NeutralModeRemoval?
    let determinism: String
    let recurrence: Recurrence?
    let discreteState: DiscreteState?
    let dimension: Int?
    let coordinates: [Coordinate]?
    let initialState: [Double]?
    let finalState: [Double]?
    let pathTangentDigest: String?
    let pathTangent: [Double]?
    let events: [Event]
    let monodromyDigest: String?
    let monodromy: [Double]?
    let multipliers: [Multiplier]?
    let spectrumDiagnostics: SpectrumDiagnostics?
    let backend: String?
    let classification: String

    var id: String { orbit }
    var accepted: Bool { status == "accepted" }
    var primalAccepted: Bool { universal.status == "accepted" }
}

struct CanonicalU2Receipt: Equatable, Identifiable, Sendable {
    let record: U2StabilityRecord
    let rawJSON: String
    let lineNumber: Int

    var id: String { record.id }
}

struct U2ReceiptFailure: Error, Equatable, Sendable, CustomStringConvertible {
    enum Kind: Equatable, Sendable {
        case noCanonicalRecord
        case malformedJSON(line: Int)
        case unsupportedSchema(String)
        case invalidShape(String)
    }

    let kind: Kind
    let detail: String

    var description: String { detail }
}

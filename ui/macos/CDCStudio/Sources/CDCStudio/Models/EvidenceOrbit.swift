import Foundation

enum EvidenceStageKind: String, CaseIterable, Identifiable, Sendable {
    case source = "Source"
    case universal = "U1 Closure"
    case tangent = "Tangent"
    case recurrence = "Recurrence"
    case spectrum = "Spectrum"

    var id: String { rawValue }
}

enum EvidenceStageState: Equatable, Sendable {
    case unavailable(String)
    case ready(String)
    case held(String)
    case analyzed(String)
    case malformed(String)

    var label: String {
        switch self {
        case .unavailable: return "unavailable"
        case .ready: return "ready"
        case .held: return "held"
        case .analyzed: return "analyzed"
        case .malformed: return "malformed"
        }
    }

    var detail: String {
        switch self {
        case let .unavailable(detail), let .ready(detail), let .held(detail),
             let .analyzed(detail), let .malformed(detail): return detail
        }
    }
}

struct EvidenceStage: Identifiable, Equatable, Sendable {
    let kind: EvidenceStageKind
    let state: EvidenceStageState
    var id: String { kind.id }
}

struct EvidenceOrbit: Equatable, Sendable {
    let receipt: CanonicalU2Receipt
    let stages: [EvidenceStage]
    let claimCeiling: String

    init(receipt: CanonicalU2Receipt) {
        self.receipt = receipt
        let record = receipt.record

        var stages: [EvidenceStage] = [
            EvidenceStage(
                kind: .source,
                state: .analyzed("\(record.sourceDigestSurface) · \(record.sourceDigest)")
            )
        ]

        if record.primalAccepted {
            stages.append(EvidenceStage(
                kind: .universal,
                state: .analyzed("\(record.universal.id) · winding \(record.universal.winding) · \(record.universal.resultDigest)")
            ))
        } else {
            stages.append(EvidenceStage(
                kind: .universal,
                state: .held(record.universal.reason)
            ))
        }

        if record.analysis == "held" {
            if record.primalAccepted {
                stages.append(EvidenceStage(kind: .tangent, state: .held(record.reason)))
                stages.append(EvidenceStage(kind: .recurrence, state: .unavailable("No admitted tangent endpoint")))
                stages.append(EvidenceStage(kind: .spectrum, state: .unavailable("No monodromy was constructed")))
                let manifest = record.dimension.map { " A \($0)-coordinate source manifest was bound." } ?? ""
                claimCeiling = "U1 accepted.\(manifest) Tangent analysis held: \(record.reason). No recurrence, monodromy, or spectrum exists for this run."
            } else {
                stages.append(EvidenceStage(kind: .tangent, state: .unavailable("U1 did not admit tangent state")))
                stages.append(EvidenceStage(kind: .recurrence, state: .unavailable("No admitted tangent endpoint")))
                stages.append(EvidenceStage(kind: .spectrum, state: .unavailable("Monodromy is forbidden")))
                claimCeiling = "U1 held: \(record.universal.reason). No U2 state or tangent was admitted."
            }
        } else {
            stages.append(EvidenceStage(
                kind: .tangent,
                state: .analyzed("dimension \(record.dimension ?? 0) · \(record.events.count) ordered events")
            ))

            if let recurrence = record.recurrence, recurrence.verified {
                stages.append(EvidenceStage(
                    kind: .recurrence,
                    state: .analyzed("\(recurrence.scope) return · residual \(Self.number(recurrence.residual))")
                ))
            } else {
                stages.append(EvidenceStage(
                    kind: .recurrence,
                    state: .held(record.reason)
                ))
            }

            if record.accepted {
                stages.append(EvidenceStage(
                    kind: .spectrum,
                    state: .analyzed("\(record.classification) · \(record.multipliers?.count ?? 0) multipliers")
                ))
                claimCeiling = "\(record.recurrence?.scope.capitalized ?? "Returned") recurrence verified. Monodromy analyzed as \(record.classification)."
            } else if record.analysis == "monodromy", record.recurrence?.verified == true {
                stages.append(EvidenceStage(
                    kind: .spectrum,
                    state: .held(record.reason)
                ))
                claimCeiling = "\(record.recurrence?.scope.capitalized ?? "Returned") recurrence verified and monodromy retained. Spectrum held: \(record.reason). No multiplier classification exists for this run."
            } else {
                stages.append(EvidenceStage(
                    kind: .spectrum,
                    state: .unavailable("No spectrum: \(record.reason)")
                ))
                claimCeiling = "U1 accepted and the U2 tangent executed. Recurrence held: \(record.reason). No Floquet spectrum exists for this run."
            }
        }
        self.stages = stages
    }

    private static func number(_ value: Double) -> String {
        String(format: "%.4g", value)
    }
}

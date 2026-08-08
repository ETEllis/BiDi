import SwiftUI

/// The product's result carrier is balanced ternary. This is the single most
/// important interface decision in the app: **zero is not a failure.** A held
/// outcome is resting equilibrium — an admissible, informative state — and it
/// is styled as calm, not as an error. Only a violated invariant is warm.
enum TernaryOutcome: Int, CaseIterable, Identifiable {
    case violated = -1
    case held = 0
    case commit = 1

    var id: Int { rawValue }

    init(status: String) {
        switch status.lowercased() {
        case "accepted", "commit", "committed", "latched": self = .commit
        case "held", "hold", "nest", "nested":             self = .held
        default:                                            self = .violated
        }
    }

    var glyph: String {
        switch self {
        case .commit:   return "+"
        case .held:     return "0"
        case .violated: return "−"
        }
    }

    var label: String {
        switch self {
        case .commit:   return "accepted"
        case .held:     return "held"
        case .violated: return "violated"
        }
    }

    var tint: Color {
        switch self {
        case .commit:   return Mobius.Resolve.cyan
        case .held:     return Mobius.Phase.indigo
        case .violated: return Mobius.Ember.core
        }
    }

    /// Held state deliberately reads as *equilibrium*, so it carries the
    /// quietest fill of the three. Violation is the only state that raises its
    /// voice.
    var fillOpacity: Double {
        switch self {
        case .commit:   return 0.16
        case .held:     return 0.10
        case .violated: return 0.20
        }
    }
}

/// Compact, self-evident outcome chip. Reads correctly at a glance and still
/// carries its reason when one exists.
struct TernaryBadge: View {
    let outcome: TernaryOutcome
    var reason: String? = nil
    var prominent: Bool = false

    var body: some View {
        HStack(spacing: Mobius.Space.sm) {
            Text(outcome.glyph)
                .font(.system(size: prominent ? 15 : 12, weight: .bold, design: .monospaced))
                .frame(width: prominent ? 22 : 18, height: prominent ? 22 : 18)
                .background(Circle().fill(outcome.tint.opacity(0.22)))
                .overlay(Circle().strokeBorder(outcome.tint.opacity(0.55), lineWidth: 1))
                .foregroundStyle(outcome.tint)

            Text(outcome.label)
                .font(.system(size: prominent ? 13 : 11, weight: .semibold))
                .foregroundStyle(outcome.tint)

            if let reason, reason != "none", !reason.isEmpty {
                Text(reason)
                    .addressFont(11)
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                    .padding(.horizontal, Mobius.Space.sm)
                    .padding(.vertical, 2)
                    .background(
                        Capsule().fill(Mobius.Substrate.raised)
                    )
            }
        }
        .padding(.horizontal, Mobius.Space.sm + 2)
        .padding(.vertical, Mobius.Space.xs + 2)
        .background(
            Capsule().fill(outcome.tint.opacity(outcome.fillOpacity))
        )
        .overlay(
            Capsule().strokeBorder(outcome.tint.opacity(0.30), lineWidth: 1)
        )
        .accessibilityElement(children: .combine)
        .accessibilityLabel("\(outcome.label)\(reason.map { $0 == "none" ? "" : ", reason \($0)" } ?? "")")
    }
}

/// A trit word (e.g. `0+-`) rendered as discrete cells. Each position is an
/// independent commitment, so it gets its own cell rather than being flattened
/// into a string.
struct TritWord: View {
    let trits: String
    var cell: CGFloat = 22

    var body: some View {
        HStack(spacing: 3) {
            ForEach(Array(trits.enumerated()), id: \.offset) { _, character in
                let outcome = Self.outcome(for: character)
                Text(String(character == "-" ? "−" : character))
                    .font(.system(size: cell * 0.55, weight: .semibold, design: .monospaced))
                    .frame(width: cell, height: cell)
                    .foregroundStyle(outcome.tint)
                    .background(
                        RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                            .fill(outcome.tint.opacity(0.12))
                    )
                    .overlay(
                        RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                            .strokeBorder(outcome.tint.opacity(0.35), lineWidth: 1)
                    )
            }
        }
        .accessibilityElement(children: .ignore)
        .accessibilityLabel("trits \(trits)")
    }

    private static func outcome(for character: Character) -> TernaryOutcome {
        switch character {
        case "+": return .commit
        case "-", "−": return .violated
        default: return .held
        }
    }
}

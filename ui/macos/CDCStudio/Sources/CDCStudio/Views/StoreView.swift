import SwiftUI

/// The durable store, presented through its actual failure taxonomy. Most
/// storage UIs show "healthy / unhealthy"; this one shows the three states the
/// implementation genuinely distinguishes, because conflating them is the bug
/// class the store was hardened against.
struct StoreView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var lastCheck: ToolchainService.Invocation?

    private let taxonomy: [(TernaryOutcome, String, String)] = [
        (.commit, "recoverable tail",
         "A torn or valid-unsealed tail is truncated at the last seal. The unfinished transaction never happened."),
        (.violated, "corrupt prefix",
         "Any integrity violation inside a complete record fails closed. No handle is returned and the bytes are never modified — corrupted evidence is preserved, not repaired."),
        (.held, "I/O fault",
         "A read fault is a fault, never an empty store and never a torn tail. Nothing is truncated.")
    ]

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.xl) {
                VStack(alignment: .leading, spacing: Mobius.Space.xs) {
                    Text("Durable store")
                        .font(.system(size: 30, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                        .tracking(-0.5)
                    Text("Append-only sealed transactions · authenticated framing · BLAKE3 identity")
                        .font(.system(size: 12))
                        .foregroundStyle(Mobius.Phase.indigo)
                }

                VStack(alignment: .leading, spacing: Mobius.Space.md) {
                    SectionLabel("Failure taxonomy")
                    ForEach(Array(taxonomy.enumerated()), id: \.offset) { _, entry in
                        HStack(alignment: .top, spacing: Mobius.Space.md) {
                            TernaryBadge(outcome: entry.0)
                                .frame(width: 150, alignment: .leading)
                            VStack(alignment: .leading, spacing: 2) {
                                Text(entry.1)
                                    .font(.system(size: 13, weight: .semibold))
                                    .foregroundStyle(Mobius.Hinge.paper)
                                Text(entry.2)
                                    .font(.system(size: 11))
                                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.55))
                                    .fixedSize(horizontal: false, vertical: true)
                            }
                            Spacer(minLength: 0)
                        }
                        .padding(.vertical, Mobius.Space.sm)
                    }
                }
                .padding(Mobius.Space.lg)
                .panel()

                if let lastCheck {
                    OutputPanel(invocation: lastCheck)
                }
            }
            .padding(Mobius.Space.xl)
        }
    }
}

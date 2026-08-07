import SwiftUI

struct StoreView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var lastCheck: ToolchainService.Invocation?
    @State private var confirmsPersistence = false

    private let taxonomy: [(TernaryOutcome, String, String)] = [
        (.commit, "recoverable tail",
         "A torn or valid-unsealed tail is truncated at the last seal. The unfinished transaction never happened."),
        (.violated, "corrupt prefix",
         "Any integrity violation inside a complete record fails closed. No handle is returned and the bytes are never modified."),
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

                HStack(spacing: Mobius.Space.md) {
                    ActionButton(title: "Verify Contract", symbol: "checkmark.shield", disabledReason: commonDisabledReason) {
                        guard let source = toolchain.selectedSource else { return }
                        guard let invocation = await toolchain.run(["verify", "--contract", source.relativePath]) else { return }
                        lastCheck = invocation
                    }
                    ActionButton(title: "Run Persistence", symbol: "externaldrive.badge.checkmark", disabledReason: persistenceDisabledReason) {
                        confirmsPersistence = true
                    }
                    if toolchain.isRunning {
                        Button("Cancel", systemImage: "stop.fill") { toolchain.cancelCurrentRun() }
                            .buttonStyle(.bordered)
                    }
                }

                if persistenceDisabledReason != nil, toolchain.selectedSource != nil {
                    Text("Select framework_persistence.cdc to execute declared persistence effects. Contract verification remains available for any source.")
                        .font(.system(size: 11.5))
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                }

                VStack(alignment: .leading, spacing: Mobius.Space.md) {
                    SectionLabel("Failure taxonomy")
                    ForEach(Array(taxonomy.enumerated()), id: \.offset) { _, entry in
                        HStack(alignment: .top, spacing: Mobius.Space.md) {
                            TernaryBadge(outcome: entry.0)
                                .frame(width: 150, alignment: .leading)
                            VStack(alignment: .leading, spacing: 3) {
                                Text(entry.1)
                                    .font(.system(size: 13, weight: .semibold))
                                    .foregroundStyle(Mobius.Hinge.paper)
                                Text(entry.2)
                                    .font(.system(size: 11.5))
                                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                                    .fixedSize(horizontal: false, vertical: true)
                            }
                            Spacer(minLength: 0)
                        }
                        .padding(.vertical, Mobius.Space.sm)
                    }
                }
                .padding(Mobius.Space.lg)
                .panel()

                if let lastCheck { OutputPanel(invocation: lastCheck) }
            }
            .padding(Mobius.Space.xl)
            .frame(maxWidth: 1050, alignment: .leading)
        }
        .confirmationDialog(
            "Run declared persistence effects?",
            isPresented: $confirmsPersistence,
            titleVisibility: .visible
        ) {
            Button("Run Persistence") {
                Task {
                    guard let source = toolchain.selectedSource,
                          source.authorizesPersistenceEffects,
                          let invocation = await toolchain.run(["persist", source.relativePath]) else { return }
                    lastCheck = invocation
                }
            }
            Button("Cancel", role: .cancel) { }
        } message: {
            Text("This invokes the real CDC runtime and may write the output declared by framework_persistence.cdc. The action is never automatic.")
        }
        .onChange(of: toolchain.contextGeneration) { _, _ in
            lastCheck = nil
            confirmsPersistence = false
        }
    }

    private var commonDisabledReason: String? {
        if toolchain.selectedSource == nil { return "Select a source first." }
        if !toolchain.runtimeState.isReady { return "An ABI-compatible native runtime is required." }
        if toolchain.isRunning { return "Another runtime invocation is active." }
        return nil
    }

    private var persistenceDisabledReason: String? {
        if let commonDisabledReason { return commonDisabledReason }
        guard toolchain.selectedSource?.authorizesPersistenceEffects == true else {
            return "Select framework_persistence.cdc before executing persistence effects."
        }
        return nil
    }
}

import SwiftUI
import UniformTypeIdentifiers

@main
struct CDCStudioApp: App {
    @StateObject private var toolchain = ToolchainService()

    var body: some Scene {
        Window("CDC Studio", id: "studio") {
            RootView()
                .environmentObject(toolchain)
                .frame(minWidth: 1040, minHeight: 660)
                .preferredColorScheme(.dark)
        }
        .windowStyle(.hiddenTitleBar)
        .commands {
            CommandGroup(replacing: .newItem) { }
            CommandMenu("Repository") {
                Button("Choose Repository…") {
                    toolchain.isRepositoryPickerPresented = true
                }
                .keyboardShortcut("o", modifiers: .command)

                Button("Refresh Repository") {
                    Task { await toolchain.refreshRepository() }
                }
                .keyboardShortcut("r", modifiers: [.command, .shift])
                .disabled(toolchain.repositoryURL == nil || toolchain.isRunning)

                Divider()

                Button("Analyze Selected Orbit") {
                    Task { await toolchain.analyzeSelectedSource() }
                }
                .keyboardShortcut(.return, modifiers: .command)
                .disabled(toolchain.selectedSource == nil || !toolchain.runtimeState.isReady || toolchain.isRunning)
            }
        }
    }
}

enum Workspace: String, CaseIterable, Identifiable {
    case execution = "Execution"
    case operatorClosure = "Universal Operator"
    case store = "Store"
    case bridge = "Bridge"

    var id: String { rawValue }

    var symbol: String {
        switch self {
        case .execution: return "play.circle"
        case .operatorClosure: return "infinity"
        case .store: return "cylinder.split.1x2"
        case .bridge: return "grid"
        }
    }

    var subtitle: String {
        switch self {
        case .execution: return "flow · commit · nest"
        case .operatorClosure: return "U1 closure · U2 recurrence"
        case .store: return "sealed · replayable"
        case .bridge: return "dyadic ↔ triadic"
        }
    }
}

struct RootView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var workspace: Workspace = .operatorClosure

    var body: some View {
        NavigationSplitView {
            SidebarView(selection: $workspace)
                .navigationSplitViewColumnWidth(min: 216, ideal: 232, max: 280)
        } content: {
            SourceRailView()
                .navigationSplitViewColumnWidth(min: 220, ideal: 260, max: 340)
        } detail: {
            Group {
                switch workspace {
                case .execution: ExecutionView()
                case .operatorClosure: OperatorView()
                case .store: StoreView()
                case .bridge: BridgeView()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Mobius.Substrate.night)
        }
        .background(Mobius.Substrate.abyss)
        .task { await toolchain.bootstrap() }
        .fileImporter(
            isPresented: Binding(
                get: { toolchain.isRepositoryPickerPresented },
                set: { toolchain.isRepositoryPickerPresented = $0 }
            ),
            allowedContentTypes: [.folder],
            allowsMultipleSelection: false
        ) { result in
            guard case let .success(urls) = result, let url = urls.first else { return }
            Task { await toolchain.chooseRepository(url) }
        }
    }
}

struct SidebarView: View {
    @Binding var selection: Workspace
    @EnvironmentObject private var toolchain: ToolchainService

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            MarkHeader()
                .padding(.horizontal, Mobius.Space.md)
                .padding(.top, Mobius.Space.xl)
                .padding(.bottom, Mobius.Space.lg)

            ForEach(Workspace.allCases) { item in
                Button {
                    selection = item
                } label: {
                    HStack(spacing: Mobius.Space.sm + 2) {
                        Image(systemName: item.symbol)
                            .font(.system(size: 14, weight: .medium))
                            .frame(width: 20)
                            .foregroundStyle(selection == item ? Mobius.Resolve.cyan : Mobius.Phase.indigo)
                        VStack(alignment: .leading, spacing: 2) {
                            Text(item.rawValue)
                                .font(.system(size: 13, weight: .semibold))
                                .foregroundStyle(selection == item ? Mobius.Hinge.paper : Mobius.Hinge.paper.opacity(0.78))
                            Text(item.subtitle)
                                .font(.system(size: 11.5))
                                .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
                        }
                        Spacer(minLength: 0)
                    }
                    .padding(.horizontal, Mobius.Space.sm + 2)
                    .padding(.vertical, Mobius.Space.sm + 1)
                    .background(
                        RoundedRectangle(cornerRadius: Mobius.Radius.sm, style: .continuous)
                            .fill(selection == item ? Mobius.Substrate.raised : .clear)
                    )
                }
                .buttonStyle(.plain)
                .padding(.horizontal, Mobius.Space.sm)
                .accessibilityLabel("\(item.rawValue), \(item.subtitle)")
            }

            Spacer()
            RuntimeStatusFooter()
                .padding(Mobius.Space.md)
        }
        .background(Mobius.Substrate.abyss)
    }
}

struct MarkHeader: View {
    var body: some View {
        HStack(spacing: Mobius.Space.sm + 2) {
            DoubleCoverView(turns: 2, holonomy: Mobius.Geometry.holonomyRadians, animated: false)
                .frame(width: 40, height: 28)
                .rotationEffect(Mobius.Geometry.identityTilt)
            VStack(alignment: .leading, spacing: 1) {
                Text("CDC Studio")
                    .font(.system(size: 15, weight: .semibold))
                    .foregroundStyle(Mobius.Hinge.paper)
                Text("BiDi Coherence-Delta Calculus")
                    .font(.system(size: 11.5, weight: .medium))
                    .foregroundStyle(Mobius.Phase.indigo)
            }
        }
    }
}

struct RuntimeStatusFooter: View {
    @EnvironmentObject private var toolchain: ToolchainService

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.xs) {
            HStack(spacing: Mobius.Space.xs + 2) {
                Image(systemName: statusSymbol)
                    .font(.system(size: 11, weight: .bold))
                    .foregroundStyle(statusTint)
                Text(statusText)
                    .font(.system(size: 11.5, weight: .semibold))
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.82))
            }
            Text(toolchain.repositoryURL?.lastPathComponent ?? "no repository")
                .addressFont(11)
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
                .lineLimit(1)
                .truncationMode(.head)
        }
        .accessibilityElement(children: .combine)
        .accessibilityLabel("Runtime status, \(statusText), repository \(toolchain.repositoryURL?.path ?? "not selected")")
    }

    private var statusText: String {
        switch toolchain.runtimeState {
        case let .ready(identity): return identity.displayName
        case .probing: return "checking runtime ABI"
        case .missing: return "runtime not built"
        case .permissionDenied: return "runtime not executable"
        case .incompatible: return "runtime incompatible"
        case .failed: return "runtime probe failed"
        case .unknown: return "runtime unavailable"
        }
    }

    private var statusSymbol: String {
        switch toolchain.runtimeState {
        case .ready: return "checkmark.circle.fill"
        case .probing: return "circle.dotted"
        case .failed, .permissionDenied, .incompatible: return "xmark.circle.fill"
        default: return "minus.circle.fill"
        }
    }

    private var statusTint: Color {
        switch toolchain.runtimeState {
        case .ready: return Mobius.Resolve.cyan
        case .failed, .permissionDenied, .incompatible: return Mobius.Ember.core
        default: return Mobius.Phase.indigo
        }
    }
}

import SwiftUI

@main
struct CDCStudioApp: App {
    @StateObject private var toolchain = ToolchainService(
        repositoryURL: ProcessInfo.processInfo.environment["CDC_REPOSITORY"].map(URL.init(fileURLWithPath:))
    )

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
        }
    }
}

/// Sidebar destinations. Ordered as the calculus itself runs: what executed,
/// what it decided, what it durably kept, and how it is addressed.
enum Workspace: String, CaseIterable, Identifiable {
    case execution = "Execution"
    case operatorClosure = "Universal Operator"
    case store = "Store"
    case bridge = "Bridge"

    var id: String { rawValue }

    var symbol: String {
        switch self {
        case .execution:       return "play.circle"
        case .operatorClosure: return "infinity"
        case .store:           return "cylinder.split.1x2"
        case .bridge:          return "grid"
        }
    }

    var subtitle: String {
        switch self {
        case .execution:       return "flow · commit · nest"
        case .operatorClosure: return "one turn inverts · two restore"
        case .store:           return "sealed · replayable"
        case .bridge:          return "dyadic ↔ triadic"
        }
    }
}

struct RootView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var workspace: Workspace = .execution

    var body: some View {
        NavigationSplitView {
            SidebarView(selection: $workspace)
                .navigationSplitViewColumnWidth(min: 216, ideal: 232, max: 280)
        } detail: {
            Group {
                switch workspace {
                case .execution:       ExecutionView()
                case .operatorClosure: OperatorView()
                case .store:           StoreView()
                case .bridge:          BridgeView()
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
            .background(Mobius.Substrate.night)
        }
        .background(Mobius.Substrate.abyss)
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
                        VStack(alignment: .leading, spacing: 1) {
                            Text(item.rawValue)
                                .font(.system(size: 13, weight: .semibold))
                                .foregroundStyle(selection == item ? Mobius.Hinge.paper : Mobius.Hinge.paper.opacity(0.72))
                            Text(item.subtitle)
                                .font(.system(size: 10))
                                .foregroundStyle(Mobius.Hinge.paper.opacity(0.38))
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
            }

            Spacer()
            RuntimeStatusFooter()
                .padding(Mobius.Space.md)
        }
        .background(Mobius.Substrate.abyss)
    }
}

/// The mark: the closed band, drawn once, at the identity tilt. It is the same
/// geometry the operator watches elsewhere in the app — one body, many
/// projections.
struct MarkHeader: View {
    var body: some View {
        HStack(spacing: Mobius.Space.sm + 2) {
            DoubleCoverView(turns: 2, holonomy: Mobius.Geometry.holonomyRadians, animated: false)
                .frame(width: 40, height: 28)
                .rotationEffect(Mobius.Geometry.identityTilt)
            VStack(alignment: .leading, spacing: 0) {
                Text("CDC Studio")
                    .font(.system(size: 15, weight: .semibold))
                    .foregroundStyle(Mobius.Hinge.paper)
                Text("BiDi Coherence-Delta Calculus")
                    .font(.system(size: 9.5, weight: .medium))
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
                Circle()
                    .fill(toolchain.binaryAvailable ? Mobius.Resolve.cyan : Mobius.Phase.indigo)
                    .frame(width: 6, height: 6)
                Text(toolchain.binaryAvailable ? "runtime linked" : "runtime not built")
                    .font(.system(size: 10, weight: .medium))
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.6))
            }
            Text(toolchain.repositoryURL.lastPathComponent)
                .addressFont(9.5)
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.32))
                .lineLimit(1)
                .truncationMode(.head)
        }
    }
}

import SwiftUI

struct ExecutionView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var lastRun: ToolchainService.Invocation?
    @State private var tally: TestTally?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                header
                controls

                if let tally { TallyStrip(tally: tally) }
                if let lastRun { OutputPanel(invocation: lastRun) }

                if toolchain.selectedSource == nil {
                    HonestDetailState(
                        symbol: "doc.badge.plus",
                        title: "Select a source",
                        detail: "Execution is bound to the source rail. No command will run without an explicit source.",
                        actionTitle: nil,
                        action: nil
                    )
                } else if !toolchain.runtimeState.isReady {
                    HonestDetailState(
                        symbol: "terminal",
                        title: "Runtime unavailable",
                        detail: runtimeDetail,
                        actionTitle: "Refresh Runtime",
                        action: { Task { await toolchain.refreshRepository() } }
                    )
                }
            }
            .padding(Mobius.Space.xl)
            .frame(maxWidth: 1050, alignment: .leading)
        }
        .onChange(of: toolchain.contextGeneration) { _, _ in
            lastRun = nil
            tally = nil
        }
    }

    private var header: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.xs) {
            Text(toolchain.selectedSource?.url.lastPathComponent ?? "No source selected")
                .font(.system(size: 28, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
                .tracking(-0.4)
            Text(toolchain.selectedSource?.relativePath ?? "One parse · one live state · every declared stage family")
                .addressFont(11)
                .foregroundStyle(Mobius.Phase.indigo)
                .textSelection(.enabled)
        }
    }

    private var controls: some View {
        ViewThatFits(in: .horizontal) {
            HStack(spacing: Mobius.Space.md) { actionControls; Spacer() }
            VStack(alignment: .leading, spacing: Mobius.Space.sm) { actionControls }
        }
    }

    @ViewBuilder
    private var actionControls: some View {
        ActionButton(title: "Run", symbol: "play.fill", emphasis: true, disabledReason: disabledReason) {
            await execute(["run", relativePath])
        }
        ActionButton(title: "Test", symbol: "checkmark.seal", disabledReason: disabledReason) {
            guard let invocation = await toolchain.run(["test", "--gate", relativePath]) else { return }
            lastRun = invocation
            tally = invocation.stdout
                .split(separator: "\n")
                .compactMap { TestTally.parse(String($0)) }
                .first
        }
        ActionButton(title: "Verify", symbol: "doc.text.magnifyingglass", disabledReason: disabledReason) {
            await execute(["verify", "--contract", relativePath])
        }
        if toolchain.isRunning {
            Button("Cancel", systemImage: "stop.fill") { toolchain.cancelCurrentRun() }
                .buttonStyle(.bordered)
                .tint(Mobius.Ember.core)
        }
    }

    private var relativePath: String { toolchain.selectedSource?.relativePath ?? "" }

    private var disabledReason: String? {
        if toolchain.selectedSource == nil { return "Select a source first." }
        if !toolchain.runtimeState.isReady { return runtimeDetail }
        if toolchain.isRunning { return "Another runtime invocation is active." }
        return nil
    }

    private var runtimeDetail: String {
        switch toolchain.runtimeState {
        case let .missing(path): return "Build the runtime at \(path) with ./scripts/verify.sh."
        case let .permissionDenied(path): return "The runtime at \(path) is not executable."
        case let .incompatible(raw): return "CDC Studio requires ABI 1.5 and grammar 1. Runtime reported: \(raw)"
        case let .failed(message): return message
        case .probing: return "Runtime ABI compatibility is being checked."
        case .unknown: return "Choose a valid CDC repository."
        case .ready: return "Runtime ready."
        }
    }

    private func execute(_ arguments: [String]) async {
        tally = nil
        guard let invocation = await toolchain.run(arguments) else { return }
        lastRun = invocation
    }
}

struct SectionLabel: View {
    let text: String
    init(_ text: String) { self.text = text }
    var body: some View {
        Text(text.uppercased())
            .font(.system(size: 11.5, weight: .bold))
            .tracking(1.05)
            .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
    }
}

struct ActionButton: View {
    let title: String
    let symbol: String
    var emphasis = false
    var disabledReason: String? = nil
    let action: () async -> Void
    @State private var busy = false

    var body: some View {
        Button {
            Task {
                busy = true
                await action()
                busy = false
            }
        } label: {
            HStack(spacing: Mobius.Space.sm - 2) {
                Image(systemName: busy ? "circle.dotted" : symbol)
                    .font(.system(size: 11, weight: .semibold))
                Text(title).font(.system(size: 12, weight: .semibold))
            }
            .padding(.horizontal, Mobius.Space.md)
            .padding(.vertical, Mobius.Space.sm + 1)
            .background(
                RoundedRectangle(cornerRadius: Mobius.Radius.sm, style: .continuous)
                    .fill(emphasis ? Mobius.Hinge.electric.opacity(0.18) : Mobius.Substrate.raised)
            )
            .overlay(
                RoundedRectangle(cornerRadius: Mobius.Radius.sm, style: .continuous)
                    .strokeBorder(
                        emphasis ? Mobius.Hinge.electric.opacity(0.55) : Mobius.Substrate.hairline,
                        lineWidth: 1
                    )
            )
            .foregroundStyle(Mobius.Hinge.paper.opacity(disabledReason == nil ? 0.9 : 0.48))
        }
        .buttonStyle(.plain)
        .disabled(busy || disabledReason != nil)
        .help(disabledReason ?? title)
        .accessibilityHint(disabledReason ?? "Runs cdc \(title.lowercased()) for the selected source")
    }
}

struct TallyStrip: View {
    let tally: TestTally

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            ViewThatFits(in: .horizontal) {
                HStack(spacing: Mobius.Space.md) { counters; Spacer(); verdict }
                VStack(alignment: .leading, spacing: Mobius.Space.md) { counters; verdict }
            }
            if tally.unexpectedHold > 0 {
                Text("\(tally.unexpectedHold) hold(s) were not declared by their job. A hold must be expected to be admissible.")
                    .font(.system(size: 11.5))
                    .foregroundStyle(Mobius.Ember.core)
            }
        }
        .padding(Mobius.Space.md + 2)
        .panel()
    }

    @ViewBuilder private var counters: some View {
        counter("commit", tally.commit, Mobius.Resolve.cyan)
        counter("hold", tally.hold, Mobius.Phase.indigo)
        counter("nest", tally.nest, Mobius.Hinge.electric)
        counter("fail", tally.fail, Mobius.Ember.core)
    }

    private var verdict: some View {
        TernaryBadge(
            outcome: tally.gateGreen ? .commit : (tally.fail > 0 ? .violated : .held),
            reason: tally.unexpectedHold > 0 ? "undeclared-hold" : nil,
            prominent: true
        )
    }

    private func counter(_ label: String, _ value: Int, _ tint: Color) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text("\(value)")
                .font(.system(size: 24, weight: .semibold, design: .rounded))
                .foregroundStyle(tint)
            Text(label)
                .font(.system(size: 11.5, weight: .medium))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
        }
        .frame(minWidth: 56, alignment: .leading)
        .accessibilityElement(children: .combine)
    }
}

struct OutputPanel: View {
    let invocation: ToolchainService.Invocation

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            HStack {
                SectionLabel(invocation.command)
                Spacer()
                Text("exit \(invocation.exitCode) · \(invocation.elapsed.formatted(.number.precision(.fractionLength(2))))s")
                    .addressFont(11)
                    .foregroundStyle(invocation.succeeded ? Mobius.Resolve.cyan : Mobius.Ember.core)
            }
            if !invocation.stdout.isEmpty {
                outputSection("Standard output", invocation.stdout, tint: Mobius.Hinge.paper)
            }
            if !invocation.stderr.isEmpty {
                outputSection("Standard error", invocation.stderr, tint: Mobius.Ember.core)
            }
            if invocation.stdout.isEmpty && invocation.stderr.isEmpty {
                Text("Command completed without text output.")
                    .font(.system(size: 11.5))
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
            }
        }
        .padding(Mobius.Space.md + 2)
        .panel(tint: Mobius.Substrate.abyss)
    }

    private func outputSection(_ label: String, _ value: String, tint: Color) -> some View {
        VStack(alignment: .leading, spacing: Mobius.Space.xs) {
            Text(label)
                .font(.system(size: 11.5, weight: .semibold))
                .foregroundStyle(tint.opacity(0.82))
            ScrollView([.horizontal, .vertical]) {
                Text(value)
                    .addressFont(11)
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.84))
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .textSelection(.enabled)
            }
            .frame(maxHeight: 220)
        }
    }
}

struct HonestDetailState: View {
    let symbol: String
    let title: String
    let detail: String
    let actionTitle: String?
    let action: (() -> Void)?

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            Image(systemName: symbol)
                .font(.system(size: 26, weight: .light))
                .foregroundStyle(Mobius.Phase.indigo)
            Text(title)
                .font(.system(size: 18, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
            Text(detail)
                .font(.system(size: 12))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.7))
                .fixedSize(horizontal: false, vertical: true)
            if let actionTitle, let action {
                Button(actionTitle, action: action).buttonStyle(.bordered)
            }
        }
        .padding(Mobius.Space.lg)
        .panel()
        .accessibilityElement(children: .combine)
    }
}

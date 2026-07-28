import SwiftUI

/// Execution: run a source through the fused executor and read the typed
/// outcome. The counts are never merged into one number — commit, hold, nest
/// and fail each keep their own column, because a held result is information,
/// not a near-miss.
struct ExecutionView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var selected: URL?
    @State private var lastRun: ToolchainService.Invocation?
    @State private var tally: TestTally?

    var body: some View {
        HSplitView {
            sourceList
                .frame(minWidth: 220, idealWidth: 250, maxWidth: 320)
            detail
                .frame(minWidth: 520)
        }
        .onAppear { selected = selected ?? toolchain.sources().first }
    }

    private var sourceList: some View {
        VStack(alignment: .leading, spacing: 0) {
            SectionLabel("Sources")
                .padding(.horizontal, Mobius.Space.md)
                .padding(.top, Mobius.Space.lg)
                .padding(.bottom, Mobius.Space.sm)

            ScrollView {
                LazyVStack(alignment: .leading, spacing: 1) {
                    ForEach(toolchain.sources(), id: \.self) { url in
                        Button {
                            selected = url
                        } label: {
                            HStack(spacing: Mobius.Space.sm) {
                                Text(url.deletingPathExtension().lastPathComponent)
                                    .addressFont(11.5)
                                    .foregroundStyle(selected == url ? Mobius.Hinge.paper : Mobius.Hinge.paper.opacity(0.62))
                                Spacer(minLength: 0)
                            }
                            .padding(.horizontal, Mobius.Space.sm + 2)
                            .padding(.vertical, Mobius.Space.sm - 1)
                            .background(
                                RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                                    .fill(selected == url ? Mobius.Substrate.raised : .clear)
                            )
                        }
                        .buttonStyle(.plain)
                    }
                }
                .padding(.horizontal, Mobius.Space.sm)
            }
        }
        .background(Mobius.Substrate.night)
    }

    private var detail: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                header

                HStack(spacing: Mobius.Space.md) {
                    ActionButton(title: "Run", symbol: "play.fill", emphasis: true) {
                        await execute(["run", relativePath])
                    }
                    ActionButton(title: "Test", symbol: "checkmark.seal") {
                        let invocation = await toolchain.run(["test", "--gate", relativePath])
                        lastRun = invocation
                        tally = invocation.stdout
                            .split(separator: "\n")
                            .compactMap { TestTally.parse(String($0)) }
                            .first
                    }
                    ActionButton(title: "Verify", symbol: "doc.text.magnifyingglass") {
                        await execute(["verify", "--contract", relativePath])
                    }
                    Spacer()
                }

                if let tally {
                    TallyStrip(tally: tally)
                }

                if let lastRun {
                    OutputPanel(invocation: lastRun)
                }
            }
            .padding(Mobius.Space.xl)
        }
    }

    private var relativePath: String {
        selected?.lastPathComponent ?? ""
    }

    private var header: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.xs) {
            Text(selected?.lastPathComponent ?? "No source selected")
                .font(.system(size: 26, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
                .tracking(-0.4)
            Text("One parse · one live state · every declared stage family")
                .font(.system(size: 12))
                .foregroundStyle(Mobius.Phase.indigo)
        }
    }

    private func execute(_ arguments: [String]) async {
        tally = nil
        lastRun = await toolchain.run(arguments)
    }
}

struct SectionLabel: View {
    let text: String
    init(_ text: String) { self.text = text }
    var body: some View {
        Text(text.uppercased())
            .font(.system(size: 9.5, weight: .bold))
            .tracking(1.1)
            .foregroundStyle(Mobius.Hinge.paper.opacity(0.35))
    }
}

struct ActionButton: View {
    let title: String
    let symbol: String
    var emphasis: Bool = false
    let action: () async -> Void
    @State private var busy = false

    var body: some View {
        Button {
            Task { busy = true; await action(); busy = false }
        } label: {
            HStack(spacing: Mobius.Space.sm - 2) {
                Image(systemName: busy ? "circle.dotted" : symbol)
                    .font(.system(size: 11, weight: .semibold))
                Text(title)
                    .font(.system(size: 12, weight: .semibold))
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
            .foregroundStyle(emphasis ? Mobius.Hinge.paper : Mobius.Hinge.paper.opacity(0.8))
        }
        .buttonStyle(.plain)
        .disabled(busy)
    }
}

/// Four independent counters. Deliberately not a progress bar and not a
/// pass/fail pill: merging them would destroy the distinction the calculus
/// exists to make.
struct TallyStrip: View {
    let tally: TestTally

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            HStack(spacing: Mobius.Space.md) {
                counter("commit", tally.commit, Mobius.Resolve.cyan)
                counter("hold", tally.hold, Mobius.Phase.indigo)
                counter("nest", tally.nest, Mobius.Hinge.electric)
                counter("fail", tally.fail, Mobius.Ember.core)
                Spacer()
                TernaryBadge(
                    outcome: tally.gateGreen ? .commit : (tally.fail > 0 ? .violated : .held),
                    reason: tally.unexpectedHold > 0 ? "undeclared-hold" : nil,
                    prominent: true
                )
            }
            if tally.unexpectedHold > 0 {
                Text("\(tally.unexpectedHold) hold(s) were not declared by their job. A hold must be expected to be admissible.")
                    .font(.system(size: 11))
                    .foregroundStyle(Mobius.Ember.core.opacity(0.9))
            }
        }
        .padding(Mobius.Space.md + 2)
        .panel()
    }

    private func counter(_ label: String, _ value: Int, _ tint: Color) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text("\(value)")
                .font(.system(size: 24, weight: .semibold, design: .rounded))
                .foregroundStyle(tint)
            Text(label)
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.45))
        }
        .frame(minWidth: 56, alignment: .leading)
    }
}

struct OutputPanel: View {
    let invocation: ToolchainService.Invocation

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.sm) {
            HStack {
                SectionLabel(invocation.command)
                Spacer()
                TernaryBadge(outcome: invocation.succeeded ? .commit : .violated)
            }
            ScrollView {
                Text(invocation.stdout.isEmpty ? invocation.stderr : invocation.stdout)
                    .addressFont(11)
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.82))
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .textSelection(.enabled)
            }
            .frame(maxHeight: 280)
        }
        .padding(Mobius.Space.md + 2)
        .panel(tint: Mobius.Substrate.abyss)
    }
}

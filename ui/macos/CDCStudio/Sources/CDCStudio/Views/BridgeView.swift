import SwiftUI

struct BridgeView: View {
    enum LookupMode: String, CaseIterable, Identifiable {
        case dyadic
        case triadic
        var id: String { rawValue }
    }

    @EnvironmentObject private var toolchain: ToolchainService
    @State private var record: ReplayRecord?
    @State private var hovered: Int?
    @State private var lookupMode: LookupMode = .dyadic
    @State private var query = "101101"
    @State private var lastRun: ToolchainService.Invocation?

    private let columns = Array(repeating: GridItem(.flexible(), spacing: 6), count: 8)

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.xl) {
                VStack(alignment: .leading, spacing: Mobius.Space.xs) {
                    Text("Bridge")
                        .font(.system(size: 30, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                        .tracking(-0.5)
                    Text("2⁶ = 4³ = 64 · receipt-bound coordinate translation")
                        .font(.system(size: 12))
                        .foregroundStyle(Mobius.Phase.indigo)
                }

                ViewThatFits(in: .horizontal) {
                    HStack(spacing: Mobius.Space.md) { controls }
                    VStack(alignment: .leading, spacing: Mobius.Space.sm) { controls }
                }

                if bridgeDisabledReason != nil {
                    Text("Select bridge64.cdc, bridge512.cdc, or bridge4096.cdc to enable the real bridge verifier and lookup boundary.")
                        .font(.system(size: 11.5))
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                }

                VStack(alignment: .leading, spacing: Mobius.Space.md) {
                    HStack {
                        SectionLabel("64-cell recorded example")
                        Spacer()
                        Text("demo/replay.json")
                            .addressFont(11)
                            .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
                    }
                    LazyVGrid(columns: columns, spacing: 6) {
                        ForEach(0..<64, id: \.self) { index in cell(index) }
                    }
                }
                .padding(Mobius.Space.lg)
                .panel()

                if let bridge = record?.bridge {
                    ViewThatFits(in: .horizontal) {
                        HStack(spacing: Mobius.Space.xl) { recordedReadouts(bridge) }
                        VStack(alignment: .leading, spacing: Mobius.Space.md) { recordedReadouts(bridge) }
                    }
                    .padding(Mobius.Space.lg)
                    .panel()
                }

                if let lastRun { OutputPanel(invocation: lastRun) }
            }
            .padding(Mobius.Space.xl)
            .frame(maxWidth: 1050, alignment: .leading)
        }
        .onAppear { record = toolchain.replay() }
        .onChange(of: toolchain.contextGeneration) { _, _ in
            lastRun = nil
            record = toolchain.replay()
        }
        .onChange(of: toolchain.repositoryState) { _, _ in
            record = toolchain.replay()
        }
    }

    @ViewBuilder private var controls: some View {
        ActionButton(title: "Verify Codebook", symbol: "checkmark.shield", disabledReason: bridgeDisabledReason) {
            guard let source = toolchain.selectedSource else { return }
            guard let invocation = await toolchain.run(["bridge", "verify", source.relativePath]) else { return }
            lastRun = invocation
        }
        Picker("Lookup", selection: $lookupMode) {
            ForEach(LookupMode.allCases) { Text($0.rawValue.capitalized).tag($0) }
        }
        .pickerStyle(.segmented)
        .frame(maxWidth: 190)
        TextField(lookupMode == .dyadic ? "six bits" : "base-4 digits", text: $query)
            .textFieldStyle(.roundedBorder)
            .frame(width: 150)
            .accessibilityLabel("\(lookupMode.rawValue) lookup value")
        ActionButton(title: "Lookup", symbol: "arrow.left.arrow.right", disabledReason: lookupDisabledReason) {
            guard let source = toolchain.selectedSource else { return }
            guard let invocation = await toolchain.run(["bridge", "lookup-\(lookupMode.rawValue)", source.relativePath, query]) else { return }
            lastRun = invocation
        }
        if toolchain.isRunning {
            Button("Cancel", systemImage: "stop.fill") { toolchain.cancelCurrentRun() }
                .buttonStyle(.bordered)
        }
    }

    private var bridgeDisabledReason: String? {
        if !toolchain.runtimeState.isReady { return "An ABI-compatible native runtime is required." }
        if toolchain.isRunning { return "Another runtime invocation is active." }
        guard let source = toolchain.selectedSource else { return "Select a bridge codebook source." }
        guard ["bridge64.cdc", "bridge512.cdc", "bridge4096.cdc"].contains(source.url.lastPathComponent) else {
            return "The selected source is not a bridge codebook."
        }
        return nil
    }

    private var lookupDisabledReason: String? {
        if let bridgeDisabledReason { return bridgeDisabledReason }
        if query.isEmpty { return "Enter a lookup coordinate." }
        return nil
    }

    private var activeIndex: Int? { record.flatMap { Int($0.bridge.index) } }

    private func cell(_ index: Int) -> some View {
        let isActive = activeIndex == index
        let isHovered = hovered == index
        return Text(String(format: "%03d", triadic(index)))
            .font(.system(size: 11, weight: .semibold, design: .monospaced))
            .frame(maxWidth: .infinity)
            .frame(height: 30)
            .foregroundStyle(isActive ? Mobius.Substrate.abyss : Mobius.Hinge.paper.opacity(isHovered ? 0.96 : 0.74))
            .background(
                RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                    .fill(isActive ? Mobius.Resolve.cyan : Mobius.Substrate.raised.opacity(isHovered ? 1 : 0.55))
            )
            .overlay(
                RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                    .strokeBorder(isActive ? Mobius.Resolve.cyanLift : Mobius.Substrate.hairline, lineWidth: 1)
            )
            .onHover { hovered = $0 ? index : nil }
            .help("index \(index) · dyadic \(dyadic(index)) · triadic \(String(format: "%03d", triadic(index)))")
            .accessibilityLabel("Index \(index), dyadic \(dyadic(index)), triadic \(String(format: "%03d", triadic(index)))\(isActive ? ", recorded coordinate" : "")")
    }

    private func triadic(_ index: Int) -> Int {
        let high = (index >> 4) & 3
        let mid = (index >> 2) & 3
        let low = index & 3
        return high * 100 + mid * 10 + low
    }

    private func dyadic(_ index: Int) -> String {
        let bits = String(index, radix: 2)
        return String(repeating: "0", count: max(0, 6 - bits.count)) + bits
    }

    @ViewBuilder private func recordedReadouts(_ bridge: ReplayRecord.Bridge) -> some View {
        readout("dyadic", bridge.dyadic)
        readout("triadic", bridge.triadic)
        readout("index", bridge.index)
    }

    private func readout(_ label: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(value).addressFont(17).foregroundStyle(Mobius.Resolve.cyan)
            Text(label).font(.system(size: 11.5, weight: .medium)).foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
        }
        .accessibilityElement(children: .combine)
    }
}

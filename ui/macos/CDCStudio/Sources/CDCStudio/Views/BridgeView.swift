import SwiftUI

/// The bridge codebook: 2^6 = 4^3 = 64, the exact point where the dyadic and
/// triadic closures meet. Rendered as the 64-cell field it actually is, so the
/// bijection is visible rather than asserted.
struct BridgeView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var record: ReplayRecord?
    @State private var hovered: Int?

    private let columns = Array(repeating: GridItem(.flexible(), spacing: 6), count: 8)

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.xl) {
                VStack(alignment: .leading, spacing: Mobius.Space.xs) {
                    Text("Bridge")
                        .font(.system(size: 30, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                        .tracking(-0.5)
                    Text("2⁶ = 4³ = 64 · every committed occupancy has exactly one coordinate")
                        .font(.system(size: 12))
                        .foregroundStyle(Mobius.Phase.indigo)
                }

                LazyVGrid(columns: columns, spacing: 6) {
                    ForEach(0..<64, id: \.self) { index in
                        cell(index)
                    }
                }
                .padding(Mobius.Space.lg)
                .panel()

                if let bridge = record?.bridge {
                    HStack(spacing: Mobius.Space.xl) {
                        readout("dyadic", bridge.dyadic)
                        readout("triadic", bridge.triadic)
                        readout("index", bridge.index)
                    }
                    .padding(Mobius.Space.lg)
                    .panel()
                }
            }
            .padding(Mobius.Space.xl)
        }
        .onAppear { record = toolchain.replay() }
    }

    private var activeIndex: Int? {
        record.flatMap { Int($0.bridge.index) }
    }

    private func cell(_ index: Int) -> some View {
        let isActive = activeIndex == index
        let isHovered = hovered == index
        return VStack(spacing: 1) {
            Text(String(format: "%03d", triadic(index)))
                .font(.system(size: 9, weight: .semibold, design: .monospaced))
        }
        .frame(maxWidth: .infinity)
        .frame(height: 30)
        .foregroundStyle(isActive ? Mobius.Substrate.abyss : Mobius.Hinge.paper.opacity(isHovered ? 0.9 : 0.4))
        .background(
            RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                .fill(isActive
                      ? Mobius.Resolve.cyan
                      : Mobius.Substrate.raised.opacity(isHovered ? 1 : 0.55))
        )
        .overlay(
            RoundedRectangle(cornerRadius: Mobius.Radius.sm - 2, style: .continuous)
                .strokeBorder(
                    isActive ? Mobius.Resolve.cyanLift : Mobius.Substrate.hairline,
                    lineWidth: 1
                )
        )
        .onHover { hovered = $0 ? index : nil }
        .help("index \(index) · dyadic \(dyadic(index)) · triadic \(String(format: "%03d", triadic(index)))")
    }

    /// Base-4 rendering of the index: three digits, the triadic coordinate.
    private func triadic(_ index: Int) -> Int {
        let high = (index >> 4) & 3
        let mid = (index >> 2) & 3
        let low = index & 3
        return high * 100 + mid * 10 + low
    }

    /// Six-bit dyadic rendering, zero-padded on the left.
    private func dyadic(_ index: Int) -> String {
        let bits = String(index, radix: 2)
        return String(repeating: "0", count: max(0, 6 - bits.count)) + bits
    }

    private func readout(_ label: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(value)
                .addressFont(17)
                .foregroundStyle(Mobius.Resolve.cyan)
            Text(label)
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.45))
        }
    }
}

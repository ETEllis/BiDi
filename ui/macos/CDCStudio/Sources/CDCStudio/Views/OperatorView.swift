import SwiftUI

/// The Universal Operator: the app's centerpiece. Six acceptance conditions
/// must hold together, and the enactment only happens when what the frame
/// *recorded* equals what the council *decided*. That equality is the whole
/// point, so it is stated as a sentence, not buried in a table.
struct OperatorView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var record: ReplayRecord?

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.xl) {
                if let universal = record?.universal {
                    header(universal)
                    HStack(alignment: .top, spacing: Mobius.Space.xl) {
                        cover(universal)
                        VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                            closureStatement(universal)
                            cones(universal)
                        }
                    }
                } else {
                    empty
                }
            }
            .padding(Mobius.Space.xl)
        }
        .onAppear { record = toolchain.replay() }
    }

    private func header(_ universal: ReplayRecord.Universal) -> some View {
        VStack(alignment: .leading, spacing: Mobius.Space.xs) {
            Text(universal.operator)
                .font(.system(size: 34, weight: .semibold, design: .rounded))
                .foregroundStyle(Mobius.Hinge.paper)
                .tracking(-0.6)
            Text("frame \(universal.frame) · one turn inverts the sheet, two turns restore it")
                .font(.system(size: 12))
                .foregroundStyle(Mobius.Phase.indigo)
        }
    }

    private func cover(_ universal: ReplayRecord.Universal) -> some View {
        VStack(spacing: Mobius.Space.md) {
            DoubleCoverView(turns: 2, holonomy: universal.holonomyValue)
                .frame(width: 300, height: 220)
            DoubleCoverLegend(
                halfSheet: universal.halfSheet,
                fullSheet: universal.fullSheet,
                winding: universal.windingRestored ? 2 : 1,
                holonomy: universal.holonomy
            )
            .frame(width: 260)
        }
        .padding(Mobius.Space.lg)
        .panel()
    }

    /// The acceptance condition in plain language. If the two coordinates
    /// disagree, nothing is enacted — and the interface says exactly that.
    private func closureStatement(_ universal: ReplayRecord.Universal) -> some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            SectionLabel("Closure")
            HStack(spacing: Mobius.Space.md) {
                coordinate("recorded", universal.recordCoordinate)
                Image(systemName: universal.coordinatesAgree ? "equal.circle.fill" : "xmark.circle.fill")
                    .font(.system(size: 18))
                    .foregroundStyle(universal.coordinatesAgree ? Mobius.Resolve.cyan : Mobius.Ember.core)
                coordinate("decided", universal.decisionCoordinate)
            }
            Text(universal.coordinatesAgree
                 ? "What the organism recorded is what it decided. The coordinate is enacted."
                 : "The record and the decision disagree. Nothing is enacted.")
                .font(.system(size: 12))
                .foregroundStyle(universal.coordinatesAgree
                                 ? Mobius.Hinge.paper.opacity(0.75)
                                 : Mobius.Ember.core)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(Mobius.Space.lg)
        .panel()
    }

    private func coordinate(_ label: String, _ value: String) -> some View {
        VStack(alignment: .leading, spacing: Mobius.Space.xs) {
            Text(label)
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.42))
            Text(value)
                .addressFont(17)
                .foregroundStyle(Mobius.Hinge.paper)
        }
    }

    private func cones(_ universal: ReplayRecord.Universal) -> some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            SectionLabel("Reciprocal cones")
            HStack(spacing: Mobius.Space.xl) {
                angle("receptive", universal.receptiveAngle, Mobius.Phase.indigo)
                angle("radiant", universal.radiantAngle, Mobius.Hinge.electric)
                angle("holonomy", universal.holonomy, Mobius.Resolve.cyan)
            }
            Text("The cones must be angularly biased and mutually reciprocal in one flow evaluation; their sum is the accumulated holonomy.")
                .font(.system(size: 11))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.5))
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(Mobius.Space.lg)
        .panel()
    }

    private func angle(_ label: String, _ value: String, _ tint: Color) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(value)
                .addressFont(15)
                .foregroundStyle(tint)
            Text(label)
                .font(.system(size: 10, weight: .medium))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.45))
        }
    }

    private var empty: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.sm) {
            Text("No replay record")
                .font(.system(size: 20, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
            Text("Run `cdc replay native_reducer.cdc native_surface.cdc framework_loop.cdc` to produce demo/replay.json.")
                .addressFont(11)
                .foregroundStyle(Mobius.Phase.indigo)
        }
        .padding(Mobius.Space.lg)
        .panel()
    }
}

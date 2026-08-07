import AppKit
import SwiftUI

struct OperatorView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    @State private var selectedStage: EvidenceStageKind = .universal

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                header
                claimCeiling
                content
                ProofRouteView()
            }
            .padding(Mobius.Space.xl)
            .frame(maxWidth: 1180, alignment: .leading)
        }
        .animation(reduceMotion ? nil : .easeOut(duration: Mobius.Motion.settle), value: toolchain.selectedOrbitID)
    }

    private var header: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            HStack(alignment: .top, spacing: Mobius.Space.md) {
                VStack(alignment: .leading, spacing: Mobius.Space.xs) {
                    Text("Universal Operator")
                        .font(.system(size: 30, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                        .tracking(-0.5)
                    Text(toolchain.selectedSource?.relativePath ?? "Select a source to establish an evidence orbit")
                        .addressFont(11)
                        .foregroundStyle(Mobius.Phase.indigo)
                        .textSelection(.enabled)
                }
                Spacer(minLength: Mobius.Space.sm)
                if case .running = toolchain.stabilityState {
                    Button("Cancel", systemImage: "stop.fill") { toolchain.cancelCurrentRun() }
                        .buttonStyle(.bordered)
                        .tint(Mobius.Ember.core)
                } else {
                    ActionButton(
                        title: "Analyze Orbit",
                        symbol: "waveform.path.ecg",
                        emphasis: true,
                        disabledReason: analyzeDisabledReason
                    ) {
                        await toolchain.analyzeSelectedSource()
                    }
                }
            }

            if receipts.count > 1 {
                Picker("Orbit", selection: orbitSelection) {
                    ForEach(receipts) { receipt in
                        Text("\(receipt.record.orbit) · \(receipt.record.status)")
                            .tag(Optional(receipt.record.orbit))
                    }
                }
                .pickerStyle(.menu)
                .frame(maxWidth: 360, alignment: .leading)
                .accessibilityHint("Selects one canonical spectrum job from this invocation")
            }
        }
    }

    @ViewBuilder
    private var claimCeiling: some View {
        let presentation = claimPresentation
        HStack(alignment: .top, spacing: Mobius.Space.md) {
            Image(systemName: presentation.symbol)
                .font(.system(size: 18, weight: .semibold))
                .foregroundStyle(presentation.tint)
                .frame(width: 26)
            VStack(alignment: .leading, spacing: Mobius.Space.xs) {
                SectionLabel("Current claim ceiling")
                Text(presentation.title)
                    .font(.system(size: 15, weight: .semibold))
                    .foregroundStyle(Mobius.Hinge.paper)
                Text(presentation.detail)
                    .font(.system(size: 12))
                    .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
                    .fixedSize(horizontal: false, vertical: true)
            }
            Spacer(minLength: 0)
        }
        .padding(Mobius.Space.lg)
        .background(
            RoundedRectangle(cornerRadius: Mobius.Radius.md, style: .continuous)
                .fill(presentation.tint.opacity(0.075))
        )
        .overlay(
            RoundedRectangle(cornerRadius: Mobius.Radius.md, style: .continuous)
                .strokeBorder(presentation.tint.opacity(0.34), lineWidth: 1)
        )
        .accessibilityElement(children: .combine)
        .accessibilityLabel("Current claim ceiling. \(presentation.title). \(presentation.detail)")
    }

    @ViewBuilder
    private var content: some View {
        switch toolchain.stabilityState {
        case .idle:
            readyState
        case let .disabled(reason):
            HonestDetailState(
                symbol: "infinity",
                title: "Analysis unavailable",
                detail: reason,
                actionTitle: toolchain.repositoryURL == nil ? "Choose Repository…" : nil,
                action: toolchain.repositoryURL == nil ? { toolchain.isRepositoryPickerPresented = true } : nil
            )
        case let .running(source):
            runningState(source)
        case .held, .analyzed:
            if let orbit = selectedReceipt.map(EvidenceOrbit.init) {
                evidenceWorkspace(orbit)
            } else {
                HonestDetailState(
                    symbol: "doc.badge.ellipsis",
                    title: "Select an orbit",
                    detail: "The runtime returned multiple records; choose one above.",
                    actionTitle: nil,
                    action: nil
                )
            }
        case let .malformed(_, failure, invocation):
            VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                HonestDetailState(
                    symbol: "doc.badge.gearshape",
                    title: "Canonical receipt rejected",
                    detail: failure.detail,
                    actionTitle: "Retry",
                    action: { Task { await toolchain.analyzeSelectedSource() } }
                )
                OutputPanel(invocation: invocation)
            }
        case let .failed(_, invocation):
            VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                HonestDetailState(
                    symbol: invocation.cancelled ? "stop.circle" : "xmark.octagon",
                    title: invocation.cancelled ? "Analysis cancelled" : "Runtime analysis failed",
                    detail: invocation.cancelled
                        ? "The active runtime process was terminated. No previous spectrum is presented as current."
                        : "The command did not produce an admissible U2 result. Inspect both output streams below.",
                    actionTitle: "Retry",
                    action: { Task { await toolchain.analyzeSelectedSource() } }
                )
                OutputPanel(invocation: invocation)
            }
        }
    }

    private var readyState: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.lg) {
            EvidenceOrbitView(
                stages: idleStages,
                selected: $selectedStage
            )
            HonestDetailState(
                symbol: "waveform.path.ecg",
                title: "Ready to traverse",
                detail: toolchain.selectedSource?.kind == .fixture
                    ? "This is an evidence fixture, not the normal product source. Analyze it to exercise a bounded acceptance or hold case."
                    : "Run the selected source through the real cdc stability boundary. U1, tangent, recurrence, and spectrum will remain separately gated.",
                actionTitle: nil,
                action: nil
            )
        }
    }

    private func runningState(_ source: SourceDocument) -> some View {
        VStack(alignment: .leading, spacing: Mobius.Space.lg) {
            EvidenceOrbitView(
                stages: [
                    EvidenceStage(kind: .source, state: .ready(source.relativePath)),
                    EvidenceStage(kind: .universal, state: .ready("executing accepted U1 path")),
                    EvidenceStage(kind: .tangent, state: .unavailable("awaiting runtime receipt")),
                    EvidenceStage(kind: .recurrence, state: .unavailable("awaiting runtime receipt")),
                    EvidenceStage(kind: .spectrum, state: .unavailable("awaiting recurrence gate"))
                ],
                selected: $selectedStage
            )
            HStack(spacing: Mobius.Space.md) {
                ProgressView().controlSize(.small)
                VStack(alignment: .leading, spacing: 2) {
                    Text("Analyzing \(source.displayName)")
                        .font(.system(size: 14, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                    Text("The command is bounded and cancellable. No result is inferred before its canonical receipt arrives.")
                        .font(.system(size: 11.5))
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                }
            }
            .padding(Mobius.Space.lg)
            .panel()
        }
    }

    private func evidenceWorkspace(_ orbit: EvidenceOrbit) -> some View {
        ViewThatFits(in: .horizontal) {
            HStack(alignment: .top, spacing: Mobius.Space.lg) {
                EvidenceOrbitView(stages: orbit.stages, selected: $selectedStage)
                    .frame(width: 260)
                EvidenceInspectorView(receipt: orbit.receipt, stage: selectedStage)
                    .frame(maxWidth: .infinity, alignment: .leading)
            }
            VStack(alignment: .leading, spacing: Mobius.Space.lg) {
                EvidenceOrbitView(stages: orbit.stages, selected: $selectedStage)
                EvidenceInspectorView(receipt: orbit.receipt, stage: selectedStage)
            }
        }
    }

    private var receipts: [CanonicalU2Receipt] {
        switch toolchain.stabilityState {
        case let .held(_, receipts, _), let .analyzed(_, receipts, _): return receipts
        default: return []
        }
    }

    private var selectedReceipt: CanonicalU2Receipt? {
        receipts.first(where: { $0.record.orbit == toolchain.selectedOrbitID }) ?? receipts.first
    }

    private var orbitSelection: Binding<String?> {
        Binding(
            get: { toolchain.selectedOrbitID },
            set: { toolchain.selectedOrbitID = $0 }
        )
    }

    private var idleStages: [EvidenceStage] {
        [
            EvidenceStage(kind: .source, state: toolchain.selectedSource.map { .ready($0.relativePath) } ?? .unavailable("Select a source")),
            EvidenceStage(kind: .universal, state: .ready("awaiting analysis")),
            EvidenceStage(kind: .tangent, state: .unavailable("awaiting U1")),
            EvidenceStage(kind: .recurrence, state: .unavailable("awaiting tangent")),
            EvidenceStage(kind: .spectrum, state: .unavailable("awaiting recurrence"))
        ]
    }

    private var analyzeDisabledReason: String? {
        if toolchain.selectedSource == nil { return "Select a source first." }
        if !toolchain.runtimeState.isReady { return "An ABI-compatible native runtime is required." }
        if toolchain.isRunning { return "Another runtime command is active." }
        return nil
    }

    private var claimPresentation: (symbol: String, tint: Color, title: String, detail: String) {
        switch toolchain.stabilityState {
        case .idle:
            return ("circle.dotted", Mobius.Phase.indigo, "Ready, not yet analyzed", "No U1 or U2 claim is current until the selected source emits a canonical receipt.")
        case let .disabled(reason):
            return ("minus.circle", Mobius.Phase.indigo, "Disabled", reason)
        case .running:
            return ("waveform.path", Mobius.Hinge.electric, "Analysis in progress", "The evidence orbit remains unresolved until the runtime returns.")
        case .held, .analyzed:
            if let orbit = selectedReceipt.map(EvidenceOrbit.init) {
                let accepted = orbit.receipt.record.accepted
                return (
                    accepted ? "checkmark.seal.fill" : "pause.circle.fill",
                    accepted ? Mobius.Resolve.cyan : Mobius.Phase.indigo,
                    accepted ? "U2 analysis admitted" : "U2 held honestly",
                    orbit.claimCeiling
                )
            }
            return ("doc.badge.ellipsis", Mobius.Phase.indigo, "Choose an orbit", "Multiple runtime records are available.")
        case let .malformed(_, failure, _):
            return ("xmark.seal", Mobius.Ember.core, "No claim admitted", failure.detail)
        case let .failed(_, invocation):
            return ("xmark.octagon", Mobius.Ember.core, "No claim admitted", invocation.cancelled ? "The analysis was cancelled." : "The runtime command failed.")
        }
    }
}

struct EvidenceOrbitView: View {
    let stages: [EvidenceStage]
    @Binding var selected: EvidenceStageKind

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            SectionLabel("Evidence orbit")
                .padding(.bottom, Mobius.Space.md)

            ForEach(Array(stages.enumerated()), id: \.element.id) { index, stage in
                Button {
                    selected = stage.kind
                } label: {
                    HStack(alignment: .top, spacing: Mobius.Space.sm + 2) {
                        VStack(spacing: 0) {
                            Circle()
                                .fill(tint(stage.state))
                                .frame(width: 9, height: 9)
                                .overlay(Circle().strokeBorder(Mobius.Hinge.paper.opacity(0.5), lineWidth: 0.5))
                            if index < stages.count - 1 {
                                Rectangle()
                                    .fill(tint(stage.state).opacity(0.34))
                                    .frame(width: 1, height: 38)
                            }
                        }
                        VStack(alignment: .leading, spacing: 2) {
                            HStack {
                                Text(stage.kind.rawValue)
                                    .font(.system(size: 12.5, weight: .semibold))
                                    .foregroundStyle(Mobius.Hinge.paper)
                                Spacer(minLength: Mobius.Space.xs)
                                Text(stage.state.label)
                                    .font(.system(size: 11, weight: .bold))
                                    .foregroundStyle(tint(stage.state))
                            }
                            Text(stage.state.detail)
                                .font(.system(size: 11.5))
                                .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                                .lineLimit(2)
                                .multilineTextAlignment(.leading)
                        }
                    }
                    .padding(.horizontal, Mobius.Space.sm)
                    .padding(.top, 2)
                    .background(
                        RoundedRectangle(cornerRadius: Mobius.Radius.sm, style: .continuous)
                            .fill(selected == stage.kind ? Mobius.Substrate.raised.opacity(0.82) : .clear)
                    )
                }
                .buttonStyle(.plain)
                .accessibilityLabel("\(stage.kind.rawValue), \(stage.state.label), \(stage.state.detail)")
            }
        }
        .padding(Mobius.Space.md)
        .panel(tint: Mobius.Substrate.abyss)
    }

    private func tint(_ state: EvidenceStageState) -> Color {
        switch state {
        case .analyzed: return Mobius.Resolve.cyan
        case .ready: return Mobius.Hinge.electric
        case .held: return Mobius.Phase.indigo
        case .unavailable: return Mobius.Hinge.paper.opacity(0.72)
        case .malformed: return Mobius.Ember.core
        }
    }
}

struct EvidenceInspectorView: View {
    let receipt: CanonicalU2Receipt
    let stage: EvidenceStageKind

    private var record: U2StabilityRecord { receipt.record }

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.lg) {
            switch stage {
            case .source: sourceInspector
            case .universal: universalInspector
            case .tangent: tangentInspector
            case .recurrence: recurrenceInspector
            case .spectrum: spectrumInspector
            }

            DisclosureGroup("Canonical raw receipt") {
                ScrollView([.horizontal, .vertical]) {
                    Text(receipt.rawJSON)
                        .addressFont(11)
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.78))
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .leading)
                }
                .frame(maxHeight: 260)
                .padding(.top, Mobius.Space.sm)
            }
            .font(.system(size: 11.5, weight: .semibold))
            .foregroundStyle(Mobius.Hinge.paper.opacity(0.82))
        }
        .padding(Mobius.Space.lg)
        .panel()
    }

    private var sourceInspector: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            inspectorHeader("Canonical source", subtitle: record.sourceDigestSurface)
            digestRow("source", record.sourceDigest)
            digestRow("manifest", record.manifestDigest ?? "not admitted")
            readoutGrid([
                ("runtime", record.runtimeIdentity),
                ("determinism", record.determinism),
                ("schema", record.schema)
            ])
        }
    }

    private var universalInspector: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            inspectorHeader("U1 closure", subtitle: "\(record.universal.id) · \(record.universal.status)")
            ViewThatFits(in: .horizontal) {
                HStack(alignment: .top, spacing: Mobius.Space.lg) {
                    coverGraphic
                    universalFacts
                }
                VStack(alignment: .leading, spacing: Mobius.Space.md) {
                    coverGraphic
                    universalFacts
                }
            }
        }
    }

    private var coverGraphic: some View {
        DoubleCoverView(
            turns: Double(record.universal.winding),
            holonomy: Double(record.universal.holonomy) ?? 0,
            animated: false
        )
        .frame(minWidth: 220, idealWidth: 280, maxWidth: 320, minHeight: 170, idealHeight: 200)
        .accessibilityLabel("U1 double cover, winding \(record.universal.winding), status \(record.universal.status), reason \(record.universal.reason)")
    }

    private var universalFacts: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            TernaryBadge(outcome: TernaryOutcome(status: record.universal.status), reason: record.universal.reason, prominent: true)
            readoutGrid([
                ("receptive", record.universal.receptiveAngle),
                ("radiant", record.universal.radiantAngle),
                ("holonomy", record.universal.holonomy),
                ("winding", "\(record.universal.winding)")
            ])
            digestRow("result", record.universal.resultDigest)
        }
    }

    private var tangentInspector: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            inspectorHeader("Executed tangent", subtitle: "\(record.dimension ?? 0) coordinates · \(record.events.count) events")
            if let coordinates = record.coordinates,
               let initial = record.initialState,
               let final = record.finalState {
                Grid(alignment: .leading, horizontalSpacing: Mobius.Space.md, verticalSpacing: Mobius.Space.sm) {
                    GridRow {
                        columnHeader("coordinate")
                        columnHeader("initial")
                        columnHeader("final")
                        columnHeader("delta")
                    }
                    ForEach(Array(coordinates.enumerated()), id: \.offset) { index, coordinate in
                        GridRow {
                            Text(coordinate.name).addressFont(11).foregroundStyle(Mobius.Hinge.paper.opacity(0.86))
                            number(initial[index])
                            number(final[index])
                            number(final[index] - initial[index])
                        }
                    }
                }
                .accessibilityElement(children: .contain)
            } else {
                unavailable("No tangent state was admitted by U1.")
            }

            if !record.events.isEmpty {
                SectionLabel("Ordered events")
                ForEach(Array(record.events.enumerated()), id: \.offset) { index, event in
                    HStack(spacing: Mobius.Space.md) {
                        Text("\(index + 1)").addressFont(11).foregroundStyle(Mobius.Resolve.cyan)
                        Text(event.kind).addressFont(11).foregroundStyle(Mobius.Hinge.paper)
                        Spacer()
                        Text("t=\(format(event.time))").addressFont(11).foregroundStyle(Mobius.Phase.indigo)
                        Text(event.transverse.map { $0 ? "transverse" : "non-transverse" } ?? "scheduled")
                            .font(.system(size: 11))
                            .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                    }
                    .accessibilityElement(children: .combine)
                }
            }
            digestRow("path tangent", record.pathTangentDigest ?? "not admitted")
        }
    }

    private var recurrenceInspector: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            inspectorHeader("Recurrence gate", subtitle: record.recurrence?.scope ?? "not admitted")
            if let recurrence = record.recurrence {
                HStack(spacing: Mobius.Space.md) {
                    Image(systemName: recurrence.verified ? "checkmark.seal.fill" : "pause.circle.fill")
                        .foregroundStyle(recurrence.verified ? Mobius.Resolve.cyan : Mobius.Phase.indigo)
                    Text(recurrence.verified ? "Return verified" : "Return held: \(record.reason)")
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                }
                readoutGrid([
                    ("kind", recurrence.kind),
                    ("scope", recurrence.scope),
                    ("residual", format(recurrence.residual)),
                    ("normalized", format(recurrence.normalizedResidual)),
                    ("absolute tol", format(recurrence.absoluteTolerance)),
                    ("relative tol", format(recurrence.relativeTolerance))
                ])
                if let restoration = recurrence.restoration {
                    Divider().overlay(Mobius.Substrate.hairline)
                    SectionLabel("Endpoint restoration")
                    Text("\(restoration.action) · \(restoration.coordinate) − \(format(restoration.displacement))")
                        .addressFont(11)
                        .foregroundStyle(Mobius.Resolve.cyan)
                    Text("Equivariance \(restoration.equivarianceWitness.verified ? "verified" : "not verified") by \(restoration.equivarianceWitness.id). Section \(restoration.sectionWitness.verified ? "verified" : "not verified") by \(restoration.sectionWitness.id).")
                        .font(.system(size: 11.5))
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
                    digestRow("Dρ", restoration.derivativeDigest)
                }
            } else {
                unavailable("The primal U1 path held before recurrence admission.")
            }
        }
    }

    private var spectrumInspector: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            inspectorHeader("Characteristic multipliers", subtitle: record.backend ?? "no backend invoked")
            if let multipliers = record.multipliers {
                HStack(spacing: Mobius.Space.md) {
                    Image(systemName: "scope")
                        .foregroundStyle(Mobius.Resolve.cyan)
                    Text(record.classification.capitalized)
                        .font(.system(size: 18, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                    Text("accepted analysis")
                        .font(.system(size: 11.5, weight: .medium))
                        .foregroundStyle(Mobius.Resolve.cyan)
                }
                MultiplierPlaneView(multipliers: multipliers)
                    .frame(height: 250)
                Grid(alignment: .leading, horizontalSpacing: Mobius.Space.md, verticalSpacing: Mobius.Space.sm) {
                    GridRow {
                        columnHeader("mode")
                        columnHeader("real")
                        columnHeader("imag")
                        columnHeader("|λ|")
                    }
                    ForEach(Array(multipliers.enumerated()), id: \.offset) { _, multiplier in
                        GridRow {
                            Text(multiplier.mode).font(.system(size: 11, weight: .semibold)).foregroundStyle(Mobius.Hinge.paper)
                            number(multiplier.real)
                            number(multiplier.imag)
                            number(multiplier.modulus)
                        }
                    }
                }
                digestRow("monodromy", record.monodromyDigest ?? "missing")
            } else {
                unavailable("No spectrum exists for this run. \(record.reason == "none" ? "Recurrence did not authorize monodromy." : record.reason)")
            }
        }
    }

    private func inspectorHeader(_ title: String, subtitle: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(title)
                .font(.system(size: 19, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
            Text(subtitle)
                .addressFont(11)
                .foregroundStyle(Mobius.Phase.indigo)
        }
    }

    private func readoutGrid(_ rows: [(String, String)]) -> some View {
        Grid(alignment: .leading, horizontalSpacing: Mobius.Space.lg, verticalSpacing: Mobius.Space.sm) {
            ForEach(Array(rows.enumerated()), id: \.offset) { _, row in
                GridRow {
                    Text(row.0)
                        .font(.system(size: 11.5, weight: .medium))
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                    Text(row.1)
                        .addressFont(11)
                        .foregroundStyle(Mobius.Hinge.paper.opacity(0.86))
                        .textSelection(.enabled)
                }
            }
        }
    }

    private func digestRow(_ label: String, _ digest: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label).font(.system(size: 11.5, weight: .medium)).foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
            Text(digest).addressFont(11).foregroundStyle(Mobius.Resolve.cyan).textSelection(.enabled)
        }
        .accessibilityElement(children: .combine)
    }

    private func columnHeader(_ text: String) -> some View {
        Text(text.uppercased())
            .font(.system(size: 11, weight: .bold))
            .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
    }

    private func number(_ value: Double) -> some View {
        Text(format(value)).addressFont(11).foregroundStyle(Mobius.Hinge.paper.opacity(0.84))
    }

    private func unavailable(_ detail: String) -> some View {
        HStack(spacing: Mobius.Space.sm) {
            Image(systemName: "minus.circle.fill").foregroundStyle(Mobius.Phase.indigo)
            Text(detail).font(.system(size: 11.5)).foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
        }
        .accessibilityElement(children: .combine)
    }

    private func format(_ value: Double) -> String { String(format: "%.6g", value) }
}

struct MultiplierPlaneView: View {
    let multipliers: [U2StabilityRecord.Multiplier]

    var body: some View {
        Canvas { context, size in
            let extent = max(1.2, multipliers.map { max(abs($0.real), abs($0.imag)) }.max() ?? 1.2)
            let center = CGPoint(x: size.width / 2, y: size.height / 2)
            let radius = min(size.width, size.height) * 0.40

            var axes = Path()
            axes.move(to: CGPoint(x: center.x - radius, y: center.y))
            axes.addLine(to: CGPoint(x: center.x + radius, y: center.y))
            axes.move(to: CGPoint(x: center.x, y: center.y - radius))
            axes.addLine(to: CGPoint(x: center.x, y: center.y + radius))
            context.stroke(axes, with: .color(Mobius.Substrate.hairline), lineWidth: 1)

            let unitRadius = radius / extent
            let unit = Path(ellipseIn: CGRect(
                x: center.x - unitRadius,
                y: center.y - unitRadius,
                width: unitRadius * 2,
                height: unitRadius * 2
            ))
            context.stroke(unit, with: .color(Mobius.Phase.indigo.opacity(0.8)), style: StrokeStyle(lineWidth: 1.2, dash: [4, 4]))

            var clusters: [String: (point: CGPoint, count: Int, mode: String)] = [:]
            for multiplier in multipliers {
                let point = CGPoint(
                    x: center.x + CGFloat(multiplier.real / extent) * radius,
                    y: center.y - CGFloat(multiplier.imag / extent) * radius
                )
                let key = "\(Int(point.x.rounded())):\(Int(point.y.rounded())):\(multiplier.mode)"
                if let existing = clusters[key] {
                    clusters[key] = (existing.point, existing.count + 1, existing.mode)
                } else {
                    clusters[key] = (point, 1, multiplier.mode)
                }
            }
            for cluster in clusters.values {
                let color = cluster.mode == "gauge" ? Mobius.Phase.indigo : Mobius.Resolve.cyan
                let dot = Path(ellipseIn: CGRect(x: cluster.point.x - 5, y: cluster.point.y - 5, width: 10, height: 10))
                context.fill(dot, with: .color(color.opacity(0.9)))
                context.stroke(dot, with: .color(Mobius.Hinge.paper.opacity(0.9)), lineWidth: 1)
                if cluster.count > 1 {
                    context.draw(
                        Text("×\(cluster.count)").font(.system(size: 11, weight: .bold)).foregroundColor(Mobius.Hinge.paper),
                        at: CGPoint(x: cluster.point.x + 16, y: cluster.point.y - 12)
                    )
                }
            }
        }
        .padding(Mobius.Space.sm)
        .background(Mobius.Substrate.abyss)
        .clipShape(RoundedRectangle(cornerRadius: Mobius.Radius.sm, style: .continuous))
        .overlay(RoundedRectangle(cornerRadius: Mobius.Radius.sm).strokeBorder(Mobius.Substrate.hairline, lineWidth: 1))
        .accessibilityElement(children: .ignore)
        .accessibilityLabel("Complex multiplier plane with unit circle")
        .accessibilityValue(multiplierSummary)
    }

    private var multiplierSummary: String {
        multipliers.enumerated().map { index, multiplier in
            "Multiplier \(index + 1), mode \(multiplier.mode), real \(multiplier.real), imaginary \(multiplier.imag), modulus \(multiplier.modulus)"
        }.joined(separator: ". ")
    }
}

struct ProofRouteView: View {
    @EnvironmentObject private var toolchain: ToolchainService

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            SectionLabel("Evidence routes")
            Text("Source → runtime → receipt → semantics → paper")
                .font(.system(size: 12, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
            ViewThatFits(in: .horizontal) {
                HStack(spacing: Mobius.Space.sm) { routeButtons }
                VStack(alignment: .leading, spacing: Mobius.Space.sm) { routeButtons }
            }
        }
        .padding(Mobius.Space.lg)
        .panel(tint: Mobius.Substrate.abyss)
    }

    @ViewBuilder
    private var routeButtons: some View {
        route("Source", symbol: "doc.text", url: toolchain.selectedSource?.url, reveal: true)
        route("README", symbol: "book", url: toolchain.snapshot?.artifact("README.md"))
        route("U2 semantics", symbol: "function", url: toolchain.snapshot?.artifact("docs/u2/U2_SEMANTICS.md"))
        route("Paper source", symbol: "doc.richtext", url: toolchain.snapshot?.artifact("paper/arxiv/main.tex"))
        route("Compiled paper", symbol: "doc.fill", url: toolchain.snapshot?.artifact("paper/arxiv/main.pdf"))
    }

    private func route(_ title: String, symbol: String, url: URL?, reveal: Bool = false) -> some View {
        Button {
            guard let url else { return }
            if reveal {
                NSWorkspace.shared.activateFileViewerSelecting([url])
            } else {
                NSWorkspace.shared.open(url)
            }
        } label: {
            Label(title, systemImage: symbol)
                .font(.system(size: 11.5, weight: .semibold))
        }
        .buttonStyle(.bordered)
        .disabled(url == nil)
        .help(url?.path ?? "Artifact is not present in this checkout")
    }
}

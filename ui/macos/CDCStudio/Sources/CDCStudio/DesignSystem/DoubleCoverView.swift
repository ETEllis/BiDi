import SwiftUI

/// The product's signature visual: the Universal Operator's lifted double
/// cover. One turn returns the projection with the sheet **inverted**; only the
/// second turn restores it. This is not decoration and it is not a spinner —
/// the drawing is the acceptance condition the runtime actually checks, so the
/// operator can see at a glance which turn the frame is on.
///
/// Form law: lean in → invert → resolve.
struct DoubleCoverView: View {
    /// 0 … 2 — turns completed. 1.0 is the inverted-sheet state, 2.0 restored.
    var turns: Double
    /// Accumulated holonomy in radians (receptive + radiant angle).
    var holonomy: Double
    var animated: Bool = true

    @Environment(\.accessibilityReduceMotion) private var reduceMotion
    @State private var phase: Double = 0

    private var effectiveTurns: Double {
        (animated && !reduceMotion) ? phase : turns
    }

    private var sheetInverted: Bool {
        let whole = Int(effectiveTurns.rounded(.down))
        return whole % 2 == 1
    }

    var body: some View {
        TimelineView(.animation(minimumInterval: 1.0 / 60.0, paused: !animated || reduceMotion)) { _ in
            Canvas { context, size in
                draw(in: &context, size: size)
            }
        }
        .onAppear {
            guard animated, !reduceMotion else { return }
            withAnimation(.easeInOut(duration: Mobius.Motion.restore).repeatForever(autoreverses: false)) {
                phase = 2
            }
        }
        .accessibilityElement()
        .accessibilityLabel(
            "Double cover, \(String(format: "%.2f", effectiveTurns)) turns, sheet \(sheetInverted ? "inverted" : "restored"), holonomy \(String(format: "%.3f", holonomy)) radians"
        )
    }

    private func draw(in context: inout GraphicsContext, size: CGSize) {
        let center = CGPoint(x: size.width / 2, y: size.height / 2)
        let radius = min(size.width, size.height) * 0.34
        let turnAngle = effectiveTurns * 2 * .pi

        // The band: a Möbius-like ribbon sampled as two counter-phased edges.
        // The visible crossing is where the sheet changes hands.
        var upper = Path()
        var lower = Path()
        let samples = 220
        for step in 0...samples {
            let t = Double(step) / Double(samples)
            let theta = t * 2 * .pi
            // half-twist: the band's width collapses and re-opens once per turn
            let twist = cos(theta / 2 + turnAngle / 2)
            let width = radius * 0.30 * twist
            let x = center.x + CGFloat(cos(theta) * radius)
            let yBase = center.y + CGFloat(sin(theta) * radius * 0.42)
            let upperPoint = CGPoint(x: x, y: yBase - CGFloat(width))
            let lowerPoint = CGPoint(x: x, y: yBase + CGFloat(width))
            if step == 0 {
                upper.move(to: upperPoint)
                lower.move(to: lowerPoint)
            } else {
                upper.addLine(to: upperPoint)
                lower.addLine(to: lowerPoint)
            }
        }

        let leading = sheetInverted ? Mobius.Phase.indigo : Mobius.Resolve.cyan
        let trailing = sheetInverted ? Mobius.Hinge.electric : Mobius.Hinge.paper

        context.stroke(
            upper,
            with: .linearGradient(
                Gradient(colors: [leading.opacity(0.95), trailing.opacity(0.35)]),
                startPoint: CGPoint(x: 0, y: 0),
                endPoint: CGPoint(x: size.width, y: size.height)
            ),
            style: StrokeStyle(lineWidth: 2, lineCap: .round)
        )
        context.stroke(
            lower,
            with: .linearGradient(
                Gradient(colors: [trailing.opacity(0.35), leading.opacity(0.95)]),
                startPoint: CGPoint(x: 0, y: 0),
                endPoint: CGPoint(x: size.width, y: size.height)
            ),
            style: StrokeStyle(lineWidth: 2, lineCap: .round)
        )

        // The traveling incident: where the operator currently is on the band.
        let markerTheta = turnAngle.truncatingRemainder(dividingBy: 2 * .pi)
        let marker = CGPoint(
            x: center.x + CGFloat(cos(markerTheta) * radius),
            y: center.y + CGFloat(sin(markerTheta) * radius * 0.42)
        )
        let dot = Path(ellipseIn: CGRect(x: marker.x - 4, y: marker.y - 4, width: 8, height: 8))
        context.fill(dot, with: .color(sheetInverted ? Mobius.Phase.indigo : Mobius.Resolve.cyan))
        context.stroke(dot, with: .color(Mobius.Hinge.paper.opacity(0.8)), lineWidth: 1)

        // Holonomy arc — the accumulated angular debt, drawn at the identity tilt.
        var arc = Path()
        arc.addArc(
            center: center,
            radius: radius * 1.28,
            startAngle: .degrees(-90),
            endAngle: .degrees(-90 + holonomy * 180 / .pi * 8),
            clockwise: false
        )
        context.stroke(
            arc,
            with: .color(Mobius.Resolve.cyanLift.opacity(0.7)),
            style: StrokeStyle(lineWidth: 1.5, lineCap: .round)
        )
    }
}

/// Compact readout that pairs with the drawing: the four acceptance facts the
/// runtime checks, stated plainly.
struct DoubleCoverLegend: View {
    let halfSheet: String
    let fullSheet: String
    let winding: Int
    let holonomy: String

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.sm) {
            row("one turn", halfSheet, restored: halfSheet == "restored")
            row("two turns", fullSheet, restored: fullSheet == "restored")
            row("winding", "\(winding)", restored: winding == 2)
            row("holonomy", holonomy, restored: true)
        }
    }

    private func row(_ label: String, _ value: String, restored: Bool) -> some View {
        HStack {
            Text(label)
                .font(.system(size: 11.5, weight: .medium))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
            Spacer(minLength: Mobius.Space.md)
            Text(value)
                .addressFont(11)
                .foregroundStyle(restored ? Mobius.Resolve.cyan : Mobius.Phase.indigo)
        }
    }
}

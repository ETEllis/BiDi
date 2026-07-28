import SwiftUI

/// Möbi𝒰s design tokens, mirrored verbatim from `ui/design/mobius-tokens.json`.
/// `scripts/verify.sh` fails if these values drift from the canonical JSON, so
/// the app cannot quietly diverge from the shipped identity system.
enum Mobius {

    // MARK: Substrate — cobalt-indigo night the whole product sits on.
    enum Substrate {
        static let abyss    = Color(hex: 0x040d1d)
        static let night    = Color(hex: 0x07152c)
        static let deep     = Color(hex: 0x0b1e3d)
        static let raised   = Color(hex: 0x122a52)
        static let hairline = Color(hex: 0x1d356b)
    }

    // MARK: Phase — the bounded subjective frame; also equilibrium.
    enum Phase {
        static let indigo     = Color(hex: 0x707ddd)
        static let indigoDeep = Color(hex: 0x394db8)
        static let violetInk  = Color(hex: 0x27316f)
    }

    // MARK: Hinge — the inversion point: white/electric-blue.
    enum Hinge {
        static let electric     = Color(hex: 0x137dff)
        static let electricDeep = Color(hex: 0x0c6fd8)
        static let paper        = Color(hex: 0xf8fbff)
    }

    // MARK: Resolve — the returned relation.
    enum Resolve {
        static let cyan     = Color(hex: 0x31e2e7)
        static let cyanLift = Color(hex: 0x43f0ec)
        static let teal     = Color(hex: 0x008d9b)
    }

    /// The only warm token in the system. Reserved for a violated invariant.
    /// Never use it for emphasis, branding, or ordinary alerts.
    enum Ember {
        static let core = Color(hex: 0xff6b57)
    }

    // MARK: Geometry — every rotational accent uses the identity invariant.
    enum Geometry {
        static let holonomyRadians = 0.125
        /// 7.16197° is the wordmark's shipped invariant, gated in the identity
        /// asset contract. It is the product's only tilt angle.
        static let identityTilt = Angle(degrees: 7.16197)
        static let turn = Angle(degrees: 360)
        static let restore = Angle(degrees: 720)
    }

    enum Radius {
        static let sm: CGFloat = 6
        static let md: CGFloat = 10
        static let lg: CGFloat = 16
    }

    enum Space {
        static let xs: CGFloat = 4
        static let sm: CGFloat = 8
        static let md: CGFloat = 16
        static let lg: CGFloat = 24
        static let xl: CGFloat = 40
        static let xxl: CGFloat = 64
    }

    enum Motion {
        static let restore: Double = 2.4
        static let settle: Double = 0.42
    }
}

extension Color {
    init(hex: UInt32) {
        self.init(
            .sRGB,
            red:   Double((hex >> 16) & 0xff) / 255.0,
            green: Double((hex >> 8) & 0xff) / 255.0,
            blue:  Double(hex & 0xff) / 255.0,
            opacity: 1.0
        )
    }
}

// MARK: - Shared surfaces

/// A raised panel: hairline edge, no drop shadow. Depth comes from value, not
/// from blur — the substrate is a night sky, not a paper stack.
struct PanelBackground: ViewModifier {
    var tint: Color = Mobius.Substrate.deep
    func body(content: Content) -> some View {
        content
            .background(
                RoundedRectangle(cornerRadius: Mobius.Radius.md, style: .continuous)
                    .fill(tint)
            )
            .overlay(
                RoundedRectangle(cornerRadius: Mobius.Radius.md, style: .continuous)
                    .strokeBorder(Mobius.Substrate.hairline, lineWidth: 1)
            )
    }
}

extension View {
    func panel(tint: Color = Mobius.Substrate.deep) -> some View {
        modifier(PanelBackground(tint: tint))
    }

    /// Monospaced treatment for anything that is an address rather than prose:
    /// trits, coordinates, digests.
    func addressFont(_ size: CGFloat = 13) -> some View {
        font(.system(size: size, weight: .medium, design: .monospaced))
    }
}

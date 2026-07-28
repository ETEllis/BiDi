// swift-tools-version: 5.9
import PackageDescription

// CDC Studio — the native macOS operator surface for the cdc toolchain.
// No third-party dependencies: the product's own runtime is the only engine,
// and the design system is vendored from ui/design/mobius-tokens.json.
let package = Package(
    name: "CDCStudio",
    platforms: [.macOS(.v14)],
    products: [
        .executable(name: "CDCStudio", targets: ["CDCStudio"])
    ],
    targets: [
        .executableTarget(
            name: "CDCStudio",
            path: "Sources/CDCStudio"
        )
    ]
)

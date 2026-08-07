import SwiftUI

struct SourceRailView: View {
    @EnvironmentObject private var toolchain: ToolchainService
    @State private var search = ""
    @State private var showFixtures = false

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            repositoryHeader
            Divider().overlay(Mobius.Substrate.hairline)
            stateContent
        }
        .background(Mobius.Substrate.night)
    }

    private var repositoryHeader: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.sm) {
            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: 2) {
                    SectionLabel("Repository")
                    Text(toolchain.repositoryURL?.lastPathComponent ?? "Not selected")
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundStyle(Mobius.Hinge.paper)
                        .lineLimit(1)
                    if let path = toolchain.repositoryURL?.path {
                        Text(path)
                            .addressFont(11.5)
                            .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                            .lineLimit(2)
                            .truncationMode(.head)
                    }
                }
                Spacer(minLength: Mobius.Space.sm)
                Button {
                    toolchain.isRepositoryPickerPresented = true
                } label: {
                    Image(systemName: "folder.badge.gearshape")
                }
                .buttonStyle(.borderless)
                .help("Choose Repository…")
                .accessibilityLabel("Choose Repository")
            }

            if toolchain.snapshot != nil {
                TextField("Filter sources", text: $search)
                    .textFieldStyle(.roundedBorder)
                    .accessibilityLabel("Filter CDC sources")
            }
        }
        .padding(.horizontal, Mobius.Space.md)
        .padding(.top, Mobius.Space.xl)
        .padding(.bottom, Mobius.Space.md)
    }

    @ViewBuilder
    private var stateContent: some View {
        switch toolchain.repositoryState {
        case .locating, .loading:
            HonestRailState(
                symbol: "folder.badge.questionmark",
                title: "Locating repository",
                detail: "Validating source markers and runtime boundaries.",
                actionTitle: nil,
                action: nil
            )
        case .unconfigured:
            HonestRailState(
                symbol: "folder.badge.plus",
                title: "Choose a CDC repository",
                detail: "CDC Studio does not assume your home directory is a checkout.",
                actionTitle: "Choose Repository…",
                action: { toolchain.isRepositoryPickerPresented = true }
            )
        case let .unavailable(problem):
            HonestRailState(
                symbol: "exclamationmark.folder",
                title: problem.title,
                detail: problem.detail,
                actionTitle: "Choose Again…",
                action: { toolchain.isRepositoryPickerPresented = true }
            )
        case .empty:
            HonestRailState(
                symbol: "doc",
                title: "No CDC sources",
                detail: "This repository passed identity checks but contains no readable .cdc sources.",
                actionTitle: "Refresh",
                action: { Task { await toolchain.refreshRepository() } }
            )
        case .ready:
            sourceList
        }
    }

    private var sourceList: some View {
        ScrollView {
            LazyVStack(alignment: .leading, spacing: 2) {
                SectionLabel("Project sources")
                    .padding(.horizontal, Mobius.Space.md)
                    .padding(.top, Mobius.Space.md)
                    .padding(.bottom, Mobius.Space.xs)

                ForEach(filtered(toolchain.projectSources)) { source in
                    sourceButton(source)
                }

                if !toolchain.fixtureSources.isEmpty {
                    Button {
                        withAnimation(.easeOut(duration: Mobius.Motion.settle)) {
                            showFixtures.toggle()
                        }
                    } label: {
                        HStack {
                            SectionLabel("Evidence fixtures")
                            Spacer()
                            Text("\(toolchain.fixtureSources.count)")
                                .addressFont(11)
                                .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                            Image(systemName: showFixtures ? "chevron.down" : "chevron.right")
                                .font(.system(size: 11, weight: .bold))
                                .foregroundStyle(Mobius.Phase.indigo)
                        }
                        .padding(.horizontal, Mobius.Space.md)
                        .padding(.top, Mobius.Space.lg)
                        .padding(.bottom, Mobius.Space.xs)
                    }
                    .buttonStyle(.plain)
                    .accessibilityLabel("\(showFixtures ? "Hide" : "Show") evidence fixtures")

                    if showFixtures {
                        ForEach(filtered(toolchain.fixtureSources)) { source in
                            sourceButton(source, fixture: true)
                        }
                    }
                }
            }
            .padding(.bottom, Mobius.Space.lg)
        }
    }

    private func sourceButton(_ source: SourceDocument, fixture: Bool = false) -> some View {
        let selected = toolchain.selectedSource == source
        return Button {
            toolchain.selectSource(source)
        } label: {
            HStack(spacing: Mobius.Space.sm) {
                Image(systemName: fixture ? "testtube.2" : "doc.text")
                    .font(.system(size: 11, weight: .medium))
                    .foregroundStyle(selected ? Mobius.Resolve.cyan : Mobius.Phase.indigo)
                    .frame(width: 16)
                VStack(alignment: .leading, spacing: 2) {
                    Text(source.displayName)
                        .addressFont(11.5)
                        .foregroundStyle(selected ? Mobius.Hinge.paper : Mobius.Hinge.paper.opacity(0.76))
                        .lineLimit(1)
                    if source.directoryLabel != "repository root" {
                        Text(source.directoryLabel)
                            .font(.system(size: 11))
                            .foregroundStyle(Mobius.Hinge.paper.opacity(0.72))
                            .lineLimit(1)
                            .truncationMode(.middle)
                    }
                }
                Spacer(minLength: 0)
            }
            .padding(.horizontal, Mobius.Space.sm + 2)
            .padding(.vertical, Mobius.Space.sm)
            .background(
                RoundedRectangle(cornerRadius: Mobius.Radius.sm, style: .continuous)
                    .fill(selected ? Mobius.Substrate.raised : .clear)
            )
            .overlay(alignment: .leading) {
                if selected {
                    Rectangle().fill(Mobius.Resolve.cyan).frame(width: 2)
                }
            }
        }
        .buttonStyle(.plain)
        .padding(.horizontal, Mobius.Space.sm)
        .accessibilityLabel("\(source.displayName), \(fixture ? "evidence fixture" : "project source"), \(selected ? "selected" : "not selected")")
    }

    private func filtered(_ sources: [SourceDocument]) -> [SourceDocument] {
        guard !search.isEmpty else { return sources }
        return sources.filter {
            $0.relativePath.localizedCaseInsensitiveContains(search)
        }
    }
}

private struct HonestRailState: View {
    let symbol: String
    let title: String
    let detail: String
    let actionTitle: String?
    let action: (() -> Void)?

    var body: some View {
        VStack(alignment: .leading, spacing: Mobius.Space.md) {
            Image(systemName: symbol)
                .font(.system(size: 24, weight: .light))
                .foregroundStyle(Mobius.Phase.indigo)
            Text(title)
                .font(.system(size: 15, weight: .semibold))
                .foregroundStyle(Mobius.Hinge.paper)
            Text(detail)
                .font(.system(size: 11.5))
                .foregroundStyle(Mobius.Hinge.paper.opacity(0.74))
                .fixedSize(horizontal: false, vertical: true)
            if let actionTitle, let action {
                Button(actionTitle, action: action)
                    .buttonStyle(.bordered)
            }
        }
        .padding(Mobius.Space.lg)
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

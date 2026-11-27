import AppKit
import SwiftUI

private struct SandboxSession: Decodable, Identifiable {
    let id: String
    let label: String
    let createdAt: String
    let sourceRoot: String

    var title: String { label.isEmpty ? id : label }
    var dateLabel: String {
        let formatter = ISO8601DateFormatter()
        formatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
        return formatter.date(from: createdAt)?.formatted(date: .abbreviated, time: .shortened) ?? createdAt
    }
}

private struct SandboxPreview: Decodable {
    let diff: String
    let digest: String?
    let conflicts: [MergeConflict]?
    let checkpointOnly: Bool?
    let recovery: Bool?
}

private struct MergeConflict: Decodable {
    let path: String
    let reason: String
    let canResolve: Bool
}

private enum TransferDirection: String, CaseIterable, Identifiable {
    case intoCheckout = "Sandbox → Checkout"
    case intoSandbox = "Checkout → Sandbox"
    var id: Self { self }
    var previewCommand: String { self == .intoSandbox ? "sync-preview" : "preview" }
    var applyCommand: String { self == .intoSandbox ? "sync-apply" : "apply" }
}

private struct CommandFailure: LocalizedError {
    let message: String
    var errorDescription: String? { message }
}

private struct ImportBackend {
    let checkout: URL
    let node: URL

    init() {
        let info = Bundle.main.infoDictionary
        checkout = URL(fileURLWithPath: info?["EarthcallCheckout"] as? String ?? "").resolvingSymlinksInPath()
        node = URL(fileURLWithPath: info?["EarthcallNode"] as? String ?? "")
    }

    func command(_ args: [String]) async throws -> Data {
        let script = Bundle.main.bundleURL.appendingPathComponent("Contents/Resources/scripts/earthcall-sandbox-import.js")
        return try await run(node, [script.path] + args)
    }

    func openDesktop() async throws {
        let standard = URL(fileURLWithPath: "/Applications/GitHub Desktop.app")
        let registered = NSWorkspace.shared.urlForApplication(withBundleIdentifier: "com.github.GitHubClient")
        let bundled = [standard, registered].compactMap { $0 }.map {
            $0.appendingPathComponent("Contents/Resources/app/static/github.sh")
        }
        let launchers = bundled + [URL(fileURLWithPath: "/usr/local/bin/github"),
                                   URL(fileURLWithPath: "/opt/homebrew/bin/github")]
        guard let cli = launchers.first(where: { FileManager.default.isExecutableFile(atPath: $0.path) }) else {
            throw CommandFailure(message: "GitHub Desktop’s command-line launcher is unavailable. In GitHub Desktop, choose Install Command Line Tool from the app menu.")
        }
        _ = try await run(cli, [checkout.path])
    }

    private func run(_ program: URL, _ args: [String]) async throws -> Data {
        try await Task.detached(priority: .userInitiated) {
            let process = Process()
            let output = Pipe()
            process.executableURL = program
            process.arguments = args
            process.currentDirectoryURL = checkout
            process.standardOutput = output
            process.standardError = output
            do {
                try process.run()
            } catch {
                throw CommandFailure(message: "Couldn’t launch \(program.path): \(error.localizedDescription)")
            }
            let data = output.fileHandleForReading.readDataToEndOfFile()
            process.waitUntilExit()
            guard process.terminationStatus == 0 else {
                let detail = String(decoding: data, as: UTF8.self).trimmingCharacters(in: .whitespacesAndNewlines)
                throw CommandFailure(message: detail.isEmpty ? "\(program.lastPathComponent) exited with status \(process.terminationStatus)." : detail)
            }
            return data
        }.value
    }
}

@MainActor
private final class ImportModel: ObservableObject {
    @Published private(set) var sessions: [SandboxSession] = []
    @Published private(set) var selection: String?
    @Published private(set) var preview: SandboxPreview?
    @Published private(set) var refreshing = false
    @Published private(set) var loadingPreview = false
    @Published private(set) var applying = false
    @Published private(set) var openingDesktop = false
    @Published private(set) var imported = false
    @Published private(set) var status = ""
    @Published private(set) var hasError = false
    @Published private(set) var direction: TransferDirection = .intoCheckout

    let backend = ImportBackend()
    private var previewTask: Task<Void, Never>?
    private var resolutions: [String: String] = [:]
    private let decoder: JSONDecoder = {
        let decoder = JSONDecoder()
        decoder.keyDecodingStrategy = .convertFromSnakeCase
        return decoder
    }()

    var selectedSession: SandboxSession? { sessions.first { $0.id == selection } }
    var canApply: Bool {
        preview?.digest != nil && selection != nil && !refreshing && !applying && !openingDesktop
    }

    func changeDirection(_ next: TransferDirection) {
        guard !applying else { return }
        direction = next
        choose(selection)
    }

    func refresh() async {
        guard !refreshing, !applying else { return }
        refreshing = true
        defer { refreshing = false }
        do {
            let result = try await backend.command(["--json", "list"])
            sessions = try decoder.decode([SandboxSession].self, from: result)
                .filter { $0.sourceRoot == backend.checkout.path }
            let preferredSelection = selection ?? UserDefaults.standard.string(forKey: "selectedSession")
            let preferred = sessions.first { $0.id == preferredSelection }
            choose(preferred?.id ?? sessions.first?.id)
        } catch {
            resetPreview()
            report(error.localizedDescription, error: true)
        }
    }

    func choose(_ id: String?, preservingResolutions: Bool = false) {
        guard !applying else { return }
        if !preservingResolutions { resolutions = [:] }
        selection = id
        resetPreview()
        imported = false
        report("")
        guard let id else { return }
        UserDefaults.standard.set(id, forKey: "selectedSession")
        loadingPreview = true
        previewTask = Task {
            do {
                let result = try await backend.command(["--json", direction.previewCommand, id] + resolutionArguments)
                guard !Task.isCancelled else { return }
                preview = try decoder.decode(SandboxPreview.self, from: result)
                if let count = preview?.conflicts?.count, count > 0 {
                    report("\(count) conflicting files. Review the markers and choose which version to keep.", error: true)
                } else if preview?.recovery == true {
                    report("An interrupted transplant is ready to resume.")
                } else if preview?.checkpointOnly == true, preview?.digest != nil {
                    report("File contents agree. Apply to record their shared baseline.")
                }
            } catch {
                guard !Task.isCancelled else { return }
                report(error.localizedDescription, error: true)
            }
            loadingPreview = false
        }
    }

    private var resolutionArguments: [String] {
        resolutions.keys.sorted().flatMap { ["--resolve-" + resolutions[$0]!, $0] }
    }

    func resolve(_ file: String, using choice: String) {
        resolutions[file] = choice
        choose(selection, preservingResolutions: true)
    }

    func applyAndOpen() async {
        guard canApply, let id = selection, let digest = preview?.digest else { return }
        applying = true
        defer { applying = false }
        report(direction == .intoSandbox ? "Merging checkout changes into the sandbox…" : "Importing into the checkout…")
        do {
            _ = try await backend.command([direction.applyCommand, id, "--expected-diff", digest] + resolutionArguments)
        } catch {
            resetPreview()
            report(error.localizedDescription, error: true)
            return
        }
        resetPreview()
        imported = true
        if direction == .intoSandbox {
            report("Checkout changes merged into the sandbox. Gemini can continue from the updated files.")
        } else {
            report("Imported into the checkout. Opening GitHub Desktop…")
            await openDesktop()
        }
    }

    func openDesktop() async {
        guard !openingDesktop else { return }
        openingDesktop = true
        defer { openingDesktop = false }
        do {
            try await backend.openDesktop()
            if imported && direction == .intoCheckout { report("Imported. Review the unstaged changes in GitHub Desktop.") }
        } catch {
            let prefix = imported && direction == .intoCheckout ? "Imported into the checkout. " : ""
            report(prefix + error.localizedDescription, error: true)
        }
    }

    private func resetPreview() {
        previewTask?.cancel()
        preview = nil
        loadingPreview = false
    }

    private func report(_ message: String, error: Bool = false) {
        status = message
        hasError = error
    }
}

private struct DiffView: NSViewRepresentable {
    let diff: String

    func makeNSView(context: Context) -> NSScrollView {
        let scroll = NSTextView.scrollableTextView()
        let editor = scroll.documentView as! NSTextView
        scroll.hasHorizontalScroller = true
        editor.isEditable = false
        editor.isHorizontallyResizable = true
        editor.textContainerInset = NSSize(width: 12, height: 12)
        editor.textContainer?.widthTracksTextView = false
        editor.textContainer?.containerSize = NSSize(width: CGFloat.greatestFiniteMagnitude,
                                                     height: CGFloat.greatestFiniteMagnitude)
        editor.autoresizingMask = []
        return scroll
    }

    func updateNSView(_ scroll: NSScrollView, context: Context) {
        guard let editor = scroll.documentView as? NSTextView, editor.string != diff else { return }
        let colored = NSMutableAttributedString(string: diff, attributes: [
            .font: NSFont.monospacedSystemFont(ofSize: 12, weight: .regular),
            .foregroundColor: NSColor.labelColor,
        ])
        let lines = diff as NSString
        lines.enumerateSubstrings(in: NSRange(location: 0, length: lines.length), options: .byLines) { line, range, _, _ in
            guard let line else { return }
            let color: NSColor
            switch line.first {
            case "+": color = .systemGreen
            case "-": color = .systemRed
            case "@": color = .systemBlue
            default: return
            }
            colored.addAttribute(.foregroundColor, value: color, range: range)
        }
        editor.textStorage?.setAttributedString(colored)
        editor.scrollRangeToVisible(NSRange(location: 0, length: 0))
    }
}

private struct ImportWindow: View {
    @StateObject private var model = ImportModel()

    var body: some View {
        NavigationSplitView {
            List(model.sessions, selection: Binding(get: { model.selection }, set: { model.choose($0) })) { session in
                VStack(alignment: .leading, spacing: 3) {
                    Text(session.title).font(.headline).lineLimit(1)
                    Text(session.dateLabel).font(.caption).foregroundStyle(.secondary)
                }
                .padding(.vertical, 5)
                .tag(session.id)
            }
            .listStyle(.sidebar)
            .disabled(model.applying)
            .navigationTitle("Gemini Sandboxes")
            .navigationSplitViewColumnWidth(min: 200, ideal: 235, max: 315)
        } detail: {
            VStack(alignment: .leading, spacing: 0) {
                VStack(alignment: .leading, spacing: 5) {
                    Text(model.selectedSession?.title ?? "Gemini Sandboxes")
                        .font(.title2.weight(.semibold))
                    Label(model.backend.checkout.path.replacingOccurrences(
                        of: NSHomeDirectory(), with: "~", options: .anchored), systemImage: "folder")
                        .font(.callout).foregroundStyle(.secondary).textSelection(.enabled)
                    Picker("Transfer direction", selection: Binding(
                        get: { model.direction }, set: { model.changeDirection($0) })) {
                        ForEach(TransferDirection.allCases) { Text($0.rawValue).tag($0) }
                    }
                    .pickerStyle(.segmented)
                    .disabled(model.applying)
                    .padding(.top, 9)
                }
                .padding(18)
                Divider()
                if let conflicts = model.preview?.conflicts, !conflicts.isEmpty {
                    ScrollView {
                        VStack(alignment: .leading, spacing: 10) {
                            ForEach(conflicts, id: \.path) { conflict in
                                VStack(alignment: .leading, spacing: 4) {
                                    Text(conflict.path).font(.callout.weight(.semibold)).textSelection(.enabled)
                                    Text(conflict.reason).font(.caption).foregroundStyle(.secondary)
                                    if conflict.canResolve {
                                        HStack {
                                            Button(model.direction == .intoSandbox ? "Keep Sandbox" : "Keep Checkout") {
                                                model.resolve(conflict.path, using: "ours")
                                            }
                                            Button(model.direction == .intoSandbox ? "Use Checkout" : "Use Sandbox") {
                                                model.resolve(conflict.path, using: "theirs")
                                            }
                                        }
                                    }
                                }
                            }
                        }.padding(12).frame(maxWidth: .infinity, alignment: .leading)
                    }
                    .frame(maxHeight: 180)
                    Divider()
                }
                detail
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                Divider()
                HStack(spacing: 12) {
                    if !model.status.isEmpty {
                        Label(model.status, systemImage: model.hasError ? "exclamationmark.circle" : "info.circle")
                            .font(.callout)
                            .foregroundStyle(model.hasError ? Color.red : Color.secondary)
                            .textSelection(.enabled)
                    }
                    Spacer(minLength: 12)
                    Button(model.direction == .intoSandbox ? "Merge Into Sandbox" : "Import & Open GitHub Desktop",
                           systemImage: model.direction == .intoSandbox ? "arrow.triangle.merge" : "arrow.down.to.line") {
                        Task { await model.applyAndOpen() }
                    }
                    .buttonStyle(.borderedProminent)
                    .controlSize(.large)
                    .disabled(!model.canApply)
                }
                .padding(14)
            }
        }
        .toolbar {
            ToolbarItemGroup {
                Button("Refresh", systemImage: "arrow.clockwise") { Task { await model.refresh() } }
                    .keyboardShortcut("r")
                    .help("Refresh sessions and preview")
                    .disabled(model.refreshing || model.applying)
                Button("Open in GitHub Desktop", systemImage: "arrow.up.right.square") {
                    Task { await model.openDesktop() }
                }
                .help("Open this checkout in GitHub Desktop")
                .disabled(model.applying || model.openingDesktop)
            }
        }
        .frame(minWidth: 810, minHeight: 470)
        .task { await model.refresh() }
    }

    @ViewBuilder private var detail: some View {
        if model.refreshing || model.loadingPreview || model.applying {
            ProgressView(model.applying ? "Importing…" : "Checking sandbox changes…")
        } else if let preview = model.preview, !preview.diff.isEmpty {
            DiffView(diff: preview.diff)
                .accessibilityLabel("Git diff")
        } else if model.imported {
            ContentUnavailableView(model.direction == .intoSandbox ? "Sandbox updated" : "Imported", systemImage: "checkmark.circle",
                                   description: Text("Refresh to check for more sandbox changes."))
        } else if model.hasError {
            ContentUnavailableView("Couldn’t load changes", systemImage: "exclamationmark.circle")
        } else if model.selection == nil {
            ContentUnavailableView("No sandboxes found", systemImage: "tray",
                                   description: Text("Create a Gemini sandbox for this checkout, then refresh."))
        } else {
            ContentUnavailableView(model.preview?.digest == nil ? "Up to date" : "Record shared baseline",
                                   systemImage: "checkmark.circle")
        }
    }
}

@main
private struct SandboxImportApp: App {
    var body: some Scene {
        Window("Earthcall Sandbox Import", id: "sandbox-import") { ImportWindow() }
            .defaultSize(width: 1020, height: 650)
    }
}

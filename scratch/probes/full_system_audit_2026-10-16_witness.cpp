// Full-system audit witness probe (2026-10-16). Not a suite test: it asserts
// what the code DOES today, so the audit can cite a run instead of a reading.
//   W1  a Law round-tripped through JSON (authority clamped as on any file
//       read) whose subject is shell-channel runs a shell command.
//   W2  a Law whose subject is an ordinary Object reaches the same command
//       through @shell-channel.* root resolution.
//   W3  writing face.0.resolution on one Object resizes the shared Material
//       that a second Object also names.
//   W4  a Law applied to the boot-registered StreamChannel launches a process
//       via stream.target + stream.open (pipeType defaults to "process").
//
// Build (after `cmake --build build --target metalaw_test`), from build/:
//   DEF=$(grep '^CXX_DEFINES' CMakeFiles/metalaw_test.dir/flags.make | cut -d= -f2-)
//   INC=$(grep '^CXX_INCLUDES' CMakeFiles/metalaw_test.dir/flags.make | cut -d= -f2-)
//   eval c++ -g -std=gnu++17 $DEF $INC -c ../scratch/probes/full_system_audit_2026-10-16_witness.cpp -o /tmp/w.o
//   eval "$(sed 's|CMakeFiles/metalaw_test.dir/tests/law/metalaw_test.cpp.o|/tmp/w.o|; s|-o metalaw_test|-o /tmp/w|' CMakeFiles/metalaw_test.dir/link.txt)"
//   /tmp/w <empty scratch dir>      # W1 touches <dir>/shell_witness_A only
// W2 printed path result 1 in this harness: the @root route is NOT witnessed.
// After the F1/F2 fixes (same session, 2026-10-16): W1 and W4 must refuse and create
// no file; with EARTHCALL_ALLOW_PROCESS_CHANNELS=1 both run. W3 must print shared=0
// and b.face0.width=16. All three confirmed after the fixes.
// Audit: docs/audits/FULL_SYSTEM_AUDIT_2026-10-16.md. Claude Code / Claude Opus 5.5 /
// session d6fe31dc-53d9-49c1-a4b7-87479d9d605b / 2026-10-16.
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "Singularity/Foreign/Shell/ShellChannel.hpp"
#include "Singularity/Storage/StreamChannel.hpp"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <filesystem>
#include <string>

extern MaterialManager materials;

static const char* res(Law::ApplicationResult r) {
    switch (r) {
        case Law::ApplicationResult::Applied: return "Applied";
        case Law::ApplicationResult::Unauthored: return "Unauthored";
        case Law::ApplicationResult::AuthorityDenied: return "AuthorityDenied";
        case Law::ApplicationResult::Disabled: return "Disabled";
        case Law::ApplicationResult::ConditionsFailed: return "ConditionsFailed";
        default: return "other";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    const std::string dir = argv[1];
    if (!glfwInit()) return 3;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* w = glfwCreateWindow(64, 64, "audit_witness", nullptr, nullptr);
    if (!w) return 4;
    glfwMakeContextCurrent(w);
    {
        Object author;
        Singularity::Foreign::Shell::ShellChannel shell;

        // W1 — file-borne Law, metalaw subject.
        const std::string fileA = dir + "/shell_witness_A";
        std::filesystem::remove(fileA);
        Law proto("audit-shell-a");
        proto.addAuthor(author);
        proto.setActionModel(ActionNode::sequence({
            ActionNode::set("command", PropertyValue(std::string("touch '" + fileA + "'"))),
            ActionNode::add("executeRequests", 1.0)}));
        nlohmann::json j = proto.toJson();
        j["authority"] = 0;
        auto loaded = Law::fromJson(j);
        std::printf("W1 authored-straight-from-json=%d authors-json=%s\n",
                    loaded ? (int)loaded->isAuthored() : -1, j.contains("authors") ? j["authors"].dump().c_str() : "none");
        if (loaded && !loaded->isAuthored()) loaded->addAuthor(author);
        std::printf("W1 loaded.authored-after-json=%d authority=%d\n",
                    loaded ? (int)loaded->isAuthored() : -1, loaded ? loaded->authorityLevel() : -1);
        auto r1 = loaded->applyTo(shell);
        std::printf("W1 apply=%s file-exists=%d exit=%s\n", res(r1),
                    (int)std::filesystem::exists(fileA), "");

        // W2 — @root resolution from an Object subject.
        const std::string fileB = dir + "/shell_witness_B";
        std::filesystem::remove(fileB);
        Object bystander;
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            out.push_back(&shell);
            out.push_back(&bystander);
        });
        Law viaRoot("audit-shell-b");
        viaRoot.addAuthor(author);
        viaRoot.setActionModel(ActionNode::sequence({
            ActionNode::set("@shell-channel.command", PropertyValue(std::string("touch '" + fileB + "'"))),
            ActionNode::add("@shell-channel.executeRequests", 1.0)}));
        auto r2 = viaRoot.applyTo(bystander);
        PropertyValue cmd;
        auto cr = PropertyPath::parse("command").getValue(shell, cmd);
        std::printf("W2 apply=%s file-exists=%d shell.command-after=%s (r=%d)\n", res(r2),
                    (int)std::filesystem::exists(fileB),
                    cr == PropertyPath::PathResult::Ok ? std::get<std::string>(cmd).c_str() : "?", (int)cr);
        PropertyValue viaPath;
        auto vr = PropertyPath::parse("@shell-channel.command").getValue(bystander, viaPath);
        std::printf("W2 read @shell-channel.command from Object subject: r=%d\n", (int)vr);
        std::printf("W2 bystander has property '@shell-channel'? %d\n", bystander.findProperty("@shell-channel.command") != nullptr);

        // W3 — shared Material resized through one Object's face path.
        auto mat = materials.create("audit-shared-stone");
        mat->initFaceTextures(6, 16, 16);
        Object a, b;
        a.setMaterialId(mat->getIdentifier());
        b.setMaterialId(mat->getIdentifier());
        PropertyValue v;
        auto pr = PropertyPath::parse("face.0.resolution").setValue(a, PropertyValue(64.0));
        auto bm = materials.resolveOrDefault(b.materialId());
        std::printf("W3 set=%d a.material=%s b.material=%s shared=%d b.face0.width=%d\n",
                    (int)pr, a.materialId().c_str(), b.materialId().c_str(),
                    (int)(a.materialId() == b.materialId()),
                    bm && !bm->faceTextures.empty() ? bm->faceTextures[0].width : -1);

        // W4 — StreamChannel process pipe.
        const std::string fileC = dir + "/shell_witness_C";
        std::filesystem::remove(fileC);
        Singularity::Storage::StreamChannel stream;
        Law viaStream("audit-stream");
        viaStream.addAuthor(author);
        viaStream.setActionModel(ActionNode::sequence({
            ActionNode::set("stream.target", PropertyValue(std::string("touch '" + fileC + "'"))),
            ActionNode::set("stream.open", PropertyValue(true)),
            ActionNode::set("stream.close", PropertyValue(true))}));
        auto r4 = viaStream.applyTo(stream);
        std::printf("W4 apply=%s file-exists=%d\n", res(r4), (int)std::filesystem::exists(fileC));
    }
    glfwDestroyWindow(w);
    glfwTerminate();
    return 0;
}

// MathNode::typeOf must cost linear time per call in the size of the tree.
//
// It used to type every child in a Matrix-refusal pre-loop, discard the answers, and
// type them all again in the operation's case: two visits per level, so 2^height, run
// at every node by MathNode::fromJson. Found when an authored aurora sky stalled Zone
// load for about a minute (docs/Agenda/Tasks/Specific Tasks/Zones and Ourverse/
// The_Merry_Dancers/The_Merry_Dancers.md). Zach, 2026-11-03: "DO 1."
//
// The fix must change cost and nothing else, so this pins both:
//   - depth: a chain far too deep for the old cost finishes at all;
//   - meaning: exact kinds and diagnostics, including the pre-loop's order (a bad child
//     is reported before the parent's arity error) and its refusal of Matrix children.
//
// --table prints every corpus answer, and --corpus <saves dir> prints the answer for
// every expression in every saved Zone, so two builds can be diffed. --scale <depth>
// times one chain, which is safe to run on the old build only at small depths.
// Claude Code / Claude Opus 5.5 / session ed9d515b-d6ac-4098-a7f9-ce71d02406b3 / 2026-11-03.

#include "Singularity/OntoMath/ScalarForm.hpp"

#include "json.hpp"

#include <chrono>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using J = nlohmann::json;
using OntoMath::MathNode;
using OntoMath::TypeEnv;
using OntoMath::ValueKind;
namespace fs = std::filesystem;

namespace {
int g_checks = 0, g_failures = 0;
void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << what << std::endl;
}

J num(double v) { return {{"op", 0}, {"scalarForm", {{"terms", J::array({{{"c", v}, {"factors", J::object()}}})}}}}; }
J var(const std::string& n) { return {{"op", 1}, {"var", n}}; }
J node(int op, std::vector<J> kids) { return {{"op", op}, {"children", kids}}; }
J vec() { return node(2, {num(1), num(2), num(3)}); }
J badAxis() { return {{"op", 3}, {"arg", "w"}, {"children", J::array({vec()})}}; }
J identity() { return {{"op", 31}, {"rows", 2}, {"cols", 2}}; }

constexpr int Add = 4, Sub = 5, Scale = 6, Dot = 7, Cross = 8, Hadamard = 9, Normalize = 10, Length = 11,
              Project = 14, Distance = 15, Union = 20, Div = 23, Pow = 24, Abs = 25, Clamp = 26, Sqrt = 27,
              Tan = 28, Noise = 29, MatScale = 34, Det = 38, Integral = 40, Let = 41;

J let(const std::string& name, J value, J body) { return {{"op", Let}, {"var", name}, {"children", {value, body}}}; }
J integral(const std::string& name, J body) { return {{"op", Integral}, {"var", name}, {"children", {body, num(0), num(1)}}}; }

std::string answer(const MathNode& n, const TypeEnv& env, bool lenient) {
    std::string error;
    ValueKind kind = ValueKind::Scalar;
    if (!n.checkTypes(env, error, &kind, lenient)) return "ERROR " + error;
    return "kind " + std::to_string(static_cast<int>(kind));
}

std::vector<std::pair<std::string, J>> corpus() {
    return {
        {"leaf", num(1)},
        {"vector", vec()},
        {"add scalar scalar", node(Add, {num(1), num(2)})},
        {"add vector vector", node(Add, {vec(), vec()})},
        {"add scalar vector mismatch", node(Add, {num(1), vec()})},
        {"sub unknown scalar", node(Sub, {var("q"), num(1)})},
        {"scale scalar vector", node(Scale, {num(2), vec()})},
        {"scale vector vector", node(Scale, {vec(), vec()})},
        {"add one child: arity", node(Add, {num(1)})},
        {"add three children: arity", node(Add, {num(1), num(2), num(3)})},
        {"arity with a bad child: child first", node(Add, {num(1), num(2), badAxis()})},
        {"clamp arity with a bad child", node(Clamp, {badAxis(), num(0)})},
        {"matrix child of add", node(Add, {identity(), identity()})},
        {"matrix second child of scale", node(Scale, {num(2), identity()})},
        {"matrix child of clamp", node(Clamp, {identity(), num(0), num(1)})},
        {"matrix child beside an arity error", node(Abs, {identity(), num(1)})},
        {"matrix scale is admitted", node(MatScale, {num(2), identity()})},
        {"determinant", node(Det, {identity()})},
        {"bad axis", badAxis()},
        {"nested path in the diagnostic", node(Clamp, {node(Add, {num(1), badAxis()}), num(0), num(1)})},
        {"second child's path", node(Div, {num(1), node(Sqrt, {node(Abs, {badAxis()})})})},
        {"first error wins", node(Add, {badAxis(), node(Add, {num(1), vec()})})},
        {"dot", node(Dot, {vec(), vec()})},
        {"dot mismatch", node(Dot, {num(1), vec()})},
        {"cross", node(Cross, {vec(), vec()})},
        {"hadamard", node(Hadamard, {vec(), vec()})},
        {"normalize", node(Normalize, {vec()})},
        {"length", node(Length, {vec()})},
        {"length of scalar", node(Length, {num(1)})},
        {"project", node(Project, {vec(), vec()})},
        {"distance", node(Distance, {vec(), vec()})},
        {"union", node(Union, {num(1), num(2)})},
        {"pow", node(Pow, {num(2), num(2)})},
        {"tan of vector", node(Tan, {vec()})},
        {"noise of vector", node(Noise, {vec()})},
        {"noise of scalar", node(Noise, {num(1)})},
        {"let scalar", let("a", num(1), node(Add, {var("a"), num(2)}))},
        {"let vector into scalar add", let("a", vec(), node(Add, {var("a"), num(2)}))},
        {"let shadows the outer binding", let("v", vec(), let("v", num(1), node(Add, {var("v"), num(2)})))},
        {"let value error", let("a", badAxis(), num(1))},
        {"let body error", let("a", num(1), badAxis())},
        {"integral", integral("s", node(Scale, {var("s"), var("s")}))},
        {"integral body error", integral("s", badAxis())},
        {"unbound inside add", node(Add, {var("nobody"), num(1)})},
        {"bound point", node(Length, {var("p")})},
    };
}

void walk(const J& j, const std::string& where, const TypeEnv& env) {
    if (j.is_object()) {
        if (j.contains("mathNode") && j["mathNode"].is_object()) {
            const auto n = MathNode::fromJson(j["mathNode"]);
            std::cout << where << " | " << answer(*n, env, true) << " | " << answer(*n, env, false) << "\n";
        }
        for (auto it = j.begin(); it != j.end(); ++it)
            if (it.key() != "mathNode") walk(it.value(), where, env);
    } else if (j.is_array()) {
        for (const auto& v : j) walk(v, where, env);
    }
}

J chain(int depth) {
    J n = num(1);
    for (int i = 0; i < depth; ++i) n = node(Add, {std::move(n), num(1)});
    return n;
}

double secondsOf(const std::function<void()>& f) {
    const auto a = std::chrono::steady_clock::now();
    f();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - a).count();
}

TypeEnv declaredPoint() {
    return {{"p", ValueKind::Vector}};
}
}  // namespace

int main(int argc, char** argv) {
    const TypeEnv env = declaredPoint();

    if (argc > 1 && !std::strcmp(argv[1], "--table")) {
        for (const auto& [name, j] : corpus()) {
            const auto n = MathNode::fromJson(j);
            std::cout << name << " | " << answer(*n, env, true) << " | " << answer(*n, env, false) << "\n";
        }
        return 0;
    }
    if (argc > 2 && !std::strcmp(argv[1], "--corpus")) {
        // Every saved form that holds expressions: Zone identities in either encoding,
        // Material roots and Law roots. Read-only.
        std::vector<fs::path> files;
        for (const auto& e : fs::recursive_directory_iterator(argv[2], fs::directory_options::skip_permission_denied)) {
            if (!e.is_regular_file()) continue;
            const std::string name = e.path().filename().string();
            if (name == "zone.json" || name == "zone.ecform" || name == "material.json" || name == "law.json") files.push_back(e.path());
        }
        std::sort(files.begin(), files.end());
        for (const auto& f : files) {
            std::ifstream in(f, std::ios::binary);
            J doc;
            if (f.extension() == ".ecform") {
                const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
                const J envelope = J::from_msgpack(bytes, true, false);
                if (envelope.is_discarded() || !envelope.contains("MigrationRoot") || !envelope["MigrationRoot"].is_string()) continue;
                doc = J::parse(envelope["MigrationRoot"].get<std::string>(), nullptr, false);
            } else {
                doc = J::parse(in, nullptr, false);
            }
            if (!doc.is_discarded()) walk(doc, fs::relative(f, argv[2]).string(), env);
        }
        return 0;
    }
    if (argc > 2 && !std::strcmp(argv[1], "--scale")) {
        const int d = std::atoi(argv[2]);
        const J j = chain(d);
        std::unique_ptr<MathNode> parsed;
        const double load = secondsOf([&] { parsed = MathNode::fromJson(j); });
        std::string error;
        const double one = secondsOf([&] { parsed->checkTypes(env, error, nullptr, false); });
        std::cout << "depth " << d << ": fromJson " << load << " s, one checkTypes " << one << " s\n";
        return 0;
    }

    std::cout << "MathNode::typeOf: linear cost, unchanged meaning\n";

    // --- Meaning: the behaviours the old pre-loop gave, pinned exactly -----------------
    {
        std::string error;
        const auto bad = MathNode::fromJson(node(Add, {num(1), num(2), badAxis()}));
        check(!bad->checkTypes(env, error, nullptr, false) && error.find("axis") != std::string::npos,
              "a bad child is still reported before the parent's arity error: " + error);
        const auto matrix = MathNode::fromJson(node(Abs, {identity(), num(1)}));
        check(!matrix->checkTypes(env, error, nullptr, false) && error.find("is Matrix") != std::string::npos,
              "a Matrix child is still refused before the parent's arity error: " + error);
        const auto nested = MathNode::fromJson(node(Clamp, {node(Add, {num(1), badAxis()}), num(0), num(1)}));
        check(!nested->checkTypes(env, error, nullptr, false) && error.find("Clamp[0]") != std::string::npos &&
                  error.find("Add[1]") != std::string::npos,
              "a nested diagnostic still names its full path: " + error);
        const auto shadow = MathNode::fromJson(let("v", vec(), let("v", num(1), node(Add, {var("v"), num(2)}))));
        ValueKind kind = ValueKind::Vector;
        check(shadow->checkTypes(env, error, &kind, false) && kind == ValueKind::Scalar,
              "Let's inner binding still shadows the outer one");
    }

    // --- Depth: impossible at two visits per level ------------------------------------
    {
        const int depth = 400;   // the old cost at this height is about 2^400 visits
        const J j = chain(depth);
        std::unique_ptr<MathNode> parsed;
        const double load = secondsOf([&] { parsed = MathNode::fromJson(j); });
        std::cout << "       depth " << depth << " fromJson = " << load << " s" << std::endl;
        check(parsed != nullptr, "a 400-level expression loads at all");
        check(load < 30.0, "and in bounded time");
        std::string error;
        ValueKind kind = ValueKind::Vector;
        const double one = secondsOf([&] { check(parsed->checkTypes(env, error, &kind, false), "and type-checks"); });
        std::cout << "       depth " << depth << " one checkTypes = " << one << " s" << std::endl;
        check(kind == ValueKind::Scalar, "with the right answer at the root");
    }

    // --- Doubling the depth must roughly double one check, not square it ---------------
    // Stops at 800: these trees are walked recursively, so about 1000 levels overflow the
    // stack in a Debug build. That limit predates this fix, which merely makes it reachable.
    // Real saved expressions are far shallower. A depth guard is a separate decision.
    {
        std::vector<double> times;
        for (int d : {200, 400, 800}) {
            const auto n = MathNode::fromJson(chain(d));
            std::string error;
            double best = 1e9;
            for (int i = 0; i < 5; ++i)
                best = std::min(best, secondsOf([&] { n->checkTypes(env, error, nullptr, false); }));
            times.push_back(best);
        }
        std::cout << "       one check: depth 200 = " << times[0] << " s, 400 = " << times[1]
                  << " s, 800 = " << times[2] << " s" << std::endl;
        check(times[2] < times[0] * 40.0, "four times the depth costs far less than an exponential would");
    }

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}

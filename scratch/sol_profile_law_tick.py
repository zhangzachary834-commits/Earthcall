#!/usr/bin/env python3
"""CI-only stage profiler for Law::applyTo.

Adds cumulative timers and dumps them with atexit, so profiler I/O happens only
after frame_lag_test has finished measuring. The engine source in the branch is
not modified; this patches only the runner checkout.
"""
from pathlib import Path

p = Path("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp")
s = p.read_text()

include_anchor = '#include <cstdio>\n'
if include_anchor not in s:
    raise SystemExit("include anchor not found")
s = s.replace(include_anchor, include_anchor + '#include <cstdlib>\n#include <unordered_map>\n', 1)

namespace_anchor = """namespace {\nstd::vector<std::string> formationMemberIds(const Formation& formation) {\n"""
namespace_replacement = """namespace {\nstruct SolApplyStagePerf {\n    double totalMs = 0.0;\n    double conditionMs = 0.0;\n    double actionMs = 0.0;\n    double recordMs = 0.0;\n    double bookMs = 0.0;\n    double auditMs = 0.0;\n    double trimMs = 0.0;\n    double publishMs = 0.0;\n    std::uint64_t calls = 0;\n};\nstd::unordered_map<std::string, SolApplyStagePerf> gSolApplyStagePerf;\n\nvoid solDumpApplyStagePerf() {\n    for (const auto& [id, p] : gSolApplyStagePerf) {\n        if (id.rfind(\"law-studio\", 0) != 0) continue;\n        const double known = p.conditionMs + p.actionMs + p.recordMs +\n                             p.bookMs + p.trimMs + p.publishMs;\n        const double residual = p.totalMs - known;\n        std::fprintf(stderr,\n            \"SOL_APPLY_STAGE\\t%s\\tcalls=%llu\\ttotal=%.6f\\tcondition=%.6f\\taction=%.6f\\trecord=%.6f\\tbook=%.6f\\taudit=%.6f\\ttrim=%.6f\\tpublish=%.6f\\tresidual=%.6f\\n\",\n            id.c_str(), static_cast<unsigned long long>(p.calls), p.totalMs,\n            p.conditionMs, p.actionMs, p.recordMs, p.bookMs, p.auditMs,\n            p.trimMs, p.publishMs, residual);\n    }\n}\n\nstd::vector<std::string> formationMemberIds(const Formation& formation) {\n"""
if namespace_anchor not in s:
    raise SystemExit("namespace anchor not found")
s = s.replace(namespace_anchor, namespace_replacement, 1)

apply_anchor = """Law::ApplicationResult Law::applyTo(Singular& target) {\n    ApplicationResult result = ApplicationResult::Applied;\n    ActionNode::Trace trace;\n"""
apply_replacement = """Law::ApplicationResult Law::applyTo(Singular& target) {\n    static const bool solDumpRegistered = [] {\n        std::atexit(solDumpApplyStagePerf);\n        return true;\n    }();\n    (void)solDumpRegistered;\n    auto& solPerf = gSolApplyStagePerf[_lawId];\n    const double solTotalT0 = glfwGetTime();\n\n    ApplicationResult result = ApplicationResult::Applied;\n    ActionNode::Trace trace;\n"""
if apply_anchor not in s:
    raise SystemExit("apply anchor not found")
s = s.replace(apply_anchor, apply_replacement, 1)

condition_anchor = """    } else if (!conditionsSatisfied(target)) {\n        result = ApplicationResult::ConditionsFailed;\n"""
condition_replacement = """    } else if ([&]() {\n        const double t0 = glfwGetTime();\n        const bool failed = !conditionsSatisfied(target);\n        solPerf.conditionMs += (glfwGetTime() - t0) * 1000.0;\n        return failed;\n    }()) {\n        result = ApplicationResult::ConditionsFailed;\n"""
if condition_anchor not in s:
    raise SystemExit("condition anchor not found")
s = s.replace(condition_anchor, condition_replacement, 1)

action_start = """    } else {\n        ECA::Event event;\n        event.type = \"law-apply\";\n"""
action_start_repl = """    } else {\n        const double solActionT0 = glfwGetTime();\n        ECA::Event event;\n        event.type = \"law-apply\";\n"""
if action_start not in s:
    raise SystemExit("action start anchor not found")
s = s.replace(action_start, action_start_repl, 1)

action_end = """        if (!_actionModel && !_actions.empty() && trace.nodes.empty()) {\n            trace.nodes.push_back(ActionNode::NodeOutcome{\n                \"FirstMover\", {}, true, PropertyPath::PathResult::Ok, {}});\n        }\n    }\n\n    _applicationLog.push_back(makeRecord(&target, result));\n    _applicationLog.back().trace = trace;\n"""
action_end_repl = """        if (!_actionModel && !_actions.empty() && trace.nodes.empty()) {\n            trace.nodes.push_back(ActionNode::NodeOutcome{\n                \"FirstMover\", {}, true, PropertyPath::PathResult::Ok, {}});\n        }\n        solPerf.actionMs += (glfwGetTime() - solActionT0) * 1000.0;\n    }\n\n    const double solRecordT0 = glfwGetTime();\n    _applicationLog.push_back(makeRecord(&target, result));\n    _applicationLog.back().trace = trace;\n    solPerf.recordMs += (glfwGetTime() - solRecordT0) * 1000.0;\n"""
if action_end not in s:
    raise SystemExit("action end anchor not found")
s = s.replace(action_end, action_end_repl, 1)

book_start = """    if (result == ApplicationResult::Applied) {\n        // Report what the NODES did, not merely that we got here. A law whose\n"""
book_start_repl = """    const double solBookT0 = glfwGetTime();\n    if (result == ApplicationResult::Applied) {\n        // Report what the NODES did, not merely that we got here. A law whose\n"""
if book_start not in s:
    raise SystemExit("book start anchor not found")
s = s.replace(book_start, book_start_repl, 1)

audit_anchor = """        if (!skipAudit) {\n            nlohmann::json nodesJson = nlohmann::json::array();\n"""
audit_repl = """        if (!skipAudit) {\n            const double solAuditT0 = glfwGetTime();\n            nlohmann::json nodesJson = nlohmann::json::array();\n"""
if audit_anchor not in s:
    raise SystemExit("audit start anchor not found")
s = s.replace(audit_anchor, audit_repl, 1)

audit_end = """            ECA::LawAuditLogger::instance().log(\"LAW\", message, {\n                {\"lawId\", getIdentifier()},\n                {\"targetId\", target.getIdentifier()},\n                {\"result\", \"Applied\"},\n                {\"changed\", wrote},\n                {\"nodes\", nodesJson}\n            });\n        }\n    }\n    // Bounded memory: a WhileTrue law applies every tick — the log is a\n"""
audit_end_repl = """            ECA::LawAuditLogger::instance().log(\"LAW\", message, {\n                {\"lawId\", getIdentifier()},\n                {\"targetId\", target.getIdentifier()},\n                {\"result\", \"Applied\"},\n                {\"changed\", wrote},\n                {\"nodes\", nodesJson}\n            });\n            solPerf.auditMs += (glfwGetTime() - solAuditT0) * 1000.0;\n        }\n    }\n    solPerf.bookMs += (glfwGetTime() - solBookT0) * 1000.0;\n    // Bounded memory: a WhileTrue law applies every tick — the log is a\n"""
if audit_end not in s:
    raise SystemExit("audit end anchor not found")
s = s.replace(audit_end, audit_end_repl, 1)

trim_anchor = """    constexpr std::size_t kMaxLogEntries = 256;\n    if (_applicationLog.size() > kMaxLogEntries) {\n        _applicationLog.erase(_applicationLog.begin(),\n                              _applicationLog.end() - kMaxLogEntries);\n    }\n    publishAppliedEvent(&target, result);\n    return result;\n}\n"""
trim_repl = """    const double solTrimT0 = glfwGetTime();\n    constexpr std::size_t kMaxLogEntries = 256;\n    if (_applicationLog.size() > kMaxLogEntries) {\n        _applicationLog.erase(_applicationLog.begin(),\n                              _applicationLog.end() - kMaxLogEntries);\n    }\n    solPerf.trimMs += (glfwGetTime() - solTrimT0) * 1000.0;\n\n    const double solPublishT0 = glfwGetTime();\n    publishAppliedEvent(&target, result);\n    solPerf.publishMs += (glfwGetTime() - solPublishT0) * 1000.0;\n    solPerf.totalMs += (glfwGetTime() - solTotalT0) * 1000.0;\n    ++solPerf.calls;\n    return result;\n}\n"""
if trim_anchor not in s:
    raise SystemExit("trim/publish anchor not found")
s = s.replace(trim_anchor, trim_repl, 1)

# Keep the coarse per-law timing from the previous profiler. It tells us how
# much of LawManager's continuous loop is outside Law::applyTo itself.
loop_anchor = """        const std::string lawId = law->getIdentifier();\n"""
loop_repl = """        const std::string lawId = law->getIdentifier();\n        const double solLawProfileT0 = glfwGetTime();\n        const auto solProfileLaw = [&](const char* path, std::size_t candidates) {\n            const double ms = (glfwGetTime() - solLawProfileT0) * 1000.0;\n            std::fprintf(stderr,\n                         \"SOL_LAW_PERF\\t%s\\t%s\\tact=%d\\tpath=%s\\tcandidates=%zu\\tms=%.6f\\n\",\n                         lawId.c_str(), law->name().c_str(),\n                         static_cast<int>(law->activation()), path, candidates, ms);\n        };\n"""
if loop_anchor not in s:
    raise SystemExit("continuous law loop anchor not found")
s = s.replace(loop_anchor, loop_repl, 1)

rete_anchor = """                applyAndMaybeDrive(*law, *subject, records);\n            }\n            continue;\n        }\n\n        // OnBecomeTrue and laws without Rete terminals: full sweep path.\n"""
rete_repl = """                applyAndMaybeDrive(*law, *subject, records);\n            }\n            solProfileLaw(\"rete\", subjects.size());\n            continue;\n        }\n\n        // OnBecomeTrue and laws without Rete terminals: full sweep path.\n"""
if rete_anchor not in s:
    raise SystemExit("rete timing anchor not found")
s = s.replace(rete_anchor, rete_repl, 1)

sweep_anchor = """            applyAndMaybeDrive(*law, *subject, records);\n        }\n    }\n    auto T3 = glfwGetTime();\n"""
sweep_repl = """            applyAndMaybeDrive(*law, *subject, records);\n        }\n        solProfileLaw(\"sweep\", subjects.size());\n    }\n    auto T3 = glfwGetTime();\n"""
if sweep_anchor not in s:
    raise SystemExit("sweep timing anchor not found")
s = s.replace(sweep_anchor, sweep_repl, 1)

p.write_text(s)
print("sol_profile_law_tick.py: Law::applyTo stage profiler applied")

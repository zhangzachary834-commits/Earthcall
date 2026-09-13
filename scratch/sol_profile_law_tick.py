#!/usr/bin/env python3
"""Temporary CI-only LawManager profiler for the Living Instrument lag hunt.

This patches only the runner checkout. Environment switches are diagnostic
ablations, never production semantics.

SOL_PROFILE_AUDIT_OFF=1
    Disable Laws audit enqueueing.
SOL_PROFILE_SKIP_RETE_RECHECK=1
    Clear derived condition closures only after a law has a Rete terminal.
    This is an UPPER BOUND only: some Rete terminals are deliberately broad
    candidates, not proofs, so this is never a production prescription.
SOL_PROFILE_LOG_TRIM_OFF=1
    Leave the per-law application history untrimmed for this short process,
    isolating the cost of vector front-erasure once the 256-entry cap is full.
SOL_PROFILE_STAKEHOLDER_OFF=1
    Skip per-write stakeholder append for this short process, isolating its
    allocation/history cost without changing action execution.
"""
from pathlib import Path

p = Path("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp")
s = p.read_text()

include_anchor = '#include <cstdio>\n'
if include_anchor not in s:
    raise SystemExit("include anchor not found")
s = s.replace(include_anchor, include_anchor + '#include <cstdlib>\n', 1)

# Process-local switches used by Law::applyTo.
apply_anchor = """Law::ApplicationResult Law::applyTo(Singular& target) {\n    ApplicationResult result = ApplicationResult::Applied;\n"""
apply_replacement = """Law::ApplicationResult Law::applyTo(Singular& target) {\n    static const bool solProfileLogTrimOff =\n        std::getenv(\"SOL_PROFILE_LOG_TRIM_OFF\") != nullptr;\n    static const bool solProfileStakeholderOff =\n        std::getenv(\"SOL_PROFILE_STAKEHOLDER_OFF\") != nullptr;\n    ApplicationResult result = ApplicationResult::Applied;\n"""
if apply_anchor not in s:
    raise SystemExit("applyTo anchor not found")
s = s.replace(apply_anchor, apply_replacement, 1)

stakeholder_anchor = """        if (wrote) {\n            for (const auto& node : trace.nodes) {\n                if (node.wrote && !node.path.empty()) {\n                    for (auto* author : _authors.getMembers()) {\n                        if (author) {\n                            target.addStakeholder(node.path, author->getIdentifier(), getIdentifier(), std::time(nullptr));\n                        }\n                    }\n                }\n            }\n        }\n"""
stakeholder_replacement = stakeholder_anchor.replace(
    "        if (wrote) {\n", "        if (wrote && !solProfileStakeholderOff) {\n", 1)
if stakeholder_anchor not in s:
    raise SystemExit("stakeholder anchor not found")
s = s.replace(stakeholder_anchor, stakeholder_replacement, 1)

trim_anchor = """    if (_applicationLog.size() > kMaxLogEntries) {\n        _applicationLog.erase(_applicationLog.begin(),\n                              _applicationLog.end() - kMaxLogEntries);\n    }\n"""
trim_replacement = """    if (!solProfileLogTrimOff && _applicationLog.size() > kMaxLogEntries) {\n        _applicationLog.erase(_applicationLog.begin(),\n                              _applicationLog.end() - kMaxLogEntries);\n    }\n"""
if trim_anchor not in s:
    raise SystemExit("application-log trim anchor not found")
s = s.replace(trim_anchor, trim_replacement, 1)

# Disable only the Laws audit category when explicitly requested.
tick_anchor = """std::vector<Law::ApplicationRecord> LawManager::tick() {\n    static bool printed = false;\n"""
tick_replacement = """std::vector<Law::ApplicationRecord> LawManager::tick() {\n    static const bool solProfileAuditOff = std::getenv(\"SOL_PROFILE_AUDIT_OFF\") != nullptr;\n    if (solProfileAuditOff) {\n        ECA::LawAuditLogger::instance().setLevel(ECA::LawAuditLogger::Level::Off);\n    }\n\n    static bool printed = false;\n"""
if tick_anchor not in s:
    raise SystemExit("tick anchor not found")
s = s.replace(tick_anchor, tick_replacement, 1)

needle = """    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;\n    for (const auto& law : continuousLaws) {\n        if (!law || law->activation() == Law::Activation::OnEvent) continue;\n        if (!law->isEnabled() || !law->isAuthored()) continue;\n\n        const std::string lawId = law->getIdentifier();\n"""
replacement = """    const std::vector<std::shared_ptr<Law>> continuousLaws = _laws;\n    for (const auto& law : continuousLaws) {\n        if (!law || law->activation() == Law::Activation::OnEvent) continue;\n        if (!law->isEnabled() || !law->isAuthored()) continue;\n\n        const std::string lawId = law->getIdentifier();\n        const double solLawProfileT0 = glfwGetTime();\n        const auto solProfileLaw = [&](const char* path, std::size_t candidates) {\n            const double ms = (glfwGetTime() - solLawProfileT0) * 1000.0;\n            std::fprintf(stderr,\n                         \"SOL_LAW_PERF\\t%s\\t%s\\tact=%d\\tpath=%s\\tcandidates=%zu\\tms=%.6f\\n\",\n                         lawId.c_str(), law->name().c_str(),\n                         static_cast<int>(law->activation()), path, candidates, ms);\n        };\n"""
if needle not in s:
    raise SystemExit("continuous-loop anchor not found")
s = s.replace(needle, replacement, 1)

needle = """            for (const Singular* subject : released) {\n                law->rememberConditionState(subject, false);\n                law->forgetOnset(subject);\n            }\n            continue;\n        }\n\n        auto termIt = _reteTerminals.find(lawId);\n"""
replacement = """            for (const Singular* subject : released) {\n                law->rememberConditionState(subject, false);\n                law->forgetOnset(subject);\n            }\n            solProfileLaw(\"gate-false\", 0);\n            continue;\n        }\n\n        auto termIt = _reteTerminals.find(lawId);\n"""
if needle not in s:
    raise SystemExit("gate anchor not found")
s = s.replace(needle, replacement, 1)

needle = """        const bool hasTerminals =\n            _connected && termIt != _reteTerminals.end() && !termIt->second.empty();\n\n\n\n        if (hasTerminals && (law->activation() == Law::Activation::WhileTrue || law->activation() == Law::Activation::OnBecomeTrue)) {\n"""
replacement = """        const bool hasTerminals =\n            _connected && termIt != _reteTerminals.end() && !termIt->second.empty();\n\n        static const bool solProfileSkipReteRecheck =\n            std::getenv(\"SOL_PROFILE_SKIP_RETE_RECHECK\") != nullptr;\n        if (solProfileSkipReteRecheck && hasTerminals) {\n            law->clearConditions();\n        }\n\n        if (hasTerminals && (law->activation() == Law::Activation::WhileTrue || law->activation() == Law::Activation::OnBecomeTrue)) {\n"""
if needle not in s:
    raise SystemExit("terminal ablation anchor not found")
s = s.replace(needle, replacement, 1)

needle = """            for (Singular* subject : subjects) {\n                if (!subject || Universe::instance().isUnmade(subject)) continue;\n                const std::string subjectId = subject->getIdentifier();\n                if (law->drives() &&\n                    hasDriveSession(lawId, subjectId)) {\n                    if (law->retrigger() == Law::Retrigger::Absorb) continue;\n                    restartDriveSession(*law, subjectId);\n                }\n                applyAndMaybeDrive(*law, *subject, records);\n            }\n            continue;\n        }\n\n        // OnBecomeTrue and laws without Rete terminals: full sweep path.\n"""
replacement = """            for (Singular* subject : subjects) {\n                if (!subject || Universe::instance().isUnmade(subject)) continue;\n                const std::string subjectId = subject->getIdentifier();\n                if (law->drives() &&\n                    hasDriveSession(lawId, subjectId)) {\n                    if (law->retrigger() == Law::Retrigger::Absorb) continue;\n                    restartDriveSession(*law, subjectId);\n                }\n                applyAndMaybeDrive(*law, *subject, records);\n            }\n            solProfileLaw(\"rete\", subjects.size());\n            continue;\n        }\n\n        // OnBecomeTrue and laws without Rete terminals: full sweep path.\n"""
if needle not in s:
    raise SystemExit("rete anchor not found")
s = s.replace(needle, replacement, 1)

needle = """            applyAndMaybeDrive(*law, *subject, records);\n        }\n    }\n    auto T3 = glfwGetTime();\n"""
replacement = """            applyAndMaybeDrive(*law, *subject, records);\n        }\n        solProfileLaw(\"sweep\", subjects.size());\n    }\n    auto T3 = glfwGetTime();\n"""
if needle not in s:
    raise SystemExit("sweep anchor not found")
s = s.replace(needle, replacement, 1)

p.write_text(s)
print("sol_profile_law_tick.py: instrumentation + history ablations applied")

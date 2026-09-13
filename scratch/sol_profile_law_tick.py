#!/usr/bin/env python3
"""Temporary CI-only LawManager profiler for the Living Instrument lag hunt.

This script patches the runner checkout, not the committed engine source. It is
intentionally loud: every continuous Law reports its path, candidate count and
wall time so the artifact can be aggregated after the benchmark.

Two opt-in *process-local* ablations let CI measure costs without changing the
committed engine semantics:

  SOL_PROFILE_AUDIT_OFF=1
      Turns the Laws audit category off for that benchmark process.

  SOL_PROFILE_SKIP_RETE_RECHECK=1
      For continuous laws that already have live Rete terminals, clears the
      derived condition predicates after terminal membership has been decided.
      The authored ConditionModel and compiled Rete network remain intact, so
      this isolates the duplicate Law::applyTo() condition re-evaluation cost.
      This is a profiler-only mutation and must never be treated as production
      implementation.
"""
from pathlib import Path

p = Path("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp")
s = p.read_text()

# getenv for the CI-only ablation switches.
include_anchor = '#include <cstdio>\n'
if include_anchor not in s:
    raise SystemExit("include anchor not found")
s = s.replace(include_anchor, include_anchor + '#include <cstdlib>\n', 1)

# Disable only the Laws audit category, and only when the benchmark process
# explicitly asks for the ablation. This occurs before any tick work so the
# measurement includes the same world/law execution but excludes audit enqueue.
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

# The Rete terminal is already the result of the authored condition model. For
# this *measurement only*, remove the derived predicate closure so applyTo does
# not calculate the same truth a second time. Each benchmark invocation is a
# fresh process, so this mutation cannot leak between the baseline/ablations.
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
print("sol_profile_law_tick.py: instrumentation + ablations applied")

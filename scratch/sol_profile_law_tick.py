#!/usr/bin/env python3
"""Temporary CI-only LawManager profiler for the Living Instrument lag hunt.

This script patches the runner checkout, not the committed engine source.  It is
intentionally loud: every continuous Law reports its path, candidate count and
wall time so the artifact can be aggregated after the benchmark.
"""
from pathlib import Path

p = Path("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp")
s = p.read_text()

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
print("sol_profile_law_tick.py: instrumentation applied")

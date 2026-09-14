#!/usr/bin/env python3
"""CI-only causal patch for the Event-as-Moment Living Instrument benchmark.

Authored state-condition alphas accept state facts; they are not evidence that
some Law hears every arbitrary Event. Preserve explicit typed listeners through
hearsType(type) and preserve fail-open behavior for genuinely foreign closures.
"""
from pathlib import Path

path = Path("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp")
text = path.read_text()

old = """    Universe::instance().setEventInterest([this](const std::string& type) {\n        return _rete.hearsType(type) || _rete.hasOpaqueBoundAlpha();\n    });\n"""
new = """    Universe::instance().setEventInterest([this](const std::string& type) {\n        return _rete.hearsType(type) || _rete.hasForeignBoundAlpha();\n    });\n"""

if old not in text:
    raise SystemExit("event-interest callback anchor not found; source moved")

text = text.replace(old, new, 1)
path.write_text(text)
print("sol_event_interest_fastpath.py: authored state alphas no longer impersonate arbitrary Event listeners")

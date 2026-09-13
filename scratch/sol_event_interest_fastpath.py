#!/usr/bin/env python3
"""CI-only causal probe for the Living Instrument event-interest tax.

The production callback currently treats every non-interned alpha as a possible
listener for every event type. Authored condition alphas are non-interned but
accept state facts only, so that fail-open answer makes generic `law-applied`
echoes fire in authored worlds even when no Law listens for that event.

This probe changes ONLY the event-interest callback from hasOpaqueBoundAlpha()
to hasForeignBoundAlpha(). Foreign hand-written predicates still fail open;
explicit interned event triggers are still handled by hearsType(type).
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
print("sol_event_interest_fastpath.py: authored state alphas no longer impersonate arbitrary event listeners")

"""Where Earthcall's save data lives — import this instead of writing "saves/".

    from earthcall_data_home import data_root
    law = data_root() / "laws" / "my-law" / "law.json"

data_root() is the choice the engine makes at boot (SaveSystem::chooseDataRoot,
keep both in step): the Person's data home once it carries data-home.json
(macOS ~/Library/Application Support/Earthcall; $EARTHCALL_DATA overrides),
else the checkout's saves/ until scripts/migrate_saves_to_data_home.py has run.
Save-file injections must still patch, never regenerate (FIRST_MOVER_AUTHORING
§7 rule 8), and the data home is its own git repository — commit there.
"""
from __future__ import annotations

import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CHECKOUT_SAVES = REPO / "saves"
MARKER = "data-home.json"


def platform_data_home() -> Path | None:
    env = os.environ.get("EARTHCALL_DATA")
    if env:
        return Path(env)
    if sys.platform == "win32":
        appdata = os.environ.get("APPDATA")
        return Path(appdata) / "Earthcall" if appdata else None
    home = os.environ.get("HOME")
    if sys.platform == "darwin":
        return Path(home) / "Library" / "Application Support" / "Earthcall" if home else None
    xdg = os.environ.get("XDG_DATA_HOME")
    if xdg:
        return Path(xdg) / "earthcall"
    return Path(home) / ".local" / "share" / "earthcall" if home else None


def data_root() -> Path:
    home = platform_data_home()
    if home is not None and (home / MARKER).is_file():
        return home
    return CHECKOUT_SAVES


if __name__ == "__main__":
    print(data_root())

import re

with open("docs/Agenda/Tasks/To-do list.md", "r") as f:
    text = f.read()

# Add the Ambient Zone success to the Production-facing Programs
ambient_bullet = "- ✅ **Ambient Zone Spatial Audio (2026-10-07)** — done and verified (2026-10-07; Zach verified native panning): Authored a pure-data 140Hz sine wave muffled environmental sound (`panning-sound-orb`) and a mathematical `law-ambient-pan` Law to physically oscillate its position, natively driving CoreAudio spatial left-to-right panning without any custom C++ classes. -> [full task](Specific%20Tasks/Audio%20and%20Spatialization/Ambient_Zone_Spatial_Audio/Ambient_Zone_Spatial_Audio.md)\n"

# Insert under ## Production-facing Programs:
if "## Production-facing Programs:" in text:
    text = text.replace("## Production-facing Programs:\n", "## Production-facing Programs:\n" + ambient_bullet)
else:
    text += "\n## Production-facing Programs:\n" + ambient_bullet

with open("docs/Agenda/Tasks/To-do list.md", "w") as f:
    f.write(text)


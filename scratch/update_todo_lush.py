with open("docs/Agenda/Tasks/To-do list.md", "r") as f:
    text = f.read()

text = text.replace(
    "Authored a pure-data 140Hz sine wave muffled environmental sound (`panning-sound-orb`) and a mathematical `law-ambient-pan` Law to physically oscillate its position",
    "Authored a lush generative chord (A2, E3, B3) using 3 pure-data muffled sine wave orbs and 3 separate mathematical panning Laws to swirl them independently"
)

with open("docs/Agenda/Tasks/To-do list.md", "w") as f:
    f.write(text)

with open("docs/Agenda/Tasks/Specific Tasks/Audio and Spatialization/Ambient_Zone_Spatial_Audio/Ambient_Zone_Spatial_Audio.md", "r") as f:
    spec = f.read()

spec = spec.replace(
    "A `panning-sound-orb` spherical Object was seeded",
    "Three spherical Objects (Root, Fifth, Ninth) were seeded"
).replace(
    "`acoustic.frequency = 140.0` (Deep, resonant hum around C#3/D3)",
    "Frequencies for a lush major ninth chord (A2: 110Hz, E3: 164.81Hz, B3: 246.94Hz)"
).replace(
    "Rather than building a fake \"panning\" property, a mathematical Law (`saves/laws/law-ambient-pan`) was authored",
    "Rather than building a fake \"panning\" property, three mathematical Laws were authored (`law-ambient-pan-root`, `-fifth`, `-ninth`)"
)

with open("docs/Agenda/Tasks/Specific Tasks/Audio and Spatialization/Ambient_Zone_Spatial_Audio/Ambient_Zone_Spatial_Audio.md", "w") as f:
    f.write(spec)

print("Updated docs!")

#!/bin/bash
sed -i '' 's/void render3DConsole(Person\* player, Object\* selectedObject3D)/void render3DConsole(Person* player, Object* selectedObject3D, Core::Engine* engine)/' src/Singularity/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp

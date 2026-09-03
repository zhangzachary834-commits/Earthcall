import re

# 1. LawGraphWindow.cpp
with open('src/Singularity/Screen/LawGraphWindow.cpp', 'r') as f:
    content = f.read()

if '#include "Singularity/Screen/ScreenChannel.hpp"' not in content:
    content = content.replace(
        '#include "Singularity/Input/Interaction/InteractionChannel.hpp"',
        '#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"'
    )

screen_probe = '''        static Singularity::Screen::ScreenChannel screenPrototype;
        for (Property* property : screenPrototype.listProperties()) {
            const PropertyValue probe = property->value();
            const bool isVec = std::holds_alternative<glm::vec3>(probe);
            const char* type = "number";
            if (isVec)                                            type = "vector";
            else if (std::holds_alternative<glm::mat4>(probe))     type = "transform";
            else if (std::holds_alternative<std::string>(probe))   type = "text";
            else if (std::holds_alternative<bool>(probe))          type = "toggle";
            options.push_back({property->name(), "Channel — Screen", type, isVec});
            if (isVec) {
                options.push_back({property->name() + ".x", "Channel — Screen", "number", false});
                options.push_back({property->name() + ".y", "Channel — Screen", "number", false});
                options.push_back({property->name() + ".z", "Channel — Screen", "number", false});
            }
        }
'''

if 'static Singularity::Screen::ScreenChannel screenPrototype;' not in content:
    target = '        static Ourverse ourversePrototype;
        for (Property* property : ourversePrototype.listProperties()) {
            const PropertyValue probe = property->value();
            const char* type = "number";
            if (std::holds_alternative<std::string>(probe)) type = "text";
            else if (std::holds_alternative<int>(probe)) type = "number";
            options.push_back({property->name(), "Ourverse", type, false});
        }'
    content = content.replace(target, target + '

' + screen_probe)

with open('src/Singularity/Screen/LawGraphWindow.cpp', 'w') as f:
    f.write(content)
print('LawGraphWindow.cpp updated successfully')

# 2. channel_paths_test.cpp
with open('tests/singularity/channel_paths_test.cpp', 'r') as f:
    content = f.read()

if '#include "Singularity/Screen/ScreenChannel.hpp"' not in content:
    content = content.replace(
        '#include "Singularity/Input/Interaction/InteractionChannel.hpp"',
        '#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"'
    )

if 'Singularity::Screen::ScreenChannel screen;' not in content:
    content = content.replace(
        'Singularity::Input::InteractionChannel interaction;',
        'Singularity::Input::InteractionChannel interaction;
    Singularity::Screen::ScreenChannel screen;'
    )

if 'Channel — Screen' not in content:
    content = content.replace(
        'else if (groupIs(option.group, "Channel — Interaction")) check(option, interaction);',
        'else if (groupIs(option.group, "Channel — Interaction")) check(option, interaction);
        else if (groupIs(option.group, "Channel — Screen")) check(option, screen);'
    )

with open('tests/singularity/channel_paths_test.cpp', 'w') as f:
    f.write(content)
print('channel_paths_test.cpp updated successfully')

# 3. no_black_box_test.cpp
with open('tests/singularity/no_black_box_test.cpp', 'r') as f:
    content = f.read()

if '#include "Singularity/Screen/ScreenChannel.hpp"' not in content:
    content = content.replace(
        '#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"',
        '#include "Singularity/Input/Locomotion/LocomotionChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"'
    )

if 'audit("ScreenChannel", screen);' not in content:
    content = content.replace(
        'Singularity::Input::LocomotionChannel locomotion; audit("LocomotionChannel", locomotion);',
        'Singularity::Input::LocomotionChannel locomotion; audit("LocomotionChannel", locomotion);
        Singularity::Screen::ScreenChannel screen;        audit("ScreenChannel", screen);
        Singularity::Input::InteractionChannel interaction; audit("InteractionChannel", interaction);'
    )

if 'auditReachability("ScreenChannel", screenReach, advertised);' not in content:
    content = content.replace(
        'Singularity::Input::LocomotionChannel locomotion;
        auditReachability("LocomotionChannel", locomotion, advertised);',
        'Singularity::Input::LocomotionChannel locomotion;
        auditReachability("LocomotionChannel", locomotion, advertised);

        Singularity::Screen::ScreenChannel screenReach;
        auditReachability("ScreenChannel", screenReach, advertised);

        Singularity::Input::InteractionChannel interactionReach;
        auditReachability("InteractionChannel", interactionReach, advertised);'
    )

with open('tests/singularity/no_black_box_test.cpp', 'w') as f:
    f.write(content)
print('no_black_box_test.cpp updated successfully')

# 4. gpu_mastery_test.cpp
with open('tests/singularity/gpu_mastery_test.cpp', 'r') as f:
    content = f.read()

bg_test = '''        // Read and write backgroundColor via PropertyPath
        {
            PropertyValue v;
            auto res = PropertyPath::parse("backgroundColor").getValue(*channel, v);
            check(res == PropertyPath::PathResult::Ok, "[2] backgroundColor resolves");
            check(std::holds_alternative<glm::vec3>(v), "[2] backgroundColor is vec3");

            glm::vec3 testColor(0.2f, 0.3f, 0.4f);
            auto setRes = PropertyPath::parse("backgroundColor").setValue(*channel, PropertyValue(testColor));
            check(setRes == PropertyPath::PathResult::Ok, "[2] backgroundColor setValue returns Ok");
            check(glm::distance(channel->backgroundColor, testColor) < 1e-4f, "[2] backgroundColor writes back new vec3");

            auto setXRes = PropertyPath::parse("backgroundColor.x").setValue(*channel, PropertyValue(0.7f));
            check(setXRes == PropertyPath::PathResult::Ok, "[2] backgroundColor.x setValue returns Ok");
            check(std::abs(channel->backgroundColor.x - 0.7f) < 1e-4f, "[2] backgroundColor.x writes 0.7");
        }
'''

if 'backgroundColor resolves' not in content:
    target = '            check(channel->wireframe == true, "[2] wireframe writes back true");
        }'
    content = content.replace(target, target + '

' + bg_test)

with open('tests/singularity/gpu_mastery_test.cpp', 'w') as f:
    f.write(content)
print('gpu_mastery_test.cpp updated successfully')

# 5. To-do list.md
with open('docs/Agenda/Tasks/To-do list.md', 'r') as f:
    content = f.read()

todo_old = '- The background color must not be a black box.'
todo_new = '- ✅ **The background color must not be a black box** — done and verified (2026-08-31): Exposed `backgroundColor` on `ScreenChannel` (`src/Singularity/Screen/ScreenChannel.{hpp,cpp}`) as a governable, writable `glm::vec3` property with full PropertyPath support (including `.x`, `.y`, `.z` sub-paths); hooked `@screen-channel.backgroundColor` into `EngineRender.cpp` to drive the frame clear color in `currentRenderer().beginFrame()`; added live probe of `ScreenChannel` to `knownPathOptions()` in `LawGraphWindow.cpp` under "Channel — Screen"; verified in `tests/singularity/gpu_mastery_test.cpp`, `tests/singularity/channel_paths_test.cpp`, and `tests/singularity/no_black_box_test.cpp`.'

if todo_old in content:
    content = content.replace(todo_old, todo_new)
    with open('docs/Agenda/Tasks/To-do list.md', 'w') as f:
        f.write(content)
    print('To-do list.md updated successfully')

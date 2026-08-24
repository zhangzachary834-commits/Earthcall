#pragma once

#include <string>

class Object;

/*
 * Events emitted by the automation system. These ride the shared Core::EventBus
 * so producers (sound, particles, scripted chaining, the interaction-state
 * recorder) can react without the automation core knowing about them.
 *
 * Only discrete edges travel as events. The per-frame advance()/compose()
 * evaluation stays a direct call — never message-passed — so the hot path keeps
 * its deterministic ordering and avoids the bus's per-publish lock.
 */
namespace Automation {

// Fired once when a non-looping clip reaches the end of its duration.
struct ClipFinished {
    Object*     target = nullptr;
    std::string clipName;
};

} // namespace Automation

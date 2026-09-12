import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

target = """    // 1. First sweep: compile Rete bindings into application bounds.
    //    If a target hasn't met the condition or left the zone, discard.
    auto activeZone = ZoneManager::instance().activeZone();
    for (const auto& entry : _rete.matchSets()) {
        Law* law = entry.first;
        if (law->activation() != Law::Activation::WhileTrue) continue;

        // Ensure Rete terminals have swept over the world.
        law->ensureReteTerminalsCollected(*this);

        for (Singular* subject : entry.second) {
            if (subject->getZoneId() != activeZone->getIdentifier()) continue;
            // The Rete guarantees structural match, but mathematical properties
            // (e.g. range, thresholds) may have drifted out of bounds.
            if (!law->conditionsSatisfied(*subject)) continue;

            const bool wasHolding = law->lastConditionState(subject);
            law->rememberConditionState(subject, true);
            if (!wasHolding && Universe::instance().hasClock()) {
                law->rememberOnset(subject, Universe::instance().now());
            }

            applyAndMaybeDrive(*law, *subject, records);
        }
    }"""

replacement = """    // 1. First sweep: compile Rete bindings into application bounds.
    //    If a target hasn't met the condition or left the zone, discard.
    auto activeZone = ZoneManager::instance().activeZone();
    auto tR1 = glfwGetTime();
    int reteExecs = 0;
    for (const auto& entry : _rete.matchSets()) {
        Law* law = entry.first;
        if (law->activation() != Law::Activation::WhileTrue) continue;

        // Ensure Rete terminals have swept over the world.
        law->ensureReteTerminalsCollected(*this);

        for (Singular* subject : entry.second) {
            if (subject->getZoneId() != activeZone->getIdentifier()) continue;
            // The Rete guarantees structural match, but mathematical properties
            // (e.g. range, thresholds) may have drifted out of bounds.
            if (!law->conditionsSatisfied(*subject)) continue;

            const bool wasHolding = law->lastConditionState(subject);
            law->rememberConditionState(subject, true);
            if (!wasHolding && Universe::instance().hasClock()) {
                law->rememberOnset(subject, Universe::instance().now());
            }

            auto t0 = glfwGetTime();
            applyAndMaybeDrive(*law, *subject, records);
            auto t1 = glfwGetTime();
            reteExecs++;
            if ((t1 - t0) * 1000.0 > 0.5) {
                printf("[RETE_EVAL] Law '%s' on '%s' took %.2f ms\\n", law->getIdentifier().c_str(), subject->getIdentifier().c_str(), (t1 - t0) * 1000.0);
            }
        }
    }
    auto tR2 = glfwGetTime();
    if ((tR2 - tR1) * 1000.0 > 1.0) {
        printf("[RETE_TOTAL] %d matches evaluated in %.2f ms\\n", reteExecs, (tR2 - tR1) * 1000.0);
    }"""

if target in content:
    content = content.replace(target, replacement)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched Rete sweep!")
else:
    print("Could not find Rete sweep target block")

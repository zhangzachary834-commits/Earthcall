// Regression witness for the Law Graph's ActionNode success/failure projection.
// No window or GPU is needed: the production UI uses this exact feed reducer.
#include "Singularity/Screen/ActionNodeFeed.hpp"

#include <cassert>
#include <string>
#include <vector>

int main() {
    using Rendering::ActionNodeFeedEntry;
    using Rendering::recordActionNodeFeed;
    std::vector<ActionNodeFeedEntry> feed;

    recordActionNodeFeed(feed, "Set", "object-a", true);
    recordActionNodeFeed(feed, "Set", "object-a", true);
    assert(feed.size() == 1 && feed.front().success && feed.front().count == 2);

    recordActionNodeFeed(feed, "Set", "object-a", false, "MissingProperty");
    assert(feed.size() == 2 && !feed.front().success);
    assert(feed.front().reason == "MissingProperty" && feed.front().count == 1);
    recordActionNodeFeed(feed, "Set", "object-a", false, "MissingProperty");
    assert(feed.size() == 2 && feed.front().count == 2);

    recordActionNodeFeed(feed, "Set", "object-a", false, "ReadOnly");
    assert(feed.size() == 3 && feed.front().count == 1);
    assert(feed.front().reason == "ReadOnly");

    recordActionNodeFeed(feed, "Set", "object-a", true, "ignored on success");
    assert(feed.size() == 4 && feed.front().success && feed.front().reason.empty());
    assert(feed.front().count == 1);

    for (int i = 0; i < 12; ++i) {
        recordActionNodeFeed(feed, "action-" + std::to_string(i), "object-a", true);
    }
    assert(feed.size() == 8);
    assert(feed.front().actionName == "action-11");
    assert(feed.back().actionName == "action-4");
    return 0;
}

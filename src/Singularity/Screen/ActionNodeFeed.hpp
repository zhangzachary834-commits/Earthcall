#pragma once

#include <string>
#include <utility>
#include <vector>

namespace Rendering {

// A bounded projection of ActionNode events, not a second authority for Law outcomes.
struct ActionNodeFeedEntry {
    std::string actionName;
    std::string targetId;
    int count = 1;
    bool success = true;
    std::string reason;
};

// Only adjacent events with the same outcome and failure reason coalesce.
// A failed write must never be counted as a successful execution.
inline void recordActionNodeFeed(std::vector<ActionNodeFeedEntry>& feed,
                                 std::string actionName,
                                 std::string targetId,
                                 bool success,
                                 std::string reason = {}) {
    if (success) reason.clear();
    if (!feed.empty() &&
        feed.front().actionName == actionName &&
        feed.front().targetId == targetId &&
        feed.front().success == success &&
        feed.front().reason == reason) {
        ++feed.front().count;
        return;
    }

    feed.insert(feed.begin(), {std::move(actionName), std::move(targetId),
                               1, success, std::move(reason)});
    if (feed.size() > 8) feed.pop_back();
}

} // namespace Rendering

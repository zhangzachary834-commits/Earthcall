#pragma once

namespace Singularity {
namespace Physical {
namespace Adapters {

// Adapters provide pure wire protocol implementations and translate to the shared vocabulary.
class SerialAdapter {
public:
    SerialAdapter();
    ~SerialAdapter();

    void connect(const char* port);
    void disconnect();
    void update();
};

} // namespace Adapters
} // namespace Physical
} // namespace Singularity

#include "SerialAdapter.hpp"

namespace Singularity {
namespace Physical {
namespace Adapters {

SerialAdapter::SerialAdapter() {}
SerialAdapter::~SerialAdapter() {}

void SerialAdapter::connect(const char* port) {
    // Basic wire protocol implementation to be added
    (void)port;
}

void SerialAdapter::disconnect() {
}

void SerialAdapter::update() {
}

} // namespace Adapters
} // namespace Physical
} // namespace Singularity

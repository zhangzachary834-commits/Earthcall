#include "OscChannel.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"


#define ASIO_STANDALONE
#include <asio.hpp>
#include <vector>
#include <cstring>

namespace Singularity {
namespace Network {
namespace Osc {

OscChannel::OscChannel() : Law("osc-channel") {}

void OscChannel::syncRegister(LawManager& laws) {
    if (!laws.find("osc-channel")) {
        laws.add(std::make_shared<OscChannel>());
    }
}

void OscChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<OscChannel, std::string>>(
        "host", this, &OscChannel::propHost, &OscChannel::propSetHost));
    registerProperty(std::make_unique<ComputedProperty<OscChannel, double>>(
        "port", this, &OscChannel::propPort, &OscChannel::propSetPort));
    registerProperty(std::make_unique<ComputedProperty<OscChannel, std::string>>(
        "address", this, &OscChannel::propAddress, &OscChannel::propSetAddress));
    registerProperty(std::make_unique<ComputedProperty<OscChannel, double>>(
        "floatArg", this, &OscChannel::propFloatArg, &OscChannel::propSetFloatArg));
    registerProperty(std::make_unique<ComputedProperty<OscChannel, double>>(
        "sendRequests", this, &OscChannel::propSendRequests, &OscChannel::propSetSendRequests));
}

void OscChannel::propSetSendRequests(const double& v) {
    if (v > _sendRequests) {
        try {
            asio::io_context io_context;
            asio::ip::udp::socket socket(io_context);
            socket.open(asio::ip::udp::v4());
            
            asio::ip::udp::resolver resolver(io_context);
            asio::ip::udp::resolver::results_type endpoints =
                resolver.resolve(asio::ip::udp::v4(), _host, std::to_string(static_cast<int>(_port)));
            
            // Build OSC message manually
            std::vector<char> buffer;
            
            // 1. Address pattern, null terminated, padded to 4 bytes
            std::string addr = _address;
            buffer.insert(buffer.end(), addr.begin(), addr.end());
            buffer.push_back(0); // null term
            while (buffer.size() % 4 != 0) {
                buffer.push_back(0); // padding
            }
            
            // 2. Type tag string: ",f" (for one float), padded
            std::string types = ",f";
            buffer.insert(buffer.end(), types.begin(), types.end());
            buffer.push_back(0);
            while (buffer.size() % 4 != 0) {
                buffer.push_back(0);
            }
            
            // 3. Float argument, big-endian
            float fArg = static_cast<float>(_floatArg);
            uint32_t fBits;
            std::memcpy(&fBits, &fArg, sizeof(float));
            // convert to big-endian if host is little-endian
            uint32_t fBitsBE = htonl(fBits);
            
            char* fPtr = reinterpret_cast<char*>(&fBitsBE);
            buffer.insert(buffer.end(), fPtr, fPtr + sizeof(uint32_t));
            
            // Send
            socket.send_to(asio::buffer(buffer), *endpoints.begin());
            
        } catch (const std::exception& e) {
            // If DNS resolution fails, just drop the OSC message silently 
            // as is standard for OSC. 
        }
    }
    _sendRequests = v;
}

} // namespace Osc
} // namespace Network
} // namespace Singularity

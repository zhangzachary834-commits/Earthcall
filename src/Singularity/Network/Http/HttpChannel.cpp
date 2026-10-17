#include "HttpChannel.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"


#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../../../../third_party/httplib/httplib.h"
#include <iostream>
#include <sstream>

namespace Singularity {
namespace Network {
namespace Http {

HttpChannel::HttpChannel() : Law("http-channel") {}

void HttpChannel::syncRegister(LawManager& laws) {
    if (!laws.find("http-channel")) {
        laws.add(std::make_shared<HttpChannel>());
    }
}

void HttpChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, std::string>>(
        "url", this, &HttpChannel::propUrl, &HttpChannel::propSetUrl));
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, std::string>>(
        "method", this, &HttpChannel::propMethod, &HttpChannel::propSetMethod));
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, std::string>>(
        "body", this, &HttpChannel::propBody, &HttpChannel::propSetBody));
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, std::string>>(
        "headers", this, &HttpChannel::propHeaders, &HttpChannel::propSetHeaders));
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, double>>(
        "sendRequests", this, &HttpChannel::propSendRequests, &HttpChannel::propSetSendRequests));
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, std::string>>(
        "responseBody", this, &HttpChannel::propResponseBody));
    registerProperty(std::make_unique<ComputedProperty<HttpChannel, double>>(
        "statusCode", this, &HttpChannel::propStatusCode));
}

void HttpChannel::propSetSendRequests(const double& v) {
    if (v > _sendRequests) {
        _responseBody.clear();
        _statusCode = 0.0;
        
        try {
            // Basic URL parsing
            std::string host = _url;
            std::string path = "/";
            
            size_t schemePos = host.find("://");
            std::string scheme = "http";
            if (schemePos != std::string::npos) {
                scheme = host.substr(0, schemePos);
                host = host.substr(schemePos + 3);
            }
            
            size_t pathPos = host.find("/");
            if (pathPos != std::string::npos) {
                path = host.substr(pathPos);
                host = host.substr(0, pathPos);
            }
            
            std::string fullHost = scheme + "://" + host;
            httplib::Client cli(fullHost.c_str());
            
            // disable cert verification for simple First Mover prototyping (not prod)
            cli.enable_server_certificate_verification(false);
            
            // Parse headers
            httplib::Headers reqHeaders;
            std::istringstream hStream(_headers);
            std::string line;
            while (std::getline(hStream, line)) {
                size_t colonPos = line.find(":");
                if (colonPos != std::string::npos) {
                    std::string key = line.substr(0, colonPos);
                    std::string val = line.substr(colonPos + 1);
                    // trim whitespace
                    if (!val.empty()) {
                        val.erase(0, val.find_first_not_of(" \t\r\n"));
                        size_t endpos = val.find_last_not_of(" \t\r\n");
                        if(endpos != std::string::npos) val.erase(endpos + 1);
                    }
                    if (!key.empty()) {
                        size_t endpos = key.find_last_not_of(" \t\r\n");
                        if(endpos != std::string::npos) key.erase(endpos + 1);
                    }
                    reqHeaders.insert({key, val});
                }
            }
            
            httplib::Result res;
            if (_method == "GET") {
                res = cli.Get(path.c_str(), reqHeaders);
            } else if (_method == "POST") {
                std::string contentType = "application/json";
                for (const auto& h : reqHeaders) {
                    if (h.first == "Content-Type") {
                        contentType = h.second;
                        break;
                    }
                }
                res = cli.Post(path.c_str(), reqHeaders, _body, contentType.c_str());
            } else if (_method == "PUT") {
                std::string contentType = "application/json";
                for (const auto& h : reqHeaders) {
                    if (h.first == "Content-Type") {
                        contentType = h.second;
                        break;
                    }
                }
                res = cli.Put(path.c_str(), reqHeaders, _body, contentType.c_str());
            } else if (_method == "DELETE") {
                res = cli.Delete(path.c_str(), reqHeaders);
            }
            
            if (res) {
                _statusCode = res->status;
                _responseBody = res->body;
            } else {
                _statusCode = -1.0;
                _responseBody = httplib::to_string(res.error());
            }
        } catch (const std::exception& e) {
            _statusCode = -2.0;
            _responseBody = e.what();
        }
    }
    _sendRequests = v;
}

} // namespace Http
} // namespace Network
} // namespace Singularity

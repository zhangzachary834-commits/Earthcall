#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Singularity {
namespace Network {
namespace Http {

class HttpChannel : public Law {
public:
    HttpChannel();
    ~HttpChannel() override = default;

    bool isFirstMover() const override { return true; }

    static void syncRegister(LawManager& laws);

private:
    void buildProperties() override;

    std::string propUrl() const { return _url; }
    void propSetUrl(const std::string& v) { _url = v; }

    std::string propMethod() const { return _method; }
    void propSetMethod(const std::string& v) { _method = v; }

    std::string propBody() const { return _body; }
    void propSetBody(const std::string& v) { _body = v; }

    std::string propHeaders() const { return _headers; }
    void propSetHeaders(const std::string& v) { _headers = v; }

    double propSendRequests() const { return _sendRequests; }
    void propSetSendRequests(const double& v);

    std::string propResponseBody() const { return _responseBody; }
    double propStatusCode() const { return _statusCode; }

    std::string _url;
    std::string _method = "GET";
    std::string _body;
    std::string _headers;
    double _sendRequests = 0.0;
    
    std::string _responseBody;
    double _statusCode = 0.0;
};

} // namespace Http
} // namespace Network
} // namespace Singularity

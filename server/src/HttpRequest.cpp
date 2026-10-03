#include "HttpRequest.h"

#include <sstream>

HttpRequest::HttpRequest(const std::string& request) {
    std::istringstream stream(request);

    // Parse request line
    if (!(stream >> method >> path >> version)) {
        return;
    }

    // Basic request-line validation
    if (version != "HTTP/1.1" || method.empty() || path.empty()) {
        return;
    }

    std::string line;

    // Move to the beginning of the headers
    std::getline(stream, line);

    while (std::getline(stream, line)) {
        if (line == "\r" || line.empty()) {
            break;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::size_t colon = line.find(':');

        if (colon == std::string::npos) {
            return;
        }

        std::string name = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (!value.empty() && value.front() == ' ') {
            value.erase(0, 1);
        }

        headers[name] = value;
    }

    // Find the body
    std::string::size_type bodyStart = request.find("\r\n\r\n");

    if (bodyStart != std::string::npos) {
        body = request.substr(bodyStart + 4);
    }

    valid = true;
}

const std::string& HttpRequest::getMethod() const {
    return method;
}

const std::string& HttpRequest::getPath() const {
    return path;
}

const std::string& HttpRequest::getVersion() const {
    return version;
}

const std::string& HttpRequest::getBody() const {
    return body;
}

std::string HttpRequest::getHeader(const std::string& name) const {
    auto it = headers.find(name);

    if (it != headers.end()) {
        return it->second;
    }

    return "";
}

std::size_t HttpRequest::getContentLength() const {
    auto it = headers.find("Content-Length");

    if (it != headers.end()) {
        return std::stoul(it->second);
    }

    return 0;
}

bool HttpRequest::isValid() const {
    return valid;
}
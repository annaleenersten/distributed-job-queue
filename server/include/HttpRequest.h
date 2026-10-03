#pragma once

#include <string>
#include <unordered_map>

class HttpRequest {
public:
    explicit HttpRequest(const std::string& request);

    const std::string& getMethod() const;
    const std::string& getPath() const;
    const std::string& getVersion() const;
    const std::string& getBody() const;

    std::string getHeader(const std::string& name) const;
    std::size_t getContentLength() const;
    bool isValid() const;

private:
    std::string method;
    std::string path;
    std::string version;
    std::string body;

    std::unordered_map<std::string, std::string> headers;

    bool valid = false;
};
#pragma once

#include <string>

class HttpResponse {
public:
    HttpResponse(int statusCode, const std::string& statusText,
                 const std::string& body);

    int getStatusCode() const;
    const std::string& getStatusText() const;
    const std::string& getBody() const;

private:
    int statusCode;
    std::string statusText;
    std::string body;
};
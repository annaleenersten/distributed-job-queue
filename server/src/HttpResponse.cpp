#include "HttpResponse.h"

HttpResponse::HttpResponse(
    int statusCode,
    const std::string& statusText,
    const std::string& body
) : statusCode(statusCode),
    statusText(statusText),
    body(body) {
}

int HttpResponse::getStatusCode() const {
    return statusCode;
}

const std::string& HttpResponse::getStatusText() const {
    return statusText;
}

const std::string& HttpResponse::getBody() const {
    return body;
}
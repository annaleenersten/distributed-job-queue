#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Cache.h"

#include <string>

class Router {
public:
    Router(Cache& cache, std::string databaseConnectionString = {});

    HttpResponse route(const HttpRequest& request) const;

private:
    Cache& cache;
    std::string databaseConnectionString;
};
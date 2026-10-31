#include "Router.h"

#include <chrono>

Router::Router(Cache& cache, JobQueueSystem* jobQueue)
    : cache(cache),
      jobQueue(jobQueue) {
}

HttpResponse Router::route(const HttpRequest& request) const {
    if (!request.isValid()) {
        return HttpResponse(
            400,
            "Bad Request",
            "Invalid HTTP request"
        );
    }

    if (request.getPath() == "/") {
        if (request.getMethod() != "GET") {
            return HttpResponse(
                405,
                "Method Not Allowed",
                "Method not allowed"
            );
        }

        return HttpResponse(
            200,
            "OK",
            "Hello, World!"
        );
    }

    if (request.getPath() == "/hello") {
        if (request.getMethod() != "GET") {
            return HttpResponse(
                405,
                "Method Not Allowed",
                "Method not allowed"
            );
        }

        return HttpResponse(
            200,
            "OK",
            "Hello from the server!"
        );
    }

    if (request.getPath() == "/about") {
        if (request.getMethod() != "GET") {
            return HttpResponse(
                405,
                "Method Not Allowed",
                "Method not allowed"
            );
        }

        return HttpResponse(
            200,
            "OK",
            "C++ Concurrent Cache Server"
        );
    }

    if (request.getPath() == "/stats") {
        if (request.getMethod() != "GET") {
            return HttpResponse(
                405,
                "Method Not Allowed",
                "Method not allowed"
            );
        }

        std::string body =
            "Hits: " + std::to_string(cache.getHits()) +
            "\nMisses: " + std::to_string(cache.getMisses());

        return HttpResponse(
            200,
            "OK",
            body
        );
    }

    if (request.getPath().rfind("/cache/", 0) == 0) {
        std::string key = request.getPath().substr(7);

        if (key.empty()) {
            return HttpResponse(
                400,
                "Bad Request",
                "Key cannot be empty"
            );
        }

        if (request.getMethod() == "PUT") {
            cache.set(
                key,
                request.getBody(),
                std::chrono::seconds(60)
            );

            return HttpResponse(
                200,
                "OK",
                "Value stored"
            );
        }

        if (request.getMethod() == "GET") {
            auto value = cache.get(key);

            if (!value.has_value()) {
                return HttpResponse(
                    404,
                    "Not Found",
                    "Key not found"
                );
            }

            return HttpResponse(
                200,
                "OK",
                value.value()
            );
        }

        if (request.getMethod() == "DELETE") {
            if (cache.remove(key)) {
                return HttpResponse(
                    200,
                    "OK",
                    "Value deleted"
                );
            }

            return HttpResponse(
                404,
                "Not Found",
                "Key not found"
            );
        }

        return HttpResponse(
            405,
            "Method Not Allowed",
            "Method not allowed"
        );
    }

    if (request.getPath() == "/jobs") {
        if (request.getMethod() != "POST") {
            return HttpResponse(
                405,
                "Method Not Allowed",
                "Method not allowed"
            );
        }

        if (jobQueue == nullptr) {
            return HttpResponse(
                500,
                "Internal Server Error",
                "Job queue is not available"
            );
        }

        if (request.getBody().empty()) {
            return HttpResponse(
                400,
                "Bad Request",
                "Job command cannot be empty"
            );
        }

        int jobId = job_queue_submit(
            jobQueue,
            request.getBody().c_str()
        );

        if (jobId == 0) {
            return HttpResponse(
                500,
                "Internal Server Error",
                "Failed to submit job"
            );
        }

        return HttpResponse(
            201,
            "Created",
            "Job submitted: " + std::to_string(jobId)
        );
    }

    return HttpResponse(
        404,
        "Not Found",
        "Not Found"
    );
}
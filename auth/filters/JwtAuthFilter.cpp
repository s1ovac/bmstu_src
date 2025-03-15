#include "JwtAuthFilter.h"
#include <drogon/drogon.h>
#include "utils/JWT.h"

JwtAuthFilter::JwtAuthFilter()
{
    // Check if keys are properly set in JwtUtils
    if (JwtUtils::getPublicKey().empty()) {
        LOG_ERROR << "JWT public key is not set. Please set it using JwtUtils::setPublicKey() before running the app.";
        throw std::runtime_error("JWT public key is not set");
    }

    if (JwtUtils::getPrivateKey().empty()) {
        LOG_WARN << "JWT private key is not set. This may be okay for services that only validate tokens.";
    }
}

void JwtAuthFilter::doFilter(const HttpRequestPtr &req, FilterCallback &&fcb, FilterChainCallback &&fccb)
{
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ")
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        fcb(resp);
        return;
    }

    std::string token = authHeader.substr(7);

    try
    {
        std::string userId;
        // Validate token and get userId using JwtUtils
        if (!JwtUtils::validateToken(token, userId))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Invalid token");
            fcb(resp);
            return;
        }

        // Save userId and roles in request attributes
        req->attributes()->insert("user_id", userId);

        fccb(); // Proceed to next filter or controller
    }
    catch (const std::exception &e)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Internal server error: " + std::string(e.what()));
        fcb(resp);
    }
}
#include "JwtAuthFilter.h"
#include <drogon/drogon.h>
#include "../pkg/jwt_utils.h"

std::string JwtAuthFilter::privateKey_;
std::string JwtAuthFilter::publicKey_;

JwtAuthFilter::JwtAuthFilter()
{
    if (publicKey_.empty() || privateKey_.empty())
    {
        LOG_ERROR << "JWT keys are not set. Please set them using JwtAuthFilter::setPublicKey() and JwtAuthFilter::setPrivateKey() before running the app.";
        throw std::runtime_error("JWT keys are not set");
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
        // Validate token and get userId
        if (!JwtUtils::validateToken(token, userId))
        {
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k401Unauthorized);
            resp->setBody("Invalid token");
            fcb(resp);
            return;
        }

        // Get roles from database
        auto db = DB::instance();
        std::vector<std::string> roles = db->getUserRoles(userId);

        // Save userId and roles in request attributes
        req->attributes()->insert("user_id", userId);
        req->attributes()->insert("roles", roles);

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
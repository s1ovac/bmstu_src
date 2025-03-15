#include "PermissionFilter.h"
#include <drogon/drogon.h>
#include <vector>
#include <string>

PermissionFilter::PermissionFilter()
{
    db_ = DB::instance();

    if (!db_) {
        LOG_ERROR << "Database instance is not initialized";
        throw std::runtime_error("Database instance is not initialized");
    }
}

void PermissionFilter::doFilter(const drogon::HttpRequestPtr &req, drogon::FilterCallback &&fcb, drogon::FilterChainCallback &&fccb) {
    auto roles = req->attributes()->get<std::vector<std::string>>("roles");
    if (roles.empty()) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        resp->setBody("Access denied: No roles found");
        fcb(resp);
        return;
    }

    // Проверяем наличие разрешения у ролей
    bool hasPermission = false;
    for (const auto& role : roles)
    {
        if (db_->roleHasPermission(role, requiredPermission_))
        {
            hasPermission = true;
            break;
        }
    }

    if (!hasPermission)
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        resp->setBody("Access denied: Permission " + requiredPermission_ + " required");
        fcb(resp);
    }
    else
    {
        fccb();
    }
}
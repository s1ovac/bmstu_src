#include "filters/PermissionFilter.h"
#include <drogon/drogon.h>
#include "pkg/permission_utils.h"

PermissionFilter::PermissionFilter()
{
    LOG_INFO << "Initializing PermissionFilter";
    permissionUtils_ = PermissionUtils::instance();

    if (!permissionUtils_) {
        throw std::runtime_error("PermissionUtils instance is not initialized");
    }
}

void PermissionFilter::doFilter(const HttpRequestPtr &req, FilterCallback &&fcb, FilterChainCallback &&fccb)
{
    // This filter assumes JwtAuthFilter has already validated the token
    // and set the user_id attribute

    try
    {
        // Get user_id from request attribute (set by JwtAuthFilter)
        std::string userId = req->attributes()->get<std::string>("user_id");
        std::string path = req->getPath();

        LOG_INFO << "Checking permissions for user " << userId << " on path " << path;

        // Check if this is an admin endpoint
        if (path.find("/api/v1/admin") == 0) {
            if (!checkAdminPermission(req, fcb)) {
                return;  // Permission check failed
            }
        }

        // Proceed to next filter or controller
        fccb();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Error in PermissionFilter: " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Internal server error");
        fcb(resp);
    }
}

bool PermissionFilter::checkAdminPermission(const HttpRequestPtr &req, FilterCallback &fcb)
{
    std::string userId = req->attributes()->get<std::string>("user_id");

    // Check if user has manage_files permission
    if (!permissionUtils_->hasPermission(userId, "manage_files")) {
        LOG_WARN << "User " << userId << " does not have required admin permissions";
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - Admin permission required");
        fcb(resp);
        return false;
    }

    // Store the permission check result in the request attributes
    req->attributes()->insert("has_admin_permission", true);
    LOG_INFO << "User " << userId << " has admin permissions, proceeding";
    return true;
}
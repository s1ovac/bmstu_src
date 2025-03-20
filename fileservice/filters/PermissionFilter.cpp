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
            // Check if user has manage_files permission
            if (!permissionUtils_->hasPermission(userId, "manage_files")) {
                LOG_WARN << "User " << userId << " does not have required admin permissions";
                auto resp = HttpResponse::newHttpResponse();
                resp->setStatusCode(k403Forbidden);
                resp->setBody("Forbidden - Admin permission required");
                fcb(resp);
                return;
            }

            // Store the permission check result in the request attributes
            req->attributes()->insert("has_admin_permission", true);
            LOG_INFO << "User " << userId << " has admin permissions, proceeding";
        }

            // Check if this is a file endpoint that requires read/write permissions
        else if (path.find("/api/v1/files") == 0 || path.find("/api/v1/folders") == 0) {
            // For file operations, we would check for different permissions
            // based on the HTTP method (GET, POST, PUT, DELETE)
            if (req->method() == drogon::Get) {
                // Read operations require read permission
                if (!permissionUtils_->hasPermission(userId, "read_files")) {
                    LOG_WARN << "User " << userId << " does not have required read_files permission";
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k403Forbidden);
                    resp->setBody("Forbidden - Read permission required");
                    fcb(resp);
                    return;
                }
            }
            else if (req->method() == drogon::Post ||
                     req->method() == drogon::Put ||
                     req->method() == drogon::Delete) {
                // Write operations require write permission
                if (!permissionUtils_->hasPermission(userId, "write_files")) {
                    LOG_WARN << "User " << userId << " does not have required write_files permission";
                    auto resp = HttpResponse::newHttpResponse();
                    resp->setStatusCode(k403Forbidden);
                    resp->setBody("Forbidden - Write permission required");
                    fcb(resp);
                    return;
                }
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
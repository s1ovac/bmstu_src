#pragma once

#include <drogon/HttpController.h>
#include "services/AccessControlService.h"

using namespace drogon;

class PermissionController : public drogon::HttpController<PermissionController>
{
public:
    METHOD_LIST_BEGIN
        // Get permissions for a specific user
        ADD_METHOD_TO(PermissionController::getUserPermissions, "/api/v1/users/{user_id}/permissions", Get, "JwtAuthFilter");

        // Get permissions for the current authenticated user
        ADD_METHOD_TO(PermissionController::getCurrentUserPermissions, "/api/v1/permissions", Get, "JwtAuthFilter");
    METHOD_LIST_END

    PermissionController();

    void getUserPermissions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string user_id);
    void getCurrentUserPermissions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

private:
    std::shared_ptr<AccessControlService> accessControlService_;
};
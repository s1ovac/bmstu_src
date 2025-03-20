#include "PermissionController.h"
#include <json/json.h>

PermissionController::PermissionController()
{
    accessControlService_ = AccessControlService::instance();
}

void PermissionController::getUserPermissions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string user_id)
{
    // Extract requester_user_id from the token
    std::string requester_user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'getUserPermissions' request by user_id: " << requester_user_id
             << " for target user_id: " << user_id;

    // Only allow if current user is requesting their own permissions or has manage_roles permission
    if (requester_user_id != user_id && !accessControlService_->hasPermission(requester_user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - You can only access your own permissions or need manage_roles permission");
        callback(resp);
        return;
    }

    // Get the user's permissions
    auto permissions = accessControlService_->getUserPermissions(user_id);

    // Create JSON response
    Json::Value respData;
    Json::Value permissionsArray(Json::arrayValue);

    for (const auto& permission : permissions)
    {
        permissionsArray.append(permission);
    }

    respData["user_id"] = user_id;
    respData["permissions"] = permissionsArray;

    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void PermissionController::getCurrentUserPermissions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Extract user_id from the token
    std::string user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'getCurrentUserPermissions' request for user_id: " << user_id;

    // Get the user's permissions
    auto permissions = accessControlService_->getUserPermissions(user_id);

    // Create JSON response
    Json::Value respData;
    Json::Value permissionsArray(Json::arrayValue);

    for (const auto& permission : permissions)
    {
        permissionsArray.append(permission);
    }

    respData["user_id"] = user_id;
    respData["permissions"] = permissionsArray;

    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}
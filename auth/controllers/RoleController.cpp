#include "RoleController.h"
#include <json/json.h>
#include "../utils/JWT.h"

RoleController::RoleController()
{
    roleService_ = RoleService::instance();
    accessControlService_ = AccessControlService::instance();
}

void RoleController::createRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Extract user_id from the token
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        callback(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    std::string user_id;
    if (!JwtUtils::validateToken(token, user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    // Check if user has permission to manage roles
    if (!accessControlService_->hasPermission(user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !(*json)["role_name"].isString())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'role_name' is required");
        callback(resp);
        return;
    }

    std::string role_name = (*json)["role_name"].asString();
    std::string description = (*json).get("description", "").asString();

    if (!roleService_->createRole(role_name, description))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Failed to create role");
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "Role created successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void RoleController::deleteRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int role_id)
{
    // Extract user_id from the token
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        callback(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    std::string user_id;
    if (!JwtUtils::validateToken(token, user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_->hasPermission(user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    if (!roleService_->deleteRole(role_id))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Role not found or failed to delete");
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "Role deleted successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void RoleController::assignPermissionsToRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int role_id)
{
    // Extract user_id from the token
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        callback(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    std::string user_id;
    if (!JwtUtils::validateToken(token, user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_->hasPermission(user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !(*json)["permission_ids"].isArray())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'permission_ids' is required and should be an array");
        callback(resp);
        return;
    }

    std::vector<int> permission_ids;
    for (const auto &id : (*json)["permission_ids"])
    {
        if (id.isInt())
        {
            permission_ids.push_back(id.asInt());
        }
    }

    if (!roleService_->assignPermissionsToRole(role_id, permission_ids))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Failed to assign permissions to role");
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "Permissions assigned to role successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void RoleController::assignRoleToUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id)
{
    // Extract requester_user_id from the token
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        callback(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_->hasPermission(requester_user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !(*json)["role_ids"].isArray())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'role_ids' is required and should be an array");
        callback(resp);
        return;
    }

    std::vector<int> role_ids;
    for (const auto &id : (*json)["role_ids"])
    {
        if (id.isInt())
        {
            role_ids.push_back(id.asInt());
        }
    }

    if (!roleService_->assignRolesToUser(user_id, role_ids))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Failed to assign roles to user");
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "Roles assigned to user successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void RoleController::getUserRoles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id)
{
    // Extract requester_user_id from the token
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        callback(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    // Only allow if current user is requesting their own roles or has manage_roles permission
    if (requester_user_id != std::to_string(user_id) && !accessControlService_->hasPermission(requester_user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto roles = roleService_->getUserRoles(user_id);

    Json::Value respData;
    Json::Value rolesJson(Json::arrayValue);
    for (const auto &role : roles)
    {
        Json::Value roleJson;
        roleJson["role_id"] = role.first;
        roleJson["role_name"] = role.second;
        rolesJson.append(roleJson);
    }
    respData["roles"] = rolesJson;

    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void RoleController::getAllRoles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Extract user_id from the token
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Unauthorized");
        callback(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    std::string user_id;
    if (!JwtUtils::validateToken(token, user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    // Check if user has permission to view all roles
    if (!accessControlService_->hasPermission(user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto allRoles = roleService_->getAllRoles();

    // Create JSON array
    Json::Value arr(Json::arrayValue);
    for (auto &rolePair : allRoles)
    {
        Json::Value roleJson;
        roleJson["role_id"] = rolePair.first;
        roleJson["role_name"] = rolePair.second;
        arr.append(roleJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(arr);
    callback(resp);
}
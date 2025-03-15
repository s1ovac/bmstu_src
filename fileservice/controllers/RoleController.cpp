#include "RoleController.h"
#include "../services/RoleService.h"
#include "../services/AccessControlService.h"
#include <json/json.h>

void RoleController::createRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");

    auto accessControlService = AccessControlService::instance();
    if (!accessControlService->hasPermission(user_id, "manage_roles"))
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

    auto roleService = RoleService::instance();
    if (!roleService->createRole(role_name, description))
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
    std::string user_id = req->attributes()->get<std::string>("user_id");

    auto accessControlService = AccessControlService::instance();
    if (!accessControlService->hasPermission(user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto roleService = RoleService::instance();
    if (!roleService->deleteRole(role_id))
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
    std::string user_id = req->attributes()->get<std::string>("user_id");

    auto accessControlService = AccessControlService::instance();
    if (!accessControlService->hasPermission(user_id, "manage_roles"))
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

    auto roleService = RoleService::instance();
    if (!roleService->assignPermissionsToRole(role_id, permission_ids))
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
    std::string requester_user_id = req->attributes()->get<std::string>("user_id");

    auto accessControlService = AccessControlService::instance();
    if (!accessControlService->hasPermission(requester_user_id, "manage_roles"))
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

    auto roleService = RoleService::instance();
    if (!roleService->assignRolesToUser(user_id, role_ids))
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
    std::string requester_user_id = req->attributes()->get<std::string>("user_id");

    auto accessControlService = AccessControlService::instance();
    if (requester_user_id != std::to_string(user_id) && !accessControlService->hasPermission(requester_user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    auto roleService = RoleService::instance();
    auto roles = roleService->getUserRoles(user_id);

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
    std::string user_id = req->attributes()->get<std::string>("user_id");

    // Проверим, есть ли право "manage_roles" —
    // или же, если хотите, чтобы любой авторизованный мог видеть список ролей, можно не проверять:
    auto accessControlService = AccessControlService::instance();
    if (!accessControlService->hasPermission(user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    // Получаем все роли
    auto roleService = RoleService::instance();
    auto allRoles = roleService->getAllRoles();

    // Формируем JSON-массив
    Json::Value arr(Json::arrayValue);
    for (auto &rolePair : allRoles)
    {
        // rolePair = (role_id, role_name)
        Json::Value roleJson;
        roleJson["role_id"]   = rolePair.first;
        roleJson["role_name"] = rolePair.second;
        arr.append(roleJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(arr);
    callback(resp);
}
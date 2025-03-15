#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class RoleController : public drogon::HttpController<RoleController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(RoleController::createRole, "/api/v1/role", Post, "JwtAuthFilter");
        ADD_METHOD_TO(RoleController::deleteRole, "/api/v1/roles/{role_id}", Delete, "JwtAuthFilter");
        ADD_METHOD_TO(RoleController::assignPermissionsToRole, "/api/v1/roles/{role_id}/permissions", Post, "JwtAuthFilter");
        ADD_METHOD_TO(RoleController::assignRoleToUser, "/api/v1/users/{user_id}/roles", Post, "JwtAuthFilter");
        ADD_METHOD_TO(RoleController::getUserRoles, "/api/v1/users/{user_id}/roles", Get, "JwtAuthFilter");
        ADD_METHOD_TO(RoleController::getAllRoles, "/api/v1/roles", Get, "JwtAuthFilter");
    METHOD_LIST_END

    void createRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int role_id);
    void assignPermissionsToRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int role_id);
    void assignRoleToUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void getUserRoles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void getAllRoles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

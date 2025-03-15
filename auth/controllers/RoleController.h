#pragma once

#include <drogon/HttpController.h>
#include "../services/RoleService.h"
#include "../services/AccessControlService.h"

using namespace drogon;

class RoleController
{
public:
    explicit RoleController(RoleService& roleService, AccessControlService& accessControlService)
            : roleService_(roleService), accessControlService_(accessControlService) {}

    void createRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int role_id);
    void assignPermissionsToRole(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int role_id);
    void assignRoleToUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void getUserRoles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void getAllRoles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    static void registerRoutes(drogon::HttpAppFramework &app, const std::shared_ptr<RoleController>& controller)
    {
        app.registerHandler("/api/v1/role",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                            {
                                controller->createRole(req, std::move(callback));
                            },
                            {drogon::Post});

        app.registerHandler("/api/v1/roles/{1}",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int role_id)
                            {
                                controller->deleteRole(req, std::move(callback), role_id);
                            },
                            {drogon::Delete});

        app.registerHandler("/api/v1/roles/{1}/permissions",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int role_id)
                            {
                                controller->assignPermissionsToRole(req, std::move(callback), role_id);
                            },
                            {drogon::Post});

        app.registerHandler("/api/v1/users/{1}/roles",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int user_id)
                            {
                                controller->assignRoleToUser(req, std::move(callback), user_id);
                            },
                            {drogon::Post});

        app.registerHandler("/api/v1/users/{1}/roles",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int user_id)
                            {
                                controller->getUserRoles(req, std::move(callback), user_id);
                            },
                            {drogon::Get});

        app.registerHandler("/api/v1/roles",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                            {
                                controller->getAllRoles(req, std::move(callback));
                            },
                            {drogon::Get});
    }

private:
    RoleService& roleService_;
    AccessControlService& accessControlService_;
};
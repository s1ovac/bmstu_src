#pragma once

#include <drogon/HttpController.h>
#include "../repository/UsersRepo.h"
#include "../services/AccessControlService.h"

using namespace drogon;

class AdminController
{
public:
    explicit AdminController(DB& db, AccessControlService& accessControlService)
            : db_(db), accessControlService_(accessControlService) {}

    // Returns all users with their roles
    void getAllUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    static void registerRoutes(drogon::HttpAppFramework &app, const std::shared_ptr<AdminController>& controller)
    {
        app.registerHandler("/api/v1/admin/users",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                            {
                                controller->getAllUsers(req, std::move(callback));
                            },
                            {drogon::Get});
    }

private:
    DB& db_;
    AccessControlService& accessControlService_;
};
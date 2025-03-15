#pragma once

#include <drogon/HttpController.h>
#include "../services/GroupService.h"
#include "../services/AccessControlService.h"

using namespace drogon;

class GroupController
{
public:
    explicit GroupController(GroupService& groupService, AccessControlService& accessControlService)
            : groupService_(groupService), accessControlService_(accessControlService) {}

    void createGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id);
    void renameGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id);
    void addUserToGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id, int user_id);
    void removeUserFromGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id, int user_id);
    void getUserGroups(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void getAllGroups(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    static void registerRoutes(drogon::HttpAppFramework &app, const std::shared_ptr<GroupController>& controller)
    {
        app.registerHandler("/api/v1/groups",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                            {
                                controller->getAllGroups(req, std::move(callback));
                            },
                            {drogon::Get});

        app.registerHandler("/api/v1/groups",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                            {
                                controller->createGroup(req, std::move(callback));
                            },
                            {drogon::Post});

        app.registerHandler("/api/v1/groups/{1}",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int group_id)
                            {
                                controller->deleteGroup(req, std::move(callback), group_id);
                            },
                            {drogon::Delete});

        app.registerHandler("/api/v1/groups/{1}",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int group_id)
                            {
                                controller->renameGroup(req, std::move(callback), group_id);
                            },
                            {drogon::Put});

        app.registerHandler("/api/v1/groups/{1}/add/{2}",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int group_id, int user_id)
                            {
                                controller->addUserToGroup(req, std::move(callback), group_id, user_id);
                            },
                            {drogon::Post});

        app.registerHandler("/api/v1/groups/{1}/remove/{2}",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int group_id, int user_id)
                            {
                                controller->removeUserFromGroup(req, std::move(callback), group_id, user_id);
                            },
                            {drogon::Delete});

        app.registerHandler("/api/v1/users/{1}/groups",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                         int user_id)
                            {
                                controller->getUserGroups(req, std::move(callback), user_id);
                            },
                            {drogon::Get});
    }

private:
    GroupService& groupService_;
    AccessControlService& accessControlService_;
};
#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class GroupController : public drogon::HttpController<GroupController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(GroupController::getAllGroups,     "/api/v1/groups",             Get,   "JwtAuthFilter");
        ADD_METHOD_TO(GroupController::createGroup,      "/api/v1/groups",             Post,   "JwtAuthFilter");
        ADD_METHOD_TO(GroupController::deleteGroup,      "/api/v1/groups/{group_id}",  Delete, "JwtAuthFilter");
        ADD_METHOD_TO(GroupController::renameGroup,      "/api/v1/groups/{group_id}",  Put,    "JwtAuthFilter");
        ADD_METHOD_TO(GroupController::addUserToGroup,   "/api/v1/groups/{group_id}/add/{user_id}", Post, "JwtAuthFilter");
        ADD_METHOD_TO(GroupController::removeUserFromGroup, "/api/v1/groups/{group_id}/remove/{user_id}", Delete, "JwtAuthFilter");
        ADD_METHOD_TO(GroupController::getUserGroups,    "/api/v1/users/{user_id}/groups", Get,  "JwtAuthFilter");
    METHOD_LIST_END

    // Обработчики
    void createGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id);
    void renameGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id);
    void addUserToGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id, int user_id);
    void removeUserFromGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id, int user_id);
    void getUserGroups(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id);
    void getAllGroups(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

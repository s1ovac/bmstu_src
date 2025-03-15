#include "GroupController.h"
#include <json/json.h>
#include "../utils/JWT.h"

void GroupController::createGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    // Check permissions
    if (!accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - manage_groups permission required");
        callback(resp);
        return;
    }

    // Read JSON data
    auto json = req->getJsonObject();
    if (!json || !(*json)["group_name"].isString())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'group_name' is required");
        callback(resp);
        return;
    }

    std::string group_name = (*json)["group_name"].asString();

    // Call service
    if (!groupService_.createGroup(group_name))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Failed to create group");
        callback(resp);
        return;
    }

    Json::Value respJson;
    respJson["message"] = "Group created successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void GroupController::deleteGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - manage_groups permission required");
        callback(resp);
        return;
    }

    if (!groupService_.deleteGroup(group_id))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Group not found or failed to delete");
        callback(resp);
        return;
    }

    Json::Value respJson;
    respJson["message"] = "Group deleted successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void GroupController::renameGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - manage_groups permission required");
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !(*json)["new_name"].isString())
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'new_name' is required");
        callback(resp);
        return;
    }

    std::string new_name = (*json)["new_name"].asString();

    if (!groupService_.renameGroup(group_id, new_name))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Group not found or failed to rename");
        callback(resp);
        return;
    }

    Json::Value respJson;
    respJson["message"] = "Group renamed successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void GroupController::addUserToGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id, int user_id)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - manage_groups permission required");
        callback(resp);
        return;
    }

    if (!groupService_.addUserToGroup(user_id, group_id))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Failed to add user to group");
        callback(resp);
        return;
    }

    Json::Value respJson;
    respJson["message"] = "User added to group successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void GroupController::removeUserFromGroup(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int group_id, int user_id)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    if (!accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - manage_groups permission required");
        callback(resp);
        return;
    }

    if (!groupService_.removeUserFromGroup(user_id, group_id))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Failed to remove user from group");
        callback(resp);
        return;
    }

    Json::Value respJson;
    respJson["message"] = "User removed from group successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void GroupController::getUserGroups(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int user_id)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    // User can only view their own groups or needs manage_groups permission
    if (requester_user_id != std::to_string(user_id) &&
        !accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - you can see only your own groups or need manage_groups permission");
        callback(resp);
        return;
    }

    auto groups = groupService_.getUserGroups(user_id);

    Json::Value respJson;
    Json::Value groupsArr(Json::arrayValue);

    for (auto &grp : groups)
    {
        Json::Value g;
        g["group_id"] = grp.first;
        g["group_name"] = grp.second;
        groupsArr.append(g);
    }
    respJson["groups"] = groupsArr;

    auto resp = HttpResponse::newHttpJsonResponse(respJson);
    callback(resp);
}

void GroupController::getAllGroups(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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
    std::string requester_user_id;
    if (!JwtUtils::validateToken(token, requester_user_id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k401Unauthorized);
        resp->setBody("Invalid token");
        callback(resp);
        return;
    }

    // Check manage_groups permission
    if (!accessControlService_.hasPermission(requester_user_id, "manage_groups"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - manage_groups permission required");
        callback(resp);
        return;
    }

    auto groups = groupService_.getAllGroups();

    // Create JSON response
    Json::Value arr(Json::arrayValue);
    for (auto &g : groups)
    {
        Json::Value obj;
        obj["group_id"] = g.first;
        obj["group_name"] = g.second;
        arr.append(obj);
    }

    auto resp = HttpResponse::newHttpJsonResponse(arr);
    callback(resp);
}
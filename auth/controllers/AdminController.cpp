#include "AdminController.h"
#include <json/json.h>
#include "../utils/JWT.h"

void AdminController::getAllUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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

    // Check if requester has manage_roles permission
    if (!accessControlService_.hasPermission(requester_user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    // Get all users with their roles from database
    auto userList = db_.getAllUsersWithRoles();

    // Create JSON array response
    Json::Value arr(Json::arrayValue);
    for (auto &userTuple : userList)
    {
        int userId = std::get<0>(userTuple);
        std::string email = std::get<1>(userTuple);
        auto rolesVector = std::get<2>(userTuple);

        Json::Value userJson;
        userJson["user_id"] = userId;
        userJson["email"] = email;

        Json::Value rolesArr(Json::arrayValue);
        for (auto &r : rolesVector)
        {
            rolesArr.append(r);
        }
        userJson["roles"] = rolesArr;

        arr.append(userJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(arr);
    callback(resp);
}
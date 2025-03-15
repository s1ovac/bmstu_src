#include "AdminController.h"
#include <json/json.h>

AdminController::AdminController()
{
    db_ = DB::instance();
    accessControlService_ = AccessControlService::instance();
}

void AdminController::getAllUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string requester_user_id = req->attributes()->get<std::string>("user_id");

    // Check if requester has manage_roles permission
    if (!accessControlService_->hasPermission(requester_user_id, "manage_roles"))
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden");
        callback(resp);
        return;
    }

    // Get all users with their roles from database
    auto userList = db_->getAllUsersWithRoles();

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
#include "AdminController.h"
#include "../services/AccessControlService.h"
#include <json/json.h>

void AdminController::getAllUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
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

    // Получаем список пользователей с ролями из БД
    auto db = DB::instance();
    auto userList = db->getAllUsersWithRoles();

    // Формируем JSON-массив
    Json::Value arr(Json::arrayValue);
    for (auto &userTuple : userList)
    {
        // userTuple = (user_id, email, vector<string> roles)
        int userId          = std::get<0>(userTuple);
        std::string email   = std::get<1>(userTuple);
        auto rolesVector    = std::get<2>(userTuple);

        Json::Value userJson;
        userJson["user_id"]    = userId;
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

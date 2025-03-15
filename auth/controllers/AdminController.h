#pragma once

#include <drogon/HttpController.h>
#include "repository/DB.h"
#include "services/AccessControlService.h"

using namespace drogon;

class AdminController : public drogon::HttpController<AdminController>
{
public:
    METHOD_LIST_BEGIN
        // GET /api/v1/admin/users
        ADD_METHOD_TO(AdminController::getAllUsers, "/api/v1/admin/users", Get, "JwtAuthFilter");
    METHOD_LIST_END

    AdminController();

    // Возвращает всех пользователей с их ролями
    void getAllUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

private:
    std::shared_ptr<DB> db_;
    std::shared_ptr<AccessControlService> accessControlService_;
};
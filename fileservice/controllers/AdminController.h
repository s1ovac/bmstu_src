#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "services/AdminService.h"

using namespace drogon;

class AdminController : public drogon::HttpController<AdminController>
{
public:
    METHOD_LIST_BEGIN
        // Get all files in the system grouped by user
        ADD_METHOD_TO(AdminController::getAllFiles, "/api/v1/admin/files", Get, "JwtAuthFilter", "PermissionFilter");

        // Get all folders in the system grouped by user
        ADD_METHOD_TO(AdminController::getAllFolders, "/api/v1/admin/folders", Get, "JwtAuthFilter", "PermissionFilter");

        // Get all files and folders for a specific user
        ADD_METHOD_TO(AdminController::getUserContent, "/api/v1/admin/users/{user_id}/content", Get, "JwtAuthFilter", "PermissionFilter");

        // Get system statistics (file counts, storage usage, etc.)
        ADD_METHOD_TO(AdminController::getSystemStats, "/api/v1/admin/stats", Get, "JwtAuthFilter", "PermissionFilter");
    METHOD_LIST_END

    AdminController();

    void getAllFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getAllFolders(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getUserContent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string user_id);
    void getSystemStats(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

private:
    std::shared_ptr<AdminService> adminService_;
};
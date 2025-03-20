#include "AdminController.h"
#include <drogon/drogon.h>

AdminController::AdminController() {
    LOG_INFO << "Initializing AdminController";
    adminService_ = AdminService::instance();
}

void AdminController::getAllFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Extract user_id from the token (set by JwtAuthFilter)
    std::string admin_user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'getAllFiles' admin request by user_id: " << admin_user_id;

    // Check if the user has the admin permission (should already be handled by PermissionFilter)
    if (!req->attributes()->get<bool>("has_admin_permission")) {
        LOG_WARN << "Unauthorized access attempt to admin endpoint by user_id: " << admin_user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - Admin permission required");
        callback(resp);
        return;
    }

    // Get all files from all users
    auto filesData = adminService_->getAllFiles();

    // Create JSON response
    Json::Value response;
    response["total_users"] = static_cast<int>(filesData.size());
    response["users"] = Json::arrayValue;

    int totalFiles = 0;

    for (const auto& userData : filesData) {
        Json::Value userJson;
        userJson["user_id"] = userData.user_id;
        userJson["email"] = userData.email;
        userJson["files_count"] = static_cast<int>(userData.files.size());

        totalFiles += userData.files.size();

        Json::Value filesArray = Json::arrayValue;
        for (const auto& file : userData.files) {
            Json::Value fileJson;
            fileJson["file_id"] = file.file_id;
            fileJson["file_name"] = file.file_name;
            fileJson["file_size"] = file.file_size;
            fileJson["folder_id"] = file.folder_id;
            fileJson["created_at"] = file.created_at;
            filesArray.append(fileJson);
        }

        userJson["files"] = filesArray;
        response["users"].append(userJson);
    }

    response["total_files"] = totalFiles;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void AdminController::getAllFolders(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Extract user_id from the token (set by JwtAuthFilter)
    std::string admin_user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'getAllFolders' admin request by user_id: " << admin_user_id;

    // Check if the user has the admin permission (should already be handled by PermissionFilter)
    if (!req->attributes()->get<bool>("has_admin_permission")) {
        LOG_WARN << "Unauthorized access attempt to admin endpoint by user_id: " << admin_user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - Admin permission required");
        callback(resp);
        return;
    }

    // Get all folders from all users
    auto foldersData = adminService_->getAllFolders();

    // Create JSON response
    Json::Value response;
    response["total_users"] = static_cast<int>(foldersData.size());
    response["users"] = Json::arrayValue;

    int totalFolders = 0;

    for (const auto& userData : foldersData) {
        Json::Value userJson;
        userJson["user_id"] = userData.user_id;
        userJson["email"] = userData.email;
        userJson["folders_count"] = static_cast<int>(userData.folders.size());

        totalFolders += userData.folders.size();

        Json::Value foldersArray = Json::arrayValue;
        for (const auto& folder : userData.folders) {
            Json::Value folderJson;
            folderJson["folder_id"] = folder.folder_id;
            folderJson["folder_name"] = folder.folder_name;
            folderJson["parent_folder_id"] = folder.parent_folder_id;
            folderJson["created_at"] = folder.created_at;
            foldersArray.append(folderJson);
        }

        userJson["folders"] = foldersArray;
        response["users"].append(userJson);
    }

    response["total_folders"] = totalFolders;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void AdminController::getUserContent(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string user_id)
{
    // Extract admin user_id from the token (set by JwtAuthFilter)
    std::string admin_user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'getUserContent' admin request by user_id: " << admin_user_id
             << " for target user_id: " << user_id;

    // Check if the user has the admin permission (should already be handled by PermissionFilter)
    if (!req->attributes()->get<bool>("has_admin_permission")) {
        LOG_WARN << "Unauthorized access attempt to admin endpoint by user_id: " << admin_user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - Admin permission required");
        callback(resp);
        return;
    }

    // Get content for specified user
    auto userContent = adminService_->getUserContent(user_id);

    if (!userContent.has_value()) {
        LOG_WARN << "Target user not found or has no content. User ID: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("User not found or has no content");
        callback(resp);
        return;
    }

    // Create JSON response
    Json::Value response;
    const auto& content = userContent.value();

    response["user_id"] = content.user_id;
    response["email"] = content.email;
    response["files_count"] = static_cast<int>(content.files.size());
    response["folders_count"] = static_cast<int>(content.folders.size());

    // Calculate total storage used
    long long totalStorageBytes = 0;
    for (const auto& file : content.files) {
        totalStorageBytes += file.file_size;
    }
    response["total_storage_bytes"] = Json::Int64(totalStorageBytes);

    // Format total storage in human-readable format
    std::string readableStorage;
    if (totalStorageBytes < 1024) {
        readableStorage = std::to_string(totalStorageBytes) + " B";
    } else if (totalStorageBytes < 1024 * 1024) {
        readableStorage = std::to_string(totalStorageBytes / 1024) + " KB";
    } else if (totalStorageBytes < 1024 * 1024 * 1024) {
        readableStorage = std::to_string(totalStorageBytes / (1024 * 1024)) + " MB";
    } else {
        readableStorage = std::to_string(totalStorageBytes / (1024 * 1024 * 1024)) + " GB";
    }
    response["total_storage_readable"] = readableStorage;

    // Add files
    Json::Value filesArray = Json::arrayValue;
    for (const auto& file : content.files) {
        Json::Value fileJson;
        fileJson["file_id"] = file.file_id;
        fileJson["file_name"] = file.file_name;
        fileJson["file_size"] = file.file_size;
        fileJson["folder_id"] = file.folder_id;
        fileJson["created_at"] = file.created_at;
        filesArray.append(fileJson);
    }
    response["files"] = filesArray;

    // Add folders
    Json::Value foldersArray = Json::arrayValue;
    for (const auto& folder : content.folders) {
        Json::Value folderJson;
        folderJson["folder_id"] = folder.folder_id;
        folderJson["folder_name"] = folder.folder_name;
        folderJson["parent_folder_id"] = folder.parent_folder_id;
        folderJson["created_at"] = folder.created_at;
        foldersArray.append(folderJson);
    }
    response["folders"] = foldersArray;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}

void AdminController::getSystemStats(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    // Extract user_id from the token (set by JwtAuthFilter)
    std::string admin_user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'getSystemStats' admin request by user_id: " << admin_user_id;

    // Check if the user has the admin permission (should already be handled by PermissionFilter)
    if (!req->attributes()->get<bool>("has_admin_permission")) {
        LOG_WARN << "Unauthorized access attempt to admin endpoint by user_id: " << admin_user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k403Forbidden);
        resp->setBody("Forbidden - Admin permission required");
        callback(resp);
        return;
    }

    // Get system statistics
    auto stats = adminService_->getSystemStats();

    // Create JSON response
    Json::Value response;
    response["total_users"] = stats.total_users;
    response["total_files"] = stats.total_files;
    response["total_folders"] = stats.total_folders;
    response["total_storage_bytes"] = Json::Int64(stats.total_storage_bytes);

    // Format total storage in human-readable format
    std::string readableStorage;
    if (stats.total_storage_bytes < 1024) {
        readableStorage = std::to_string(stats.total_storage_bytes) + " B";
    } else if (stats.total_storage_bytes < 1024 * 1024) {
        readableStorage = std::to_string(stats.total_storage_bytes / 1024) + " KB";
    } else if (stats.total_storage_bytes < 1024 * 1024 * 1024) {
        readableStorage = std::to_string(stats.total_storage_bytes / (1024 * 1024)) + " MB";
    } else {
        readableStorage = std::to_string(stats.total_storage_bytes / (1024 * 1024 * 1024)) + " GB";
    }
    response["total_storage_readable"] = readableStorage;

    // Add file type distribution
    Json::Value fileTypesArray = Json::arrayValue;
    for (const auto& type : stats.files_by_type) {
        Json::Value typeJson;
        typeJson["type"] = type.first;
        typeJson["count"] = type.second;
        fileTypesArray.append(typeJson);
    }
    response["files_by_type"] = fileTypesArray;

    // Add top users by storage
    Json::Value topUsersArray = Json::arrayValue;
    for (const auto& user : stats.top_users_by_storage) {
        Json::Value userJson;
        userJson["user_id"] = user.first;
        userJson["storage_bytes"] = user.second;
        topUsersArray.append(userJson);
    }
    response["top_users_by_storage"] = topUsersArray;

    auto resp = HttpResponse::newHttpJsonResponse(response);
    callback(resp);
}
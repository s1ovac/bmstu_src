#include "FavoritesController.h"
#include <drogon/drogon.h>

FavoritesController::FavoritesController() {
    LOG_INFO << "Initializing FavoritesController";
    fileService_ = FileService::instance();
}

void FavoritesController::toggleFileFavorite(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();

    if (!json || !(*json)["file_id"].isInt() || !(*json)["is_favorite"].isBool())
    {
        LOG_ERROR << "Invalid JSON in toggleFileFavorite request for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'file_id' and 'is_favorite' are required");
        callback(resp);
        return;
    }

    int file_id = (*json)["file_id"].asInt();
    bool is_favorite = (*json)["is_favorite"].asBool();

    LOG_INFO << "Processing 'toggleFileFavorite' request for user_id: " << user_id
             << ", file_id: " << file_id << ", is_favorite: " << is_favorite;

    std::string errorMsg;
    if (!fileService_->toggleFileFavorite(user_id, file_id, is_favorite, errorMsg))
    {
        LOG_ERROR << "Failed to toggle file favorite for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();

        if (errorMsg.find("access denied") != std::string::npos) {
            resp->setStatusCode(k403Forbidden);
        } else {
            resp->setStatusCode(k500InternalServerError);
        }

        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = is_favorite ? "File added to favorites" : "File removed from favorites";
    respData["is_favorite"] = is_favorite;
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void FavoritesController::toggleFolderFavorite(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();

    if (!json || !(*json)["folder_id"].isInt() || !(*json)["is_favorite"].isBool())
    {
        LOG_ERROR << "Invalid JSON in toggleFolderFavorite request for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'folder_id' and 'is_favorite' are required");
        callback(resp);
        return;
    }

    int folder_id = (*json)["folder_id"].asInt();
    bool is_favorite = (*json)["is_favorite"].asBool();

    LOG_INFO << "Processing 'toggleFolderFavorite' request for user_id: " << user_id
             << ", folder_id: " << folder_id << ", is_favorite: " << is_favorite;

    std::string errorMsg;
    if (!fileService_->toggleFolderFavorite(user_id, folder_id, is_favorite, errorMsg))
    {
        LOG_ERROR << "Failed to toggle folder favorite for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();

        if (errorMsg.find("access denied") != std::string::npos) {
            resp->setStatusCode(k403Forbidden);
        } else {
            resp->setStatusCode(k500InternalServerError);
        }

        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = is_favorite ? "Folder added to favorites" : "Folder removed from favorites";
    respData["is_favorite"] = is_favorite;
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void FavoritesController::getFavoriteFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    LOG_INFO << "Processing 'getFavoriteFiles' request for user_id: " << user_id;

    auto files = fileService_->getFavoriteFiles(user_id);

    Json::Value data;
    data["files"] = Json::arrayValue;

    for (const auto& file : files)
    {
        Json::Value fileJson;
        fileJson["file_id"] = file.file_id;
        fileJson["file_name"] = file.file_name;
        fileJson["file_size"] = file.file_size;
        fileJson["created_at"] = file.created_at;
        fileJson["file_type"] = file.file_type;
        fileJson["owner_id"] = file.owner_id;
        fileJson["owner_email"] = file.owner_email;
        fileJson["group_id"] = file.group_id;
        fileJson["group_name"] = file.group_name;
        fileJson["can_modify"] = (file.owner_id == std::stoi(user_id));
        fileJson["is_favorite"] = file.is_favorite;
        data["files"].append(fileJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(data);
    callback(resp);
}

void FavoritesController::getFavoriteFolders(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    LOG_INFO << "Processing 'getFavoriteFolders' request for user_id: " << user_id;

    auto folders = fileService_->getFavoriteFolders(user_id);

    Json::Value data;
    data["folders"] = Json::arrayValue;

    for (const auto& folder : folders)
    {
        Json::Value folderJson;
        folderJson["folder_id"] = folder.folder_id;
        folderJson["folder_name"] = folder.folder_name;
        folderJson["parent_folder_id"] = folder.parent_folder_id;
        folderJson["created_at"] = folder.created_at;
        folderJson["folder_type"] = folder.folder_type;
        folderJson["owner_id"] = folder.owner_id;
        folderJson["owner_email"] = folder.owner_email;
        folderJson["group_id"] = folder.group_id;
        folderJson["group_name"] = folder.group_name;
        folderJson["can_modify"] = (folder.owner_id == std::stoi(user_id));
        folderJson["is_favorite"] = folder.is_favorite;
        data["folders"].append(folderJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(data);
    callback(resp);
}

void FavoritesController::getAllFavorites(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    std::string user_id = req->attributes()->get<std::string>("user_id");
    LOG_INFO << "Processing 'getAllFavorites' request for user_id: " << user_id;

    auto files = fileService_->getFavoriteFiles(user_id);
    auto folders = fileService_->getFavoriteFolders(user_id);

    Json::Value data;
    data["files"] = Json::arrayValue;
    data["folders"] = Json::arrayValue;

    for (const auto &file: files) {
        Json::Value fileJson;
        fileJson["file_id"] = file.file_id;
        fileJson["file_name"] = file.file_name;
        fileJson["file_size"] = file.file_size;
        fileJson["created_at"] = file.created_at;
        fileJson["file_type"] = file.file_type;
        fileJson["owner_id"] = file.owner_id;
        fileJson["owner_email"] = file.owner_email;
        fileJson["group_id"] = file.group_id;
        fileJson["group_name"] = file.group_name;
        fileJson["can_modify"] = (file.owner_id == std::stoi(user_id));
        fileJson["is_favorite"] = file.is_favorite;
        data["files"].append(fileJson);
    }

    for (const auto &folder: folders) {
        Json::Value folderJson;
        folderJson["folder_id"] = folder.folder_id;
        folderJson["folder_name"] = folder.folder_name;
        folderJson["parent_folder_id"] = folder.parent_folder_id;
        folderJson["created_at"] = folder.created_at;
        folderJson["folder_type"] = folder.folder_type;
        folderJson["owner_id"] = folder.owner_id;
        folderJson["owner_email"] = folder.owner_email;
        folderJson["group_id"] = folder.group_id;
        folderJson["group_name"] = folder.group_name;
        folderJson["can_modify"] = (folder.owner_id == std::stoi(user_id));
        folderJson["is_favorite"] = folder.is_favorite;
        data["folders"].append(folderJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(data);
    callback(resp);
}
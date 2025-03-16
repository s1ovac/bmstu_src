#include "FileController.h"
#include <drogon/drogon.h>

FileController::FileController() {
    LOG_INFO << "Initializing FileController";
    fileService_ = FileService::instance();
}

void FileController::getFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    int folder_id = req->getOptionalParameter<int>("folder_id").value_or(0);

    LOG_INFO << "Processing 'getFiles' request for user_id: " << user_id << ", folder_id: " << folder_id;

    auto files = fileService_->getFiles(user_id, folder_id);
    if (files.empty()) {
        LOG_WARN << "No files found for user_id: " << user_id << " in folder_id: " << folder_id;
    }

    Json::Value data;
    data["files"] = Json::arrayValue;

    for (const auto& file : files)
    {
        int file_id;
        std::string file_name;
        int file_size;
        std::string created_at;
        std::tie(file_id, file_name, file_size, created_at) = file;

        Json::Value fileJson;
        fileJson["file_id"] = file_id;
        fileJson["file_name"] = file_name;
        fileJson["file_size"] = file_size;
        fileJson["created_at"] = created_at;
        data["files"].append(fileJson);
    }

    auto resp = HttpResponse::newHttpJsonResponse(data);
    callback(resp);
}

void FileController::uploadFile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    int folder_id = req->getOptionalParameter<int>("folder_id").value_or(0);

    LOG_INFO << "Processing 'uploadFile' request for user_id: " << user_id << ", folder_id: " << folder_id;

    std::string errorMsg;
    if (!fileService_->uploadFile(user_id, folder_id, req, errorMsg))
    {
        LOG_ERROR << "File upload failed for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();

        if (errorMsg.find("Invalid filename:") != std::string::npos) {
            resp->setStatusCode(k400BadRequest);
        } else {
            resp->setStatusCode(k500InternalServerError);
        }

        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "File uploaded successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void FileController::deleteFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    LOG_INFO << "Processing 'deleteFiles' request for user_id: " << user_id;

    auto json = req->getJsonObject();
    if (!json || !(*json)["file_ids"].isArray())
    {
        LOG_ERROR << "Invalid JSON in request for deleting files for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    std::vector<int> file_ids;
    for (const auto& id : (*json)["file_ids"])
    {
        file_ids.push_back(id.asInt());
    }

    std::string errorMsg;
    if (!fileService_->deleteFiles(user_id, file_ids, errorMsg))
    {
        LOG_ERROR << "Failed to delete files for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k204NoContent);
    callback(resp);
}

void FileController::moveFile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();

    if (!json || !(*json)["file_id"].isInt() || !(*json)["target_folder_id"].isInt())
    {
        LOG_ERROR << "Invalid JSON in request for moving file for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'file_id' and 'target_folder_id' are required");
        callback(resp);
        return;
    }

    int file_id = (*json)["file_id"].asInt();
    int target_folder_id = (*json)["target_folder_id"].asInt();

    LOG_INFO << "Processing 'moveFile' request for user_id: " << user_id
             << ", file_id: " << file_id
             << ", target_folder_id: " << target_folder_id;

    std::string errorMsg;
    if (!fileService_->moveFile(user_id, file_id, target_folder_id, errorMsg))
    {
        LOG_ERROR << "Failed to move file for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "File moved successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void FileController::moveFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();

    if (!json || !(*json)["file_ids"].isArray() || !(*json)["target_folder_id"].isInt())
    {
        LOG_ERROR << "Invalid JSON in request for batch moving files for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON: 'file_ids' array and 'target_folder_id' are required");
        callback(resp);
        return;
    }

    std::vector<int> file_ids;
    for (const auto& id : (*json)["file_ids"])
    {
        if (id.isInt())
        {
            file_ids.push_back(id.asInt());
        }
    }

    if (file_ids.empty())
    {
        LOG_ERROR << "Empty file_ids array in batch move request for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("No valid file IDs provided");
        callback(resp);
        return;
    }

    int target_folder_id = (*json)["target_folder_id"].asInt();

    LOG_INFO << "Processing 'moveFiles' batch request for user_id: " << user_id
             << ", file_count: " << file_ids.size()
             << ", target_folder_id: " << target_folder_id;

    std::string errorMsg;
    if (!fileService_->moveFiles(user_id, file_ids, target_folder_id, errorMsg))
    {
        LOG_ERROR << "Failed to move files for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "Files moved successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void FileController::downloadFile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    auto file_id_str = req->getParameter("file_id");
    LOG_INFO << "Processing 'downloadFile' request for user_id: " << user_id << " with file_id: " << file_id_str;

    if (file_id_str.empty())
    {
        LOG_ERROR << "file_id parameter is missing in download request for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("file_id parameter is required");
        callback(resp);
        return;
    }

    int file_id = std::stoi(file_id_str);
    auto filePathOpt = fileService_->getFilePath(user_id, file_id);
    if (!filePathOpt.has_value())
    {
        LOG_ERROR << "File not found for user_id: " << user_id << " with file_id: " << file_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("File not found");
        callback(resp);
        return;
    }

    auto resp = HttpResponse::newFileResponse(filePathOpt.value());
    resp->setStatusCode(k200OK);
    resp->addHeader("Content-Disposition", "attachment; filename=\"" + fs::path(filePathOpt.value()).filename().string() + "\"");
    callback(resp);
}

// Методы для работы с папками
void FileController::getFolders(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
    {
    try {
        std::string user_id = req->attributes()->get<std::string>("user_id");
        int parent_folder_id = req->getOptionalParameter<int>("parent_folder_id").value_or(0);

        LOG_INFO << "Processing 'getFolders' request for user_id: " << user_id << ", parent_folder_id: " << parent_folder_id;

        auto folders = fileService_->getFolders(user_id, parent_folder_id);

        Json::Value data;
        data["folders"] = Json::arrayValue;

        for (const auto& folder : folders)
        {
            int folder_id;
            std::string folder_name;
            int parent_id;
            std::string created_at;
            std::tie(folder_id, folder_name, parent_id, created_at) = folder;

            Json::Value folderJson;
            folderJson["folder_id"] = folder_id;
            folderJson["folder_name"] = folder_name;
            folderJson["parent_folder_id"] = parent_id;
            folderJson["created_at"] = created_at;
            data["folders"].append(folderJson);
        }

        auto resp = HttpResponse::newHttpJsonResponse(data);
        callback(resp);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Exception in 'getFolders': " << e.what();
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid parent_folder_id parameter");
        callback(resp);
    }
}


void FileController::createFolder(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();

    if (!json || !(*json)["folder_name"].isString())
    {
        LOG_ERROR << "Invalid JSON in request for creating folder for user_id: " << user_id;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("Invalid JSON");
        callback(resp);
        return;
    }

    std::string folder_name = (*json)["folder_name"].asString();
    int parent_folder_id = (*json).get("parent_folder_id", 0).asInt();

    LOG_INFO << "Processing 'createFolder' request for user_id: " << user_id << ", folder_name: " << folder_name << ", parent_folder_id: " << parent_folder_id;

    std::string errorMsg;
    if (!fileService_->createFolder(user_id, folder_name, parent_folder_id, errorMsg))
    {
        LOG_ERROR << "Failed to create folder for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();

        if (errorMsg.find("Invalid folder name:") != std::string::npos) {
            resp->setStatusCode(k400BadRequest);
        } else {
            resp->setStatusCode(k500InternalServerError);
        }

        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    Json::Value respData;
    respData["message"] = "Folder created successfully";
    auto resp = HttpResponse::newHttpJsonResponse(respData);
    callback(resp);
}

void FileController::deleteFolder(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int folder_id)
{
    std::string user_id = req->attributes()->get<std::string>("user_id");

    LOG_INFO << "Processing 'deleteFolder' request for user_id: " << user_id << ", folder_id: " << folder_id;

    std::string errorMsg;
    if (!fileService_->deleteFolder(user_id, folder_id, errorMsg))
    {
        LOG_ERROR << "Failed to delete folder for user_id: " << user_id << " with error: " << errorMsg;
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody(errorMsg);
        callback(resp);
        return;
    }

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k204NoContent);
    callback(resp);
}

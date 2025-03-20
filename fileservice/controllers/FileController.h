#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "services/FileService.h"

using namespace drogon;

class FileController : public drogon::HttpController<FileController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(FileController::getFiles, "/api/v1/files", Get, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::uploadFile, "/api/v1/files", Post, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::deleteFiles, "/api/v1/files", Delete, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::moveFiles, "/api/v1/files/move", Put, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::moveFile, "/api/v1/file/move", Put, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::downloadFile, "/api/v1/file", Get, "JwtAuthFilter");

        // Методы для работы с папками
        ADD_METHOD_TO(FileController::getFolders, "/api/v1/folders", Get, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::createFolder, "/api/v1/folders", Post, "JwtAuthFilter");
        ADD_METHOD_TO(FileController::deleteFolder, "/api/v1/folders/{folder_id}", Delete, "JwtAuthFilter");
    METHOD_LIST_END

    FileController();

    void getFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void uploadFile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void downloadFile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void moveFile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void moveFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);


    // Методы для работы с папками
    void getFolders(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void createFolder(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void deleteFolder(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int folder_id);

private:
    std::shared_ptr<FileService> fileService_;
};

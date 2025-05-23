#pragma once

#include <drogon/HttpController.h>
#include <memory>
#include "services/FileService.h"

using namespace drogon;

class FavoritesController : public drogon::HttpController<FavoritesController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(FavoritesController::toggleFileFavorite, "/api/v1/favorites/file", Put, "JwtAuthFilter");
        ADD_METHOD_TO(FavoritesController::toggleFolderFavorite, "/api/v1/favorites/folder", Put, "JwtAuthFilter");
        ADD_METHOD_TO(FavoritesController::getFavoriteFiles, "/api/v1/favorites/files", Get, "JwtAuthFilter");
        ADD_METHOD_TO(FavoritesController::getFavoriteFolders, "/api/v1/favorites/folders", Get, "JwtAuthFilter");
        ADD_METHOD_TO(FavoritesController::getAllFavorites, "/api/v1/favorites", Get, "JwtAuthFilter");
    METHOD_LIST_END

    FavoritesController();

    void toggleFileFavorite(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void toggleFolderFavorite(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getFavoriteFiles(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getFavoriteFolders(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getAllFavorites(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

private:
    std::shared_ptr<FileService> fileService_;
};
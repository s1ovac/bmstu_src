#pragma once

#include <drogon/HttpRequest.h>
#include <filesystem>
#include <string>
#include <memory>
#include <vector>
#include <tuple>
#include <optional>
#include "db.h"

namespace fs = std::filesystem;

class FileService {
public:
    // Метод для получения единственного экземпляра (Singleton)
    static std::shared_ptr<FileService> instance();

    // Конструктор по умолчанию
    FileService();

    // Методы для работы с файлами
    std::vector<std::tuple<int, std::string, int, std::string>> getFiles(const std::string& user_id, int folder_id);
    bool uploadFile(const std::string& user_id, int folder_id, const drogon::HttpRequestPtr &req, std::string &errorMsg);
    bool uploadFile(const std::string& user_id, int folder_id, const drogon::HttpRequestPtr &req, std::string &errorMsg, int group_id = 0);
    bool deleteFiles(const std::string& user_id, const std::vector<int>& file_ids, std::string &errorMsg);
    std::optional<std::string> getFilePath(const std::string& user_id, int file_id);
    bool moveFile(const std::string& user_id, int file_id, int target_folder_id, std::string &errorMsg);
    bool moveFiles(const std::string& user_id, const std::vector<int>& file_ids, int target_folder_id, std::string &errorMsg);
    std::vector<ExtendedFileInfo> getExtendedFiles(const std::string& user_id, int folder_id);

    // Методы для работы с папками
    std::vector<std::tuple<int, std::string, int, std::string>> getFolders(const std::string& user_id, int parent_folder_id = -1);
    bool createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id, std::string &errorMsg);
    bool createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id, std::string &errorMsg, int group_id = 0);
    bool deleteFolder(const std::string& user_id, int folder_id, std::string &errorMsg);
    std::vector<ExtendedFolderInfo> getExtendedFolders(const std::string& user_id, int parent_folder_id = -1);

    std::vector<std::pair<int, std::string>> getUserGroups(const std::string& user_id);
    bool isUserInGroup(const std::string& user_id, int group_id);

    // Методы для работы с избранным
    bool toggleFileFavorite(const std::string& user_id, int file_id, bool is_favorite, std::string& errorMsg);
    bool toggleFolderFavorite(const std::string& user_id, int folder_id, bool is_favorite, std::string& errorMsg);
    std::vector<ExtendedFileInfo> getFavoriteFiles(const std::string& user_id);
    std::vector<ExtendedFolderInfo> getFavoriteFolders(const std::string& user_id);

private:
    std::shared_ptr<DB> db_;
    std::string storagePath_;
};

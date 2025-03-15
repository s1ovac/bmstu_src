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
    bool deleteFiles(const std::string& user_id, const std::vector<int>& file_ids, std::string &errorMsg);
    std::optional<std::string> getFilePath(const std::string& user_id, int file_id);

    // Методы для работы с папками
    std::vector<std::tuple<int, std::string, int, std::string>> getFolders(const std::string& user_id, int parent_folder_id = -1);
    bool createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id, std::string &errorMsg);
    bool deleteFolder(const std::string& user_id, int folder_id, std::string &errorMsg);

private:
    std::shared_ptr<DB> db_;
    std::string storagePath_;
};

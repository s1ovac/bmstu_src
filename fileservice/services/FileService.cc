#include "FileService.h"
#include <drogon/drogon.h>
#include "validation.h"
#include <fstream>

std::shared_ptr<FileService> FileService::instance()
{
    static std::shared_ptr<FileService> instance(new FileService());
    return instance;
}

FileService::FileService()
{
    db_ = DB::instance();

    if (!db_)
    {
        LOG_ERROR << "Database instance is not initialized";
        throw std::runtime_error("Database instance is not initialized");
    }

    // Устанавливаем путь к хранилищу файлов
    storagePath_ = (fs::current_path() / "storage").string();
    if (!fs::exists(storagePath_))
    {
        fs::create_directories(storagePath_);
    }
}

std::vector<std::tuple<int, std::string, int, std::string>> FileService::getFiles(const std::string& user_id, int folder_id)
{
    return db_->getFiles(user_id, folder_id);
}

bool FileService::uploadFile(const std::string& user_id, int folder_id, const drogon::HttpRequestPtr &req, std::string &errorMsg)
{
    // Создаем парсер для multipart/form-data
    drogon::MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0 || fileUpload.getFiles().empty())
    {
        errorMsg = "No files uploaded or failed to parse multipart data";
        return false;
    }

    // Получаем первый загруженный файл
    auto &file = fileUpload.getFiles()[0];

    // Получаем безопасное имя файла
    std::string filename = fs::path(file.getFileName()).filename().string();

    // Validate the filename
    auto validationResult = ValidationUtils::validateName(filename);
    if (!validationResult.valid) {
        errorMsg = "Invalid filename: " + validationResult.errorMessage;
        return false;
    }

    // Путь для сохранения файла
    fs::path filePath = storagePath_ + "/" + filename;

    // Сохраняем файл на диск
    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile)
    {
        errorMsg = "Failed to save file";
        return false;
    }
    outFile.write(file.fileContent().data(), file.fileLength());
    outFile.close();

    // Получаем размер файла
    int file_size = static_cast<int>(fs::file_size(filePath));

    // Сохраняем информацию о файле в базе данных
    if (!db_->insertFile(user_id, folder_id, filename, file_size))
    {
        errorMsg = "Failed to insert file into database";
        return false;
    }

    return true;
}

bool FileService::uploadFile(const std::string& user_id, int folder_id, const drogon::HttpRequestPtr &req, std::string &errorMsg, int group_id)
{
    drogon::MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0 || fileUpload.getFiles().empty())
    {
        errorMsg = "No files uploaded or failed to parse multipart data";
        return false;
    }

    auto &file = fileUpload.getFiles()[0];
    std::string filename = fs::path(file.getFileName()).filename().string();

    auto validationResult = ValidationUtils::validateName(filename);
    if (!validationResult.valid) {
        errorMsg = "Invalid filename: " + validationResult.errorMessage;
        return false;
    }

    fs::path filePath = storagePath_ + "/" + filename;

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile)
    {
        errorMsg = "Failed to save file";
        return false;
    }
    outFile.write(file.fileContent().data(), file.fileLength());
    outFile.close();

    int file_size = static_cast<int>(fs::file_size(filePath));

    if (group_id > 0) {
        if (!db_->insertSharedFile(user_id, folder_id, filename, file_size, group_id))
        {
            errorMsg = "Failed to insert shared file into database";
            return false;
        }
    } else {
        if (!db_->insertFile(user_id, folder_id, filename, file_size))
        {
            errorMsg = "Failed to insert file into database";
            return false;
        }
    }

    return true;
}

bool FileService::createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id, std::string &errorMsg, int group_id)
{
    auto validationResult = ValidationUtils::validateName(folder_name);
    if (!validationResult.valid) {
        errorMsg = "Invalid folder name: " + validationResult.errorMessage;
        return false;
    }

    if (group_id > 0) {
        if (!db_->createSharedFolder(user_id, folder_name, parent_folder_id, group_id))
        {
            errorMsg = "Failed to create shared folder in database";
            return false;
        }
    } else {
        if (!db_->createFolder(user_id, folder_name, parent_folder_id))
        {
            errorMsg = "Failed to create folder in database";
            return false;
        }
    }
    return true;
}

bool FileService::deleteFiles(const std::string& user_id, const std::vector<int>& file_ids, std::string &errorMsg)
{
    for (int file_id : file_ids)
    {
        if (!db_->canUserModifyFile(user_id, file_id))
        {
            errorMsg = "Permission denied: cannot modify file " + std::to_string(file_id);
            return false;
        }

        auto fileNameOpt = db_->getFilePath(user_id, file_id);
        if (!fileNameOpt.has_value())
        {
            errorMsg = "File not found";
            return false;
        }

        fs::path filePath = storagePath_ + "/" + fileNameOpt.value();

        if (!db_->deleteFile(user_id, file_id))
        {
            errorMsg = "Failed to delete file from database";
            return false;
        }

        if (fs::exists(filePath))
        {
            fs::remove(filePath);
        }
    }

    return true;
}

bool FileService::deleteFolder(const std::string& user_id, int folder_id, std::string &errorMsg)
{
    if (!db_->canUserModifyFolder(user_id, folder_id))
    {
        errorMsg = "Permission denied: cannot modify folder";
        return false;
    }

    if (!db_->deleteFolder(user_id, folder_id))
    {
        errorMsg = "Failed to delete folder from database";
        return false;
    }
    return true;
}

bool FileService::moveFile(const std::string& user_id, int file_id, int target_folder_id, std::string &errorMsg)
{
    if (!db_->canUserModifyFile(user_id, file_id))
    {
        errorMsg = "Permission denied: cannot modify file";
        return false;
    }

    if (target_folder_id > 0 && !db_->canUserAccessFolder(user_id, target_folder_id))
    {
        errorMsg = "Permission denied: cannot access target folder";
        return false;
    }

    if (!db_->moveFile(user_id, file_id, target_folder_id))
    {
        errorMsg = "Failed to move file";
        return false;
    }
    return true;
}

bool FileService::moveFiles(const std::string& user_id, const std::vector<int>& file_ids, int target_folder_id, std::string &errorMsg)
{
    for (int file_id : file_ids)
    {
        if (!db_->canUserModifyFile(user_id, file_id))
        {
            errorMsg = "Permission denied: cannot modify file " + std::to_string(file_id);
            return false;
        }
    }

    if (target_folder_id > 0 && !db_->canUserAccessFolder(user_id, target_folder_id))
    {
        errorMsg = "Permission denied: cannot access target folder";
        return false;
    }

    if (!db_->moveFiles(user_id, file_ids, target_folder_id))
    {
        errorMsg = "Failed to move files";
        return false;
    }
    return true;
}

std::optional<std::string> FileService::getFilePath(const std::string& user_id, int file_id)
{
    if (!db_->canUserAccessFile(user_id, file_id))
    {
        return std::nullopt;
    }

    auto fileNameOpt = db_->getFilePath(user_id, file_id);

    if (fileNameOpt.has_value())
    {
        fs::path filePath = storagePath_ + "/" + fileNameOpt.value();
        return filePath.string();
    }
    else
    {
        return std::nullopt;
    }
}

// Методы для работы с папками

std::vector<std::tuple<int, std::string, int, std::string>> FileService::getFolders(const std::string& user_id, int parent_folder_id)
{
    return db_->getFolders(user_id, parent_folder_id);
}

bool FileService::createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id, std::string &errorMsg)
{
    // Validate the folder name
    auto validationResult = ValidationUtils::validateName(folder_name);
    if (!validationResult.valid) {
        errorMsg = "Invalid folder name: " + validationResult.errorMessage;
        return false;
    }

    if (!db_->createFolder(user_id, folder_name, parent_folder_id))
    {
        errorMsg = "Failed to create folder in database";
        return false;
    }
    return true;
}

std::vector<ExtendedFileInfo> FileService::getExtendedFiles(const std::string& user_id, int folder_id)
{
    return db_->getExtendedFiles(user_id, folder_id);
}

std::vector<ExtendedFolderInfo> FileService::getExtendedFolders(const std::string& user_id, int parent_folder_id)
{
    return db_->getExtendedFolders(user_id, parent_folder_id);
}

std::vector<std::pair<int, std::string>> FileService::getUserGroups(const std::string& user_id)
{
    return db_->getUserGroups(std::stoi(user_id));
}

bool FileService::isUserInGroup(const std::string& user_id, int group_id)
{
    auto user_groups = getUserGroups(user_id);
    for (const auto& group : user_groups)
    {
        if (group.first == group_id)
        {
            return true;
        }
    }
    return false;
}

bool FileService::toggleFileFavorite(const std::string& user_id, int file_id, bool is_favorite, std::string& errorMsg)
{
    if (!db_->toggleFileFavorite(user_id, file_id, is_favorite)) {
        errorMsg = "Failed to update file favorite status or access denied";
        return false;
    }

    return true;
}

bool FileService::toggleFolderFavorite(const std::string& user_id, int folder_id, bool is_favorite, std::string& errorMsg)
{
    if (!db_->toggleFolderFavorite(user_id, folder_id, is_favorite)) {
        errorMsg = "Failed to update folder favorite status or access denied";
        return false;
    }

    return true;
}

std::vector<ExtendedFileInfo> FileService::getFavoriteFiles(const std::string& user_id)
{
    return db_->getFavoriteFiles(user_id);
}

std::vector<ExtendedFolderInfo> FileService::getFavoriteFolders(const std::string& user_id)
{
    return db_->getFavoriteFolders(user_id);
}

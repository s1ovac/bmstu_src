#include "AdminService.h"
#include <drogon/drogon.h>
#include <algorithm>
#include <unordered_map>
#include <filesystem>

std::shared_ptr<AdminService> AdminService::instance()
{
    static std::shared_ptr<AdminService> instance(new AdminService());
    return instance;
}

AdminService::AdminService()
{
    LOG_INFO << "Initializing AdminService";
    db_ = DB::instance();

    if (!db_)
    {
        LOG_ERROR << "Database instance is not initialized";
        throw std::runtime_error("Database instance is not initialized");
    }
}

std::vector<UserFilesData> AdminService::getAllFiles()
{
    LOG_INFO << "Getting all files for all users";

    std::vector<UserFilesData> result;

    try {
        // Get all users
        auto users = db_->getAllUsers();

        // Use the user data to create a map for easy access
        std::unordered_map<int, std::string> userEmailMap;
        for (const auto& [userId, email, _] : users) {
            userEmailMap[userId] = email;
        }

        // Get all files with user information
        auto allFiles = db_->getAllFilesAdmin();

        // Group files by user
        std::unordered_map<int, std::vector<FileInfo>> userFilesMap;

        for (const auto& [fileId, fileName, fileSize, createdAt, userId, _] : allFiles) {
            FileInfo file;
            file.file_id = fileId;
            file.file_name = fileName;
            file.file_size = fileSize;
            file.folder_id = 0; // This should be updated if folder information is needed
            file.created_at = createdAt;

            userFilesMap[userId].push_back(file);
        }

        // Convert the map to the result format
        for (const auto& [userId, files] : userFilesMap) {
            if (userEmailMap.find(userId) != userEmailMap.end()) {
                UserFilesData userData;
                userData.user_id = std::to_string(userId);
                userData.email = userEmailMap[userId];
                userData.files = files;

                result.push_back(userData);
            }
        }

        LOG_INFO << "Retrieved files for " << result.size() << " users";

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getAllFiles: " << e.what();
    }

    return result;
}

std::vector<UserFoldersData> AdminService::getAllFolders()
{
    LOG_INFO << "Getting all folders for all users";

    std::vector<UserFoldersData> result;

    try {
        // Get all users
        auto users = db_->getAllUsers();

        // Use the user data to create a map for easy access
        std::unordered_map<int, std::string> userEmailMap;
        for (const auto& [userId, email, _] : users) {
            userEmailMap[userId] = email;
        }

        // Get all folders with user information
        auto allFolders = db_->getAllFoldersAdmin();

        // Group folders by user
        std::unordered_map<int, std::vector<FolderInfo>> userFoldersMap;

        for (const auto& [folderId, folderName, parentFolderId, createdAt, userId, _] : allFolders) {
            FolderInfo folder;
            folder.folder_id = folderId;
            folder.folder_name = folderName;
            folder.parent_folder_id = parentFolderId;
            folder.created_at = createdAt;

            userFoldersMap[userId].push_back(folder);
        }

        // Convert the map to the result format
        for (const auto& [userId, folders] : userFoldersMap) {
            if (userEmailMap.find(userId) != userEmailMap.end()) {
                UserFoldersData userData;
                userData.user_id = std::to_string(userId);
                userData.email = userEmailMap[userId];
                userData.folders = folders;

                result.push_back(userData);
            }
        }

        LOG_INFO << "Retrieved folders for " << result.size() << " users";

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getAllFolders: " << e.what();
    }

    return result;
}

std::optional<UserContentData> AdminService::getUserContent(const std::string& user_id)
{
    LOG_INFO << "Getting content for user " << user_id;

    try {
        // Check if user exists and get email
        auto users = db_->getAllUsers();

        // Find user in the list
        auto userIt = std::find_if(users.begin(), users.end(),
                                   [&user_id](const auto& user) {
                                       return std::to_string(std::get<0>(user)) == user_id;
                                   });

        if (userIt == users.end()) {
            LOG_ERROR << "User not found: " << user_id;
            return std::nullopt;
        }

        std::string email = std::get<1>(*userIt);

        // Create result
        UserContentData result;
        result.user_id = user_id;
        result.email = email;

        // Get files for this user
        auto files = db_->getFilesForUser(user_id);
        for (const auto& [fileId, fileName, fileSize, createdAt] : files) {
            FileInfo file;
            file.file_id = fileId;
            file.file_name = fileName;
            file.file_size = fileSize;
            file.folder_id = 0; // This should be updated if folder information is needed
            file.created_at = createdAt;

            result.files.push_back(file);
        }

        // Get folders for this user
        auto folders = db_->getFoldersForUser(user_id);
        for (const auto& [folderId, folderName, parentFolderId, createdAt] : folders) {
            FolderInfo folder;
            folder.folder_id = folderId;
            folder.folder_name = folderName;
            folder.parent_folder_id = parentFolderId;
            folder.created_at = createdAt;

            result.folders.push_back(folder);
        }

        LOG_INFO << "Retrieved " << result.files.size() << " files and " << result.folders.size()
                 << " folders for user " << user_id;

        return result;

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getUserContent: " << e.what();
        return std::nullopt;
    }
}

SystemStats AdminService::getSystemStats()
{
    LOG_INFO << "Getting system statistics";

    SystemStats stats;
    stats.total_users = 0;
    stats.total_files = 0;
    stats.total_folders = 0;
    stats.total_storage_bytes = 0;

    try {
        // Get the basic system stats
        auto [users, files, folders, storage] = db_->getSystemStats();

        stats.total_users = users;
        stats.total_files = files;
        stats.total_folders = folders;
        stats.total_storage_bytes = storage;

        // Get file type distribution
        stats.files_by_type = db_->getFileTypeDistribution();

        // Get top users by storage
        auto topUsers = db_->getTopUsersByStorage(5);
        for (const auto& [userId, email, storageBytes] : topUsers) {
            std::string userDisplay = userId + " (" + email + ")";
            stats.top_users_by_storage.emplace_back(userDisplay, static_cast<int>(storageBytes));
        }

        LOG_INFO << "Retrieved system stats: "
                 << stats.total_users << " users, "
                 << stats.total_files << " files, "
                 << stats.total_folders << " folders, "
                 << stats.total_storage_bytes << " bytes total storage";

    } catch (const std::exception& e) {
        LOG_ERROR << "Error in getSystemStats: " << e.what();
    }

    return stats;
}
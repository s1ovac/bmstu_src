#pragma once

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include "db.h"

/**
 * Represents a file in the system
 */
struct FileInfo {
    int file_id;
    std::string file_name;
    int file_size;
    int folder_id;
    std::string created_at;
};

/**
 * Represents a folder in the system
 */
struct FolderInfo {
    int folder_id;
    std::string folder_name;
    int parent_folder_id;
    std::string created_at;
};

/**
 * Represents a user's file data
 */
struct UserFilesData {
    std::string user_id;
    std::string email;
    std::vector<FileInfo> files;
};

/**
 * Represents a user's folder data
 */
struct UserFoldersData {
    std::string user_id;
    std::string email;
    std::vector<FolderInfo> folders;
};

/**
 * Represents all content for a specific user
 */
struct UserContentData {
    std::string user_id;
    std::string email;
    std::vector<FileInfo> files;
    std::vector<FolderInfo> folders;
};

/**
 * System statistics structure
 */
struct SystemStats {
    int total_users;
    int total_files;
    int total_folders;
    long long total_storage_bytes;
    std::vector<std::pair<std::string, int>> files_by_type;
    std::vector<std::pair<std::string, int>> top_users_by_storage;
};

/**
 * Service for administrative operations
 */
class AdminService {
public:
    /**
     * Get the singleton instance of the AdminService
     */
    static std::shared_ptr<AdminService> instance();

    /**
     * Get all files in the system grouped by user
     */
    std::vector<UserFilesData> getAllFiles();

    /**
     * Get all folders in the system grouped by user
     */
    std::vector<UserFoldersData> getAllFolders();

    /**
     * Get all content (files and folders) for a specific user
     */
    std::optional<UserContentData> getUserContent(const std::string& user_id);

    /**
     * Get system-wide statistics
     */
    SystemStats getSystemStats();

private:
    /**
     * Private constructor for singleton pattern
     */
    AdminService();

    /**
     * Database connection
     */
    std::shared_ptr<DB> db_;
};
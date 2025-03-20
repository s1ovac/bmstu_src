#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <libpq-fe.h>
#include <memory>

class DB {
public:
    // Метод для получения единственного экземпляра (Singleton)
    static std::shared_ptr<DB> instance();

    // Метод для инициализации экземпляра
    static void initInstance(const std::string& host, const std::string& port,
                             const std::string& dbname, const std::string& user,
                             const std::string& password);

    ~DB();

    bool init();
    PGconn* getConnection();

    // Методы для работы с файлами и папками
    std::vector<std::tuple<int, std::string, int, std::string>> getFiles(const std::string& user_id, int folder_id);
    bool insertFile(const std::string& user_id, int folder_id, const std::string& file_name, int file_size);
    bool deleteFile(const std::string& user_id, int file_id);
    std::optional<std::string> getFilePath(const std::string& user_id, int file_id);
    bool moveFile(const std::string& user_id, int file_id, int target_folder_id);
    bool moveFiles(const std::string& user_id, const std::vector<int>& file_ids, int target_folder_id);

    std::vector<std::tuple<int, std::string, int, std::string>> getFolders(const std::string& user_id, int parent_folder_id = -1);
    bool createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id = -1);
    bool deleteFolder(const std::string& user_id, int folder_id);

    // Admin methods
    std::vector<std::tuple<int, std::string, std::string>> getAllUsers();
    std::vector<std::tuple<int, std::string, int, std::string, int, std::string>> getAllFilesAdmin();
    std::vector<std::tuple<int, std::string, int, std::string, int, std::string>> getAllFoldersAdmin();
    std::vector<std::tuple<int, std::string, int, std::string>> getFilesForUser(const std::string& user_id);
    std::vector<std::tuple<int, std::string, int, std::string>> getFoldersForUser(const std::string& user_id);

    // System statistics
    std::tuple<int, int, int, long long> getSystemStats();
    std::vector<std::pair<std::string, int>> getFileTypeDistribution();
    std::vector<std::tuple<std::string, std::string, long long>> getTopUsersByStorage(int limit = 5);

private:
    // Приватный конструктор
    DB(const std::string& host, const std::string& port,
       const std::string& dbname, const std::string& user,
       const std::string& password);

    // Статический экземпляр
    static std::shared_ptr<DB> instance_;

    std::string host_;
    std::string port_;
    std::string dbname_;
    std::string user_;
    std::string password_;
    PGconn* conn_;

    void connect();
};
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

    std::vector<std::tuple<int, std::string, int, std::string>> getFolders(const std::string& user_id, int parent_folder_id = -1);
    bool createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id = -1);
    bool deleteFolder(const std::string& user_id, int folder_id);

    // Методы для работы с ролями и разрешениями
    bool createRole(const std::string &role_name, const std::string &description);
    bool assignPermissionsToRole(int role_id, const std::vector<int> &permission_ids);
    std::vector<std::string> getUserRoles(const std::string &user_id);
    bool roleHasPermission(const std::string &role_name, const std::string &permission_name);
    bool userHasPermission(const std::string &user_id, const std::string &permission_name);
    std::vector<std::string> getUserPermissions(const std::string &user_id);
    bool deleteRole(int role_id);
    bool assignRolesToUser(int user_id, const std::vector<int> &role_ids);
    std::vector<std::pair<int, std::string>> getUserRolesWithIds(int user_id);
    std::vector<std::pair<int, std::string>> getAllRoles();
    std::vector<std::tuple<int, std::string, std::vector<std::string>>> getAllUsersWithRoles();

    // Методы для работы с группами
    bool createGroup(const std::string &group_name);
    bool deleteGroup(int group_id);
    bool renameGroup(int group_id, const std::string &new_name);
    bool addUserToGroup(int user_id, int group_id);
    bool removeUserFromGroup(int user_id, int group_id);
    std::vector<std::pair<int, std::string>> getUserGroups(int user_id);
    std::vector<std::pair<int, std::string>> getAllGroups();

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

#ifndef AUTH_USER_DB_H
#define AUTH_USER_DB_H

#include <string>
#include <utility>
#include <vector>
#include <tuple>
#include <postgresql@14/libpq-fe.h>

enum class UserFetchStatus {
    UserNotFound,
    QueryFailed,
    Success
};

enum class CreateUserStatus {
    Success,
    UserAlreadyExists,
    QueryFailed
};

class DB {
public:
    DB(std::string host, std::string port, std::string dbname, std::string user, std::string password);
    ~DB();

    bool init();
    std::tuple<std::string, std::string, UserFetchStatus> getPasswordHashByLogin(const std::string& login);
    CreateUserStatus createUser(const std::string& login, const std::string& password_hash);

    // User roles and permissions
    std::vector<std::string> getUserRoles(const std::string& userId);
    bool createRole(const std::string &role_name, const std::string &description);
    bool deleteRole(int role_id);
    bool assignPermissionsToRole(int role_id, const std::vector<int> &permission_ids);
    bool assignRolesToUser(int user_id, const std::vector<int> &role_ids);
    std::vector<std::pair<int, std::string>> getUserRolesWithIds(int user_id);
    std::vector<std::pair<int, std::string>> getAllRoles();
    std::vector<std::tuple<int, std::string, std::vector<std::string>>> getAllUsersWithRoles();
    bool roleHasPermission(const std::string &role_name, const std::string &permission_name);
    std::vector<std::string> getUserPermissions(const std::string &user_id);
    bool userHasPermission(const std::string &user_id, const std::string &permission_name);

    // Groups management
    bool createGroup(const std::string &group_name);
    bool deleteGroup(int group_id);
    bool renameGroup(int group_id, const std::string &new_name);
    bool addUserToGroup(int user_id, int group_id);
    bool removeUserFromGroup(int user_id, int group_id);
    std::vector<std::pair<int, std::string>> getUserGroups(int user_id);
    std::vector<std::pair<int, std::string>> getAllGroups();

private:
    std::string host_;
    std::string port_;
    std::string dbname_;
    std::string user_;
    std::string password_;
    PGconn* conn_;

    void connect();
};

#endif //AUTH_USER_DB_H
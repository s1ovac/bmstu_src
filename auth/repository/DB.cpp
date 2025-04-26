#include "DB.h"
#include <iostream>
#include <vector>
#include <tuple>
#include <optional>

std::shared_ptr<DB> DB::instance_ = nullptr;

std::shared_ptr<DB> DB::instance()
{
    if (!instance_)
    {
        throw std::runtime_error("DB instance is not initialized. Call DB::initInstance() first.");
    }
    return instance_;
}

void DB::initInstance(const std::string& host, const std::string& port,
                      const std::string& dbname, const std::string& user,
                      const std::string& password)
{
    if (!instance_)
    {
        instance_ = std::shared_ptr<DB>(new DB(host, port, dbname, user, password));
        if (!instance_->init())
        {
            throw std::runtime_error("Failed to initialize the database");
        }
    }
}

DB::DB(const std::string& host, const std::string& port,
       const std::string& dbname, const std::string& user,
       const std::string& password)
        : host_(host), port_(port), dbname_(dbname), user_(user), password_(password), conn_(nullptr)
{
    connect();
}

DB::~DB()
{
    if (conn_)
    {
        PQfinish(conn_);
    }
}

void DB::connect()
{
    if (!conn_)
    {
        std::string connInfo =
                "host=" + host_ +
                " port=" + port_ +
                " dbname=" + dbname_ +
                " user=" + user_ +
                " password=" + password_;
        conn_ = PQconnectdb(connInfo.c_str());

        if (PQstatus(conn_) != CONNECTION_OK)
        {
            std::cerr << "Connection to database failed: " << PQerrorMessage(conn_) << std::endl;
            PQfinish(conn_);
            conn_ = nullptr;
        }
    }
}

bool DB::init()
{
    if (!conn_) return false;

    // Create tables for users, roles, permissions, groups
    std::vector<std::string> queries = {
            // Users table
            R"(
            CREATE TABLE IF NOT EXISTS users (
                user_id SERIAL PRIMARY KEY,
                email VARCHAR(100) NOT NULL UNIQUE,
                password_hash VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                last_login TIMESTAMP,
                is_active BOOLEAN DEFAULT TRUE
            );
        )",

            // Roles table
            R"(
            CREATE TABLE IF NOT EXISTS roles (
                role_id SERIAL PRIMARY KEY,
                role_name VARCHAR(50) NOT NULL UNIQUE,
                description TEXT
            );
        )",

            // Permissions table
            R"(
            CREATE TABLE IF NOT EXISTS permissions (
                permission_id SERIAL PRIMARY KEY,
                permission_name VARCHAR(50) NOT NULL UNIQUE,
                description TEXT
            );
        )",

            // User-Role mapping table
            R"(
            CREATE TABLE IF NOT EXISTS user_roles (
                user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
                role_id INT REFERENCES roles(role_id) ON DELETE CASCADE,
                PRIMARY KEY (user_id, role_id)
            );
        )",

            // Role-Permission mapping table
            R"(
            CREATE TABLE IF NOT EXISTS role_permissions (
                role_id INT REFERENCES roles(role_id) ON DELETE CASCADE,
                permission_id INT REFERENCES permissions(permission_id) ON DELETE CASCADE,
                PRIMARY KEY (role_id, permission_id)
            );
        )",

            // Groups table
            R"(
            CREATE TABLE IF NOT EXISTS groups (
                group_id SERIAL PRIMARY KEY,
                group_name VARCHAR(100) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )",

            // User-Group mapping table
            R"(
            CREATE TABLE IF NOT EXISTS user_groups (
                user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
                group_id INT REFERENCES groups(group_id) ON DELETE CASCADE,
                PRIMARY KEY (user_id, group_id)
            );
        )",

            // User activity logs table
            R"(
            CREATE TABLE IF NOT EXISTS user_activity_logs (
                log_id SERIAL PRIMARY KEY,
                user_id INT REFERENCES users(user_id) ON DELETE SET NULL,
                action VARCHAR(100) NOT NULL,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )"
    };

    for (const auto& query : queries)
    {
        PGresult *res = PQexec(conn_, query.c_str());
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::cerr << "Failed to execute query: " << PQerrorMessage(conn_) << std::endl;
            PQclear(res);
            return false;
        }
        PQclear(res);
    }

    return true;
}

std::tuple<std::string, std::string, UserFetchStatus> DB::getPasswordHashByLogin(const std::string& login)
{
    if (!conn_) return {"", "", UserFetchStatus::QueryFailed};

    // Запрашиваем id и password_hash
    std::string query = "SELECT user_id, password_hash FROM users WHERE email = $1;";
    const char* paramValues[1] = { login.c_str() };

    PGresult *res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Query failed: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return {"", "", UserFetchStatus::QueryFailed};
    }

    std::string userId;
    std::string hash;
    UserFetchStatus status;

    if (PQntuples(res) > 0)
    {
        userId = PQgetvalue(res, 0, 0);
        hash = PQgetvalue(res, 0, 1);
        status = UserFetchStatus::Success;
    }
    else
    {
        status = UserFetchStatus::UserNotFound;
    }

    PQclear(res);
    return {userId, hash, status};
}

std::tuple<std::string, std::string, UserFetchStatus> DB::getPasswordHashByUserID(const std::string& userID)
{
    if (!conn_) return {"", "", UserFetchStatus::QueryFailed};

    // Запрашиваем id и password_hash
    std::string query = "SELECT user_id, password_hash FROM users WHERE user_id = $1;";
    const char* paramValues[1] = { userID.c_str() };

    PGresult *res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Query failed: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return {"", "", UserFetchStatus::QueryFailed};
    }

    std::string userId;
    std::string hash;
    UserFetchStatus status;

    if (PQntuples(res) > 0)
    {
        userId = PQgetvalue(res, 0, 0);
        hash = PQgetvalue(res, 0, 1);
        status = UserFetchStatus::Success;
    }
    else
    {
        status = UserFetchStatus::UserNotFound;
    }

    PQclear(res);
    return {userId, hash, status};
}

CreateUserStatus DB::createUser(const std::string& login, const std::string& password_hash)
{
    if (!conn_) return CreateUserStatus::QueryFailed;

    std::string query = "INSERT INTO users (email, password_hash) VALUES ($1, $2);";
    const char* paramValues[2] = { login.c_str(), password_hash.c_str() };

    PGresult *res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        // Check if the error is due to a duplicate
        std::string errorMsg = PQerrorMessage(conn_);
        if (errorMsg.find("duplicate key") != std::string::npos)
        {
            PQclear(res);
            return CreateUserStatus::UserAlreadyExists;
        }

        std::cerr << "Insert failed: " << errorMsg << std::endl;
        PQclear(res);
        return CreateUserStatus::QueryFailed;
    }

    PQclear(res);
    return CreateUserStatus::Success;
}

// Role and Permission Methods

std::vector<std::string> DB::getUserRoles(const std::string& userId)
{
    std::vector<std::string> roles;
    if (!conn_) return roles;

    std::string query = R"(
        SELECT r.role_name
        FROM roles r
        INNER JOIN user_roles ur ON r.role_id = ur.role_id
        WHERE ur.user_id = $1;
    )";

    const char* paramValues[1] = { userId.c_str() };

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get user roles: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return roles;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        std::string role_name = PQgetvalue(res, i, 0);
        roles.push_back(role_name);
    }

    PQclear(res);
    return roles;
}

bool DB::createRole(const std::string &role_name, const std::string &description)
{
    if (!conn_) return false;

    std::string query = R"(
        INSERT INTO roles (role_name, description)
        VALUES ($1, $2);
    )";

    const char* paramValues[2];
    paramValues[0] = role_name.c_str();
    paramValues[1] = description.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to create role: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::deleteRole(int role_id)
{
    if (!conn_) return false;

    std::string query = "DELETE FROM roles WHERE role_id = $1;";
    const char* paramValues[1];
    std::string roleIdStr = std::to_string(role_id);
    paramValues[0] = roleIdStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to delete role: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::assignPermissionsToRole(int role_id, const std::vector<int> &permission_ids)
{
    if (!conn_) return false;

    std::string query = R"(
        INSERT INTO role_permissions (role_id, permission_id)
        VALUES ($1, $2)
        ON CONFLICT DO NOTHING;
    )";

    for (const auto& permission_id : permission_ids)
    {
        const char* paramValues[2];
        std::string roleIdStr = std::to_string(role_id);
        std::string permIdStr = std::to_string(permission_id);

        paramValues[0] = roleIdStr.c_str();
        paramValues[1] = permIdStr.c_str();

        PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::cerr << "Failed to assign permission to role: " << PQerrorMessage(conn_) << std::endl;
            PQclear(res);
            return false;
        }
        PQclear(res);
    }

    return true;
}

bool DB::assignRolesToUser(int user_id, const std::vector<int> &role_ids)
{
    if (!conn_) return false;

    std::string query = R"(
        INSERT INTO user_roles (user_id, role_id)
        VALUES ($1, $2)
        ON CONFLICT DO NOTHING;
    )";

    for (const auto& role_id : role_ids)
    {
        const char* paramValues[2];
        std::string userIdStr = std::to_string(user_id);
        std::string roleIdStr = std::to_string(role_id);

        paramValues[0] = userIdStr.c_str();
        paramValues[1] = roleIdStr.c_str();

        PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::cerr << "Failed to assign role to user: " << PQerrorMessage(conn_) << std::endl;
            PQclear(res);
            return false;
        }
        PQclear(res);
    }

    return true;
}

std::vector<std::pair<int, std::string>> DB::getUserRolesWithIds(int user_id)
{
    std::vector<std::pair<int, std::string>> roles;
    if (!conn_) return roles;

    std::string query = R"(
        SELECT r.role_id, r.role_name
        FROM roles r
        INNER JOIN user_roles ur ON r.role_id = ur.role_id
        WHERE ur.user_id = $1;
    )";

    const char* paramValues[1];
    std::string userIdStr = std::to_string(user_id);
    paramValues[0] = userIdStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get user roles: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return roles;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int role_id = std::stoi(PQgetvalue(res, i, 0));
        std::string role_name = PQgetvalue(res, i, 1);
        roles.emplace_back(role_id, role_name);
    }

    PQclear(res);
    return roles;
}

bool DB::roleHasPermission(const std::string &role_name, const std::string &permission_name)
{
    if (!conn_) return false;

    std::string query = R"(
        SELECT 1
        FROM roles r
        INNER JOIN role_permissions rp ON r.role_id = rp.role_id
        INNER JOIN permissions p ON rp.permission_id = p.permission_id
        WHERE r.role_name = $1
          AND p.permission_name = $2
        LIMIT 1;
    )";

    const char* paramValues[2];
    paramValues[0] = role_name.c_str();
    paramValues[1] = permission_name.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to check role permission: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    bool hasPermission = (PQntuples(res) > 0);
    PQclear(res);
    return hasPermission;
}

std::vector<std::string> DB::getUserPermissions(const std::string &user_id)
{
    std::vector<std::string> permissions;
    if (!conn_) return permissions;

    std::string query = R"(
        SELECT DISTINCT p.permission_name
        FROM permissions p
        INNER JOIN role_permissions rp ON p.permission_id = rp.permission_id
        INNER JOIN roles r ON rp.role_id = r.role_id
        INNER JOIN user_roles ur ON r.role_id = ur.role_id
        WHERE ur.user_id = $1;
    )";

    const char* paramValues[1];
    paramValues[0] = user_id.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get user permissions: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return permissions;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        std::string permission_name = PQgetvalue(res, i, 0);
        permissions.push_back(permission_name);
    }

    PQclear(res);
    return permissions;
}

bool DB::userHasPermission(const std::string &user_id, const std::string &permission_name)
{
    if (!conn_) return false;

    std::string query = R"(
        SELECT 1
        FROM permissions p
        INNER JOIN role_permissions rp ON p.permission_id = rp.permission_id
        INNER JOIN roles r ON rp.role_id = r.role_id
        INNER JOIN user_roles ur ON r.role_id = ur.role_id
        WHERE ur.user_id = $1
          AND p.permission_name = $2
        LIMIT 1;
    )";

    const char* paramValues[2];
    paramValues[0] = user_id.c_str();
    paramValues[1] = permission_name.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to check user permission: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    bool hasPermission = (PQntuples(res) > 0);
    PQclear(res);
    return hasPermission;
}

std::vector<std::pair<int, std::string>> DB::getAllRoles()
{
    std::vector<std::pair<int, std::string>> roles;
    if (!conn_) return roles;

    std::string query = R"(
        SELECT role_id, role_name
        FROM roles
        ORDER BY role_id;
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get all roles: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return roles;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int role_id = std::stoi(PQgetvalue(res, i, 0));
        std::string role_name = PQgetvalue(res, i, 1);
        roles.emplace_back(role_id, role_name);
    }

    PQclear(res);
    return roles;
}

std::vector<std::tuple<int, std::string, std::vector<std::string>>> DB::getAllUsersWithRoles()
{
    // Возвращаем кортеж (user_id, email, vector<role_name>)
    std::vector<std::tuple<int, std::string, std::vector<std::string>>> result;
    if (!conn_) return result;

    std::string query = R"(
        SELECT u.user_id, u.email,
               COALESCE(array_agg(r.role_name), '{}') AS roles
        FROM users u
        LEFT JOIN user_roles ur ON u.user_id = ur.user_id
        LEFT JOIN roles r      ON ur.role_id = r.role_id
        GROUP BY u.user_id, u.email
        ORDER BY u.user_id;
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to getAllUsersWithRoles: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return result;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int user_id = std::stoi(PQgetvalue(res, i, 0));
        std::string email = PQgetvalue(res, i, 1);

        std::string rolesStr = PQgetvalue(res, i, 2); // например: "{admin,other}"
        std::vector<std::string> rolesVec;
        if (rolesStr.size() >= 2 && rolesStr.front() == '{' && rolesStr.back() == '}')
        {
            rolesStr = rolesStr.substr(1, rolesStr.size() - 2);
            if (!rolesStr.empty())
            {
                size_t start = 0;
                while (true)
                {
                    auto pos = rolesStr.find(',', start);
                    if (pos == std::string::npos)
                    {
                        rolesVec.push_back(rolesStr.substr(start));
                        break;
                    }
                    rolesVec.push_back(rolesStr.substr(start, pos - start));
                    start = pos + 1;
                }
            }
        }

        result.emplace_back(user_id, email, rolesVec);
    }

    PQclear(res);
    return result;
}

// ===========================================================================
//                     Методы для работы с группами (Groups)
// ===========================================================================
bool DB::createGroup(const std::string &group_name)
{
    if (!conn_) return false;

    std::string query = R"(
        INSERT INTO groups (group_name)
        VALUES ($1);
    )";

    const char* paramValues[1];
    paramValues[0] = group_name.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to create group: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::deleteGroup(int group_id)
{
    if (!conn_) return false;

    std::string query = R"(
        DELETE FROM groups
        WHERE group_id = $1;
    )";

    const char* paramValues[1];
    std::string groupIdStr = std::to_string(group_id);
    paramValues[0] = groupIdStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to delete group: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::renameGroup(int group_id, const std::string &new_name)
{
    if (!conn_) return false;

    std::string query = R"(
        UPDATE groups
        SET group_name = $2
        WHERE group_id = $1;
    )";

    const char* paramValues[2];
    std::string groupIdStr = std::to_string(group_id);
    paramValues[0] = groupIdStr.c_str();
    paramValues[1] = new_name.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to rename group: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::addUserToGroup(int user_id, int group_id)
{
    if (!conn_) return false;

    // Вставим связь (user_id, group_id) в таблицу user_groups
    std::string query = R"(
        INSERT INTO user_groups (user_id, group_id)
        VALUES ($1, $2)
        ON CONFLICT DO NOTHING;
    )";

    const char* paramValues[2];
    std::string userIdStr = std::to_string(user_id);
    std::string groupIdStr = std::to_string(group_id);
    paramValues[0] = userIdStr.c_str();
    paramValues[1] = groupIdStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to add user to group: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

bool DB::removeUserFromGroup(int user_id, int group_id)
{
    if (!conn_) return false;

    std::string query = R"(
        DELETE FROM user_groups
        WHERE user_id = $1 AND group_id = $2;
    )";

    const char* paramValues[2];
    std::string userIdStr = std::to_string(user_id);
    std::string groupIdStr = std::to_string(group_id);
    paramValues[0] = userIdStr.c_str();
    paramValues[1] = groupIdStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to remove user from group: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }
    PQclear(res);
    return true;
}

std::vector<std::pair<int, std::string>> DB::getUserGroups(int user_id)
{
    std::vector<std::pair<int, std::string>> result;
    if (!conn_) return result;

    std::string query = R"(
        SELECT g.group_id, g.group_name
        FROM groups g
        INNER JOIN user_groups ug ON g.group_id = ug.group_id
        WHERE ug.user_id = $1
        ORDER BY g.group_id;
    )";

    const char* paramValues[1];
    std::string userIdStr = std::to_string(user_id);
    paramValues[0] = userIdStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get user groups: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return result;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int group_id = std::stoi(PQgetvalue(res, i, 0));
        std::string group_name = PQgetvalue(res, i, 1);
        result.emplace_back(group_id, group_name);
    }

    PQclear(res);
    return result;
}

std::vector<std::pair<int, std::string>> DB::getAllGroups()
{
    std::vector<std::pair<int, std::string>> result;
    if (!conn_)
        return result;

    std::string query = R"(
        SELECT group_id, group_name
        FROM groups
        ORDER BY group_id
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get all groups: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return result;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int group_id = std::stoi(PQgetvalue(res, i, 0));
        std::string group_name = PQgetvalue(res, i, 1);
        result.emplace_back(group_id, group_name);
    }

    PQclear(res);
    return result;
}

bool DB::updatePasswordHash(const std::string& userId, const std::string& newPasswordHash)
{
    if (!conn_) return false;

    std::string query = "UPDATE users SET password_hash = $1 WHERE user_id = $2;";
    const char* paramValues[2];
    paramValues[0] = newPasswordHash.c_str();
    paramValues[1] = userId.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to update password: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    if (strcmp(PQcmdTuples(res), "0") == 0)
    {
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}
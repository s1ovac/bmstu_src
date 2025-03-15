#include "db.h"
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

    // Создаём (или проверяем) необходимые таблицы

    std::vector<std::string> queries =
            {
                    // Таблица пользователей
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

                    // Таблица папок
                    R"(
            CREATE TABLE IF NOT EXISTS folders (
                folder_id SERIAL PRIMARY KEY,
                user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
                parent_folder_id INT,
                folder_name VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (parent_folder_id) REFERENCES folders(folder_id) ON DELETE CASCADE
            );
        )",

                    // Таблица файлов
                    R"(
            CREATE TABLE IF NOT EXISTS files (
                file_id SERIAL PRIMARY KEY,
                user_id INT NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
                folder_id INT NULL REFERENCES folders(folder_id) ON DELETE CASCADE,
                file_name VARCHAR(255) NOT NULL,
                file_size INT,
                file_type VARCHAR(50),
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )",

                    // Таблица ролей
                    R"(
            CREATE TABLE IF NOT EXISTS roles (
                role_id SERIAL PRIMARY KEY,
                role_name VARCHAR(50) NOT NULL UNIQUE,
                description TEXT
            );
        )",

                    // Таблица разрешений
                    R"(
            CREATE TABLE IF NOT EXISTS permissions (
                permission_id SERIAL PRIMARY KEY,
                permission_name VARCHAR(50) NOT NULL UNIQUE,
                description TEXT
            );
        )",

                    // Таблица связи пользователей и ролей
                    R"(
            CREATE TABLE IF NOT EXISTS user_roles (
                user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
                role_id INT REFERENCES roles(role_id) ON DELETE CASCADE,
                PRIMARY KEY (user_id, role_id)
            );
        )",

                    // Таблица связи ролей и разрешений
                    R"(
            CREATE TABLE IF NOT EXISTS role_permissions (
                role_id INT REFERENCES roles(role_id) ON DELETE CASCADE,
                permission_id INT REFERENCES permissions(permission_id) ON DELETE CASCADE,
                PRIMARY KEY (role_id, permission_id)
            );
        )",

                    // Таблица логов активности пользователей
                    R"(
            CREATE TABLE IF NOT EXISTS user_activity_logs (
                log_id SERIAL PRIMARY KEY,
                user_id INT REFERENCES users(user_id) ON DELETE SET NULL,
                action VARCHAR(100) NOT NULL,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )",

                    // Таблица групп
                    R"(
            CREATE TABLE IF NOT EXISTS groups (
                group_id SERIAL PRIMARY KEY,
                group_name VARCHAR(100) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
        )",

                    // Таблица связи пользователей и групп (many-to-many)
                    R"(
            CREATE TABLE IF NOT EXISTS user_groups (
                user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
                group_id INT REFERENCES groups(group_id) ON DELETE CASCADE,
                PRIMARY KEY (user_id, group_id)
            );
        )"
            };

    for (const auto& query : queries)
    {
        PGresult* res = PQexec(conn_, query.c_str());
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

PGconn* DB::getConnection()
{
    return conn_;
}

// ===========================================================================
//                           Методы для работы с файлами
// ===========================================================================
std::vector<std::tuple<int, std::string, int, std::string>> DB::getFiles(const std::string& user_id, int folder_id)
{
    std::vector<std::tuple<int, std::string, int, std::string>> files;
    if (!conn_) return files;

    // Если folder_id == 0 => folder_id IS NULL
    // Иначе folder_id = $2
    std::string query;
    const char* paramValues[2];
    paramValues[0] = user_id.c_str();

    if (folder_id < 0)
    {
        folder_id = 0;
    }

    if (folder_id == 0)
    {
        // folder_id IS NULL
        query = R"(
            SELECT f.file_id, f.file_name, f.file_size, f.created_at
            FROM files f
            WHERE f.user_id = $1
              AND f.folder_id IS NULL
            ORDER BY f.file_id;
        )";
        // paramCount = 1
    }
    else
    {
        // folder_id = ...
        query = R"(
            SELECT f.file_id, f.file_name, f.file_size, f.created_at
            FROM files f
            WHERE f.user_id = $1
              AND f.folder_id = $2
            ORDER BY f.file_id;
        )";
        // paramCount = 2
        std::string folderIdStr = std::to_string(folder_id);
        paramValues[1] = folderIdStr.c_str();
    }

    int paramCount = (folder_id == 0) ? 1 : 2;

    PGresult* res = PQexecParams(conn_, query.c_str(), paramCount, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Query failed: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return files;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int file_id = std::stoi(PQgetvalue(res, i, 0));
        std::string file_name = PQgetvalue(res, i, 1);
        int file_size = std::stoi(PQgetvalue(res, i, 2));
        std::string created_at = PQgetvalue(res, i, 3);

        files.emplace_back(file_id, file_name, file_size, created_at);
    }

    PQclear(res);
    return files;
}

bool DB::insertFile(const std::string& user_id, int folder_id, const std::string& file_name, int file_size)
{
    if (!conn_) return false;

    // Если folder_id > 0 => проверяем, что такая папка существует
    // Если folder_id == 0 => трактуем как NULL (корневая директория)
    if (folder_id > 0)
    {
        std::string checkFolderQuery = R"(
            SELECT 1 FROM folders WHERE folder_id = $1 AND user_id = $2;
        )";
        const char* folderParamValues[2];
        std::string folderIdStr = std::to_string(folder_id);
        folderParamValues[0] = folderIdStr.c_str();
        folderParamValues[1] = user_id.c_str();

        PGresult* folderRes = PQexecParams(conn_, checkFolderQuery.c_str(), 2, nullptr, folderParamValues, nullptr, nullptr, 0);
        if (PQresultStatus(folderRes) != PGRES_TUPLES_OK)
        {
            std::cerr << "Failed to check folder: " << PQerrorMessage(conn_) << std::endl;
            PQclear(folderRes);
            return false;
        }

        if (PQntuples(folderRes) == 0)
        {
            std::cerr << "Folder with folder_id " << folder_id << " does not exist for user " << user_id << std::endl;
            PQclear(folderRes);
            return false;
        }
        PQclear(folderRes);
    }

    if (folder_id < 0)
    {
        folder_id = 0;
    }

    // Если folder_id == 0 => вставляем NULL
    std::string query = R"(
        INSERT INTO files (user_id, folder_id, file_name, file_size)
        VALUES
        (
            $1,
            CASE WHEN $2::int = 0 THEN NULL ELSE $2::int END,
            $3,
            $4
        );
    )";

    const char* paramValues[4];
    paramValues[0] = user_id.c_str();
    std::string folderIdStr2 = std::to_string(folder_id);
    paramValues[1] = folderIdStr2.c_str();
    paramValues[2] = file_name.c_str();
    std::string fileSizeStr = std::to_string(file_size);
    paramValues[3] = fileSizeStr.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 4, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to insert file: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::deleteFile(const std::string& user_id, int file_id)
{
    if (!conn_) return false;

    std::string query = R"(
        DELETE FROM files
        WHERE file_id = $1 AND user_id = $2;
    )";
    const char* paramValues[2];
    std::string fileIdStr = std::to_string(file_id);
    paramValues[0] = fileIdStr.c_str();
    paramValues[1] = user_id.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to delete file: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

std::optional<std::string> DB::getFilePath(const std::string& user_id, int file_id)
{
    if (!conn_) return std::nullopt;

    std::string query = R"(
        SELECT f.file_name
        FROM files f
        WHERE f.file_id = $1 AND f.user_id = $2;
    )";
    const char* paramValues[2];
    std::string fileIdStr = std::to_string(file_id);
    paramValues[0] = fileIdStr.c_str();
    paramValues[1] = user_id.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get file path: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return std::nullopt;
    }

    if (PQntuples(res) > 0)
    {
        std::string file_name = PQgetvalue(res, 0, 0);
        PQclear(res);
        return file_name;
    }
    else
    {
        PQclear(res);
        return std::nullopt;
    }
}

// ===========================================================================
//                           Методы для работы с папками
// ===========================================================================
std::vector<std::tuple<int, std::string, int, std::string>> DB::getFolders(const std::string& user_id, int parent_folder_id)
{
    std::vector<std::tuple<int, std::string, int, std::string>> folders;
    if (!conn_) return folders;

    // Если parent_folder_id == 0 => parent_folder_id IS NULL
    // Иначе parent_folder_id = $2
    std::string query;
    const char* paramValues[2];
    paramValues[0] = user_id.c_str();

    if (parent_folder_id == 0)
    {
        // parent_folder_id IS NULL
        query = R"(
            SELECT folder_id, folder_name, parent_folder_id, created_at
            FROM folders
            WHERE user_id = $1
              AND parent_folder_id IS NULL
            ORDER BY folder_id;
        )";
    }
    else
    {
        query = R"(
            SELECT folder_id, folder_name, parent_folder_id, created_at
            FROM folders
            WHERE user_id = $1
              AND parent_folder_id = $2
            ORDER BY folder_id;
        )";
        std::string parentIdStr = std::to_string(parent_folder_id);
        paramValues[1] = parentIdStr.c_str();
    }

    int paramCount = (parent_folder_id == 0) ? 1 : 2;

    PGresult* res = PQexecParams(conn_, query.c_str(), paramCount, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get folders: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return folders;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int folder_id = std::stoi(PQgetvalue(res, i, 0));
        std::string folder_name = PQgetvalue(res, i, 1);

        bool is_parent_null = PQgetisnull(res, i, 2);
        int parent_id = is_parent_null ? 0 : std::stoi(PQgetvalue(res, i, 2));

        std::string created_at = PQgetvalue(res, i, 3);

        folders.emplace_back(folder_id, folder_name, parent_id, created_at);
    }

    PQclear(res);
    return folders;
}

bool DB::createFolder(const std::string& user_id, const std::string& folder_name, int parent_folder_id)
{
    if (!conn_) return false;

    // Если parent_folder_id > 0 => проверяем, что такая папка существует
    // Если parent_folder_id == 0 => трактуем как NULL
    if (parent_folder_id > 0)
    {
        std::string checkParentFolderQuery = R"(
            SELECT 1 FROM folders WHERE folder_id = $1 AND user_id = $2;
        )";

        const char* parentFolderParamValues[2];
        std::string parentIdStr = std::to_string(parent_folder_id);
        parentFolderParamValues[0] = parentIdStr.c_str();
        parentFolderParamValues[1] = user_id.c_str();

        PGresult* parentFolderRes = PQexecParams(conn_, checkParentFolderQuery.c_str(), 2, nullptr, parentFolderParamValues, nullptr, nullptr, 0);
        if (PQresultStatus(parentFolderRes) != PGRES_TUPLES_OK)
        {
            std::cerr << "Failed to check parent folder: " << PQerrorMessage(conn_) << std::endl;
            PQclear(parentFolderRes);
            return false;
        }

        if (PQntuples(parentFolderRes) == 0)
        {
            std::cerr << "Parent folder with folder_id " << parent_folder_id << " does not exist for user " << user_id << std::endl;
            PQclear(parentFolderRes);
            return false;
        }
        PQclear(parentFolderRes);
    }

    std::string query = R"(
        INSERT INTO folders (user_id, folder_name, parent_folder_id)
        VALUES
        (
            $1,
            $2,
            CASE WHEN $3::int = 0 THEN NULL ELSE $3::int END
        );
    )";

    const char* paramValues[3];
    paramValues[0] = user_id.c_str();
    paramValues[1] = folder_name.c_str();
    std::string parentIdStr2 = std::to_string(parent_folder_id);
    paramValues[2] = parentIdStr2.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 3, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to create folder: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

bool DB::deleteFolder(const std::string& user_id, int folder_id)
{
    if (!conn_) return false;

    std::string query = R"(
        DELETE FROM folders
        WHERE folder_id = $1 AND user_id = $2;
    )";
    const char* paramValues[2];
    std::string folderIdStr = std::to_string(folder_id);
    paramValues[0] = folderIdStr.c_str();
    paramValues[1] = user_id.c_str();

    PGresult* res = PQexecParams(conn_, query.c_str(), 2, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to delete folder: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

// ===========================================================================
//                        Методы для работы с ролями / RBAC
// ===========================================================================
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

std::vector<std::string> DB::getUserRoles(const std::string &user_id)
{
    std::vector<std::string> roles;
    if (!conn_) return roles;

    std::string query = R"(
        SELECT r.role_name
        FROM roles r
        INNER JOIN user_roles ur ON r.role_id = ur.role_id
        WHERE ur.user_id = $1;
    )";

    const char* paramValues[1];
    paramValues[0] = user_id.c_str();

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

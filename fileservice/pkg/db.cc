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

bool DB::moveFile(const std::string& user_id, int file_id, int target_folder_id)
{
    if (!conn_) return false;

    // Check if the file exists and belongs to the user
    std::string checkFileQuery = R"(
        SELECT 1 FROM files WHERE file_id = $1 AND user_id = $2;
    )";
    const char* fileParamValues[2];
    std::string fileIdStr = std::to_string(file_id);
    fileParamValues[0] = fileIdStr.c_str();
    fileParamValues[1] = user_id.c_str();

    PGresult* fileRes = PQexecParams(conn_, checkFileQuery.c_str(), 2, nullptr, fileParamValues, nullptr, nullptr, 0);
    if (PQresultStatus(fileRes) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to check file: " << PQerrorMessage(conn_) << std::endl;
        PQclear(fileRes);
        return false;
    }

    if (PQntuples(fileRes) == 0)
    {
        std::cerr << "File with file_id " << file_id << " does not exist for user " << user_id << std::endl;
        PQclear(fileRes);
        return false;
    }
    PQclear(fileRes);

    // If target_folder_id > 0, check if the target folder exists and belongs to the user
    if (target_folder_id > 0)
    {
        std::string checkFolderQuery = R"(
            SELECT 1 FROM folders WHERE folder_id = $1 AND user_id = $2;
        )";
        const char* folderParamValues[2];
        std::string folderIdStr = std::to_string(target_folder_id);
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
            std::cerr << "Folder with folder_id " << target_folder_id << " does not exist for user " << user_id << std::endl;
            PQclear(folderRes);
            return false;
        }
        PQclear(folderRes);
    }

    // Update the file's folder_id
    // If target_folder_id == 0, set folder_id to NULL (root directory)
    std::string updateQuery = R"(
        UPDATE files
        SET folder_id = CASE WHEN $2::int = 0 THEN NULL ELSE $2::int END
        WHERE file_id = $1 AND user_id = $3;
    )";

    const char* updateParamValues[3];
    updateParamValues[0] = fileIdStr.c_str();
    std::string targetFolderIdStr = std::to_string(target_folder_id);
    updateParamValues[1] = targetFolderIdStr.c_str();
    updateParamValues[2] = user_id.c_str();

    PGresult* updateRes = PQexecParams(conn_, updateQuery.c_str(), 3, nullptr, updateParamValues, nullptr, nullptr, 0);
    if (PQresultStatus(updateRes) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to move file: " << PQerrorMessage(conn_) << std::endl;
        PQclear(updateRes);
        return false;
    }

    PQclear(updateRes);
    return true;
}

bool DB::moveFiles(const std::string& user_id, const std::vector<int>& file_ids, int target_folder_id)
{
    if (!conn_) return false;

    // Start a transaction
    PGresult* transRes = PQexec(conn_, "BEGIN");
    if (PQresultStatus(transRes) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to start transaction: " << PQerrorMessage(conn_) << std::endl;
        PQclear(transRes);
        return false;
    }
    PQclear(transRes);

    // If target_folder_id > 0, check if the target folder exists and belongs to the user
    if (target_folder_id > 0)
    {
        std::string checkFolderQuery = R"(
            SELECT 1 FROM folders WHERE folder_id = $1 AND user_id = $2;
        )";
        const char* folderParamValues[2];
        std::string folderIdStr = std::to_string(target_folder_id);
        folderParamValues[0] = folderIdStr.c_str();
        folderParamValues[1] = user_id.c_str();

        PGresult* folderRes = PQexecParams(conn_, checkFolderQuery.c_str(), 2, nullptr, folderParamValues, nullptr, nullptr, 0);
        if (PQresultStatus(folderRes) != PGRES_TUPLES_OK)
        {
            std::cerr << "Failed to check folder: " << PQerrorMessage(conn_) << std::endl;
            PQclear(folderRes);
            PQexec(conn_, "ROLLBACK");
            return false;
        }

        if (PQntuples(folderRes) == 0)
        {
            std::cerr << "Folder with folder_id " << target_folder_id << " does not exist for user " << user_id << std::endl;
            PQclear(folderRes);
            PQexec(conn_, "ROLLBACK");
            return false;
        }
        PQclear(folderRes);
    }

    // Check if all files exist and belong to the user
    for (int file_id : file_ids)
    {
        std::string checkFileQuery = R"(
            SELECT 1 FROM files WHERE file_id = $1 AND user_id = $2;
        )";
        const char* fileParamValues[2];
        std::string fileIdStr = std::to_string(file_id);
        fileParamValues[0] = fileIdStr.c_str();
        fileParamValues[1] = user_id.c_str();

        PGresult* fileRes = PQexecParams(conn_, checkFileQuery.c_str(), 2, nullptr, fileParamValues, nullptr, nullptr, 0);
        if (PQresultStatus(fileRes) != PGRES_TUPLES_OK)
        {
            std::cerr << "Failed to check file: " << PQerrorMessage(conn_) << std::endl;
            PQclear(fileRes);
            PQexec(conn_, "ROLLBACK");
            return false;
        }

        if (PQntuples(fileRes) == 0)
        {
            std::cerr << "File with file_id " << file_id << " does not exist for user " << user_id << std::endl;
            PQclear(fileRes);
            PQexec(conn_, "ROLLBACK");
            return false;
        }
        PQclear(fileRes);
    }

    // Build a query to update all files at once with a single SQL statement
    std::string fileIdsList;
    for (size_t i = 0; i < file_ids.size(); ++i)
    {
        if (i > 0) fileIdsList += ",";
        fileIdsList += std::to_string(file_ids[i]);
    }

    std::string updateQuery = R"(
        UPDATE files
        SET folder_id = CASE WHEN $1::int = 0 THEN NULL ELSE $1::int END
        WHERE file_id IN ()" + fileIdsList + R"() AND user_id = $2;
    )";

    const char* updateParamValues[2];
    std::string targetFolderIdStr = std::to_string(target_folder_id);
    updateParamValues[0] = targetFolderIdStr.c_str();
    updateParamValues[1] = user_id.c_str();

    PGresult* updateRes = PQexecParams(conn_, updateQuery.c_str(), 2, nullptr, updateParamValues, nullptr, nullptr, 0);
    if (PQresultStatus(updateRes) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to move files: " << PQerrorMessage(conn_) << std::endl;
        PQclear(updateRes);
        PQexec(conn_, "ROLLBACK");
        return false;
    }
    PQclear(updateRes);

    // Commit the transaction
    PGresult* commitRes = PQexec(conn_, "COMMIT");
    if (PQresultStatus(commitRes) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to commit transaction: " << PQerrorMessage(conn_) << std::endl;
        PQclear(commitRes);
        return false;
    }
    PQclear(commitRes);

    return true;
}

// Get all users with their IDs and emails
std::vector<std::tuple<int, std::string, std::string>> DB::getAllUsers()
{
    std::vector<std::tuple<int, std::string, std::string>> users;
    if (!conn_) return users;

    std::string query = R"(
        SELECT user_id, email, created_at
        FROM users
        ORDER BY user_id;
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get all users: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return users;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int user_id = std::stoi(PQgetvalue(res, i, 0));
        std::string email = PQgetvalue(res, i, 1);
        std::string created_at = PQgetvalue(res, i, 2);

        users.emplace_back(user_id, email, created_at);
    }

    PQclear(res);
    return users;
}

// Get all files from all users
std::vector<std::tuple<int, std::string, int, std::string, int, std::string>> DB::getAllFilesAdmin()
{
    std::vector<std::tuple<int, std::string, int, std::string, int, std::string>> files;
    if (!conn_) return files;

    // Query to get all files with user information
    std::string query = R"(
        SELECT f.file_id, f.file_name, f.file_size, COALESCE(f.folder_id, 0) as folder_id,
               f.user_id, u.email, f.created_at
        FROM files f
        JOIN users u ON f.user_id = u.user_id
        ORDER BY f.user_id, f.file_id;
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get all files: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return files;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int file_id = std::stoi(PQgetvalue(res, i, 0));
        std::string file_name = PQgetvalue(res, i, 1);
        int file_size = std::stoi(PQgetvalue(res, i, 2));
        int folder_id = std::stoi(PQgetvalue(res, i, 3));
        int user_id = std::stoi(PQgetvalue(res, i, 4));
        std::string email = PQgetvalue(res, i, 5);
        std::string created_at = PQgetvalue(res, i, 6);

        files.emplace_back(file_id, file_name, file_size, created_at, user_id, email);
    }

    PQclear(res);
    return files;
}

// Get all folders from all users
std::vector<std::tuple<int, std::string, int, std::string, int, std::string>> DB::getAllFoldersAdmin()
{
    std::vector<std::tuple<int, std::string, int, std::string, int, std::string>> folders;
    if (!conn_) return folders;

    // Query to get all folders with user information
    std::string query = R"(
        SELECT f.folder_id, f.folder_name, COALESCE(f.parent_folder_id, 0) as parent_folder_id,
               f.user_id, u.email, f.created_at
        FROM folders f
        JOIN users u ON f.user_id = u.user_id
        ORDER BY f.user_id, f.folder_id;
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get all folders: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return folders;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int folder_id = std::stoi(PQgetvalue(res, i, 0));
        std::string folder_name = PQgetvalue(res, i, 1);

        bool is_parent_null = PQgetisnull(res, i, 2);
        int parent_folder_id = is_parent_null ? 0 : std::stoi(PQgetvalue(res, i, 2));

        int user_id = std::stoi(PQgetvalue(res, i, 3));
        std::string email = PQgetvalue(res, i, 4);
        std::string created_at = PQgetvalue(res, i, 5);

        folders.emplace_back(folder_id, folder_name, parent_folder_id, created_at, user_id, email);
    }

    PQclear(res);
    return folders;
}

// Get files for a specific user (admin view)
std::vector<std::tuple<int, std::string, int, std::string>> DB::getFilesForUser(const std::string& user_id)
{
    std::vector<std::tuple<int, std::string, int, std::string>> files;
    if (!conn_) return files;

    // Query to get all files for the specified user
    std::string query = R"(
        SELECT file_id, file_name, file_size, COALESCE(folder_id, 0) as folder_id, created_at
        FROM files
        WHERE user_id = $1
        ORDER BY file_id;
    )";

    const char* paramValues[1] = { user_id.c_str() };

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get files for user " << user_id << ": " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return files;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int file_id = std::stoi(PQgetvalue(res, i, 0));
        std::string file_name = PQgetvalue(res, i, 1);
        int file_size = std::stoi(PQgetvalue(res, i, 2));
        int folder_id = std::stoi(PQgetvalue(res, i, 3));
        std::string created_at = PQgetvalue(res, i, 4);

        files.emplace_back(file_id, file_name, file_size, created_at);
    }

    PQclear(res);
    return files;
}

// Get folders for a specific user (admin view)
std::vector<std::tuple<int, std::string, int, std::string>> DB::getFoldersForUser(const std::string& user_id)
{
    std::vector<std::tuple<int, std::string, int, std::string>> folders;
    if (!conn_) return folders;

    // Query to get all folders for the specified user
    std::string query = R"(
        SELECT folder_id, folder_name, COALESCE(parent_folder_id, 0) as parent_folder_id, created_at
        FROM folders
        WHERE user_id = $1
        ORDER BY folder_id;
    )";

    const char* paramValues[1] = { user_id.c_str() };

    PGresult* res = PQexecParams(conn_, query.c_str(), 1, nullptr, paramValues, nullptr, nullptr, 0);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get folders for user " << user_id << ": " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return folders;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        int folder_id = std::stoi(PQgetvalue(res, i, 0));
        std::string folder_name = PQgetvalue(res, i, 1);

        bool is_parent_null = PQgetisnull(res, i, 2);
        int parent_folder_id = is_parent_null ? 0 : std::stoi(PQgetvalue(res, i, 2));

        std::string created_at = PQgetvalue(res, i, 3);

        folders.emplace_back(folder_id, folder_name, parent_folder_id, created_at);
    }

    PQclear(res);
    return folders;
}

// Get system statistics: total_users, total_files, total_folders, total_storage_bytes
std::tuple<int, int, int, long long> DB::getSystemStats()
{
    int total_users = 0;
    int total_files = 0;
    int total_folders = 0;
    long long total_storage_bytes = 0;

    if (!conn_) return std::make_tuple(0, 0, 0, 0);

    // Get total users count
    std::string userCountQuery = "SELECT COUNT(*) FROM users;";
    PGresult* userCountRes = PQexec(conn_, userCountQuery.c_str());
    if (PQresultStatus(userCountRes) == PGRES_TUPLES_OK && PQntuples(userCountRes) > 0) {
        total_users = std::stoi(PQgetvalue(userCountRes, 0, 0));
    }
    PQclear(userCountRes);

    // Get total files count and storage size
    std::string fileStatsQuery = "SELECT COUNT(*), COALESCE(SUM(file_size), 0) FROM files;";
    PGresult* fileStatsRes = PQexec(conn_, fileStatsQuery.c_str());
    if (PQresultStatus(fileStatsRes) == PGRES_TUPLES_OK && PQntuples(fileStatsRes) > 0) {
        total_files = std::stoi(PQgetvalue(fileStatsRes, 0, 0));
        total_storage_bytes = std::stoll(PQgetvalue(fileStatsRes, 0, 1));
    }
    PQclear(fileStatsRes);

    // Get total folders count
    std::string folderCountQuery = "SELECT COUNT(*) FROM folders;";
    PGresult* folderCountRes = PQexec(conn_, folderCountQuery.c_str());
    if (PQresultStatus(folderCountRes) == PGRES_TUPLES_OK && PQntuples(folderCountRes) > 0) {
        total_folders = std::stoi(PQgetvalue(folderCountRes, 0, 0));
    }
    PQclear(folderCountRes);

    return std::make_tuple(total_users, total_files, total_folders, total_storage_bytes);
}

// Get file type distribution (for admin dashboard)
std::vector<std::pair<std::string, int>> DB::getFileTypeDistribution()
{
    std::vector<std::pair<std::string, int>> distribution;
    if (!conn_) return distribution;

    // Query to get file extension distribution
    std::string query = R"(
        SELECT
            CASE
                WHEN RIGHT(file_name, POSITION('.' IN REVERSE(file_name))) = '' THEN 'no_extension'
                ELSE LOWER(RIGHT(file_name, POSITION('.' IN REVERSE(file_name)) - 1))
            END AS extension,
            COUNT(*) as count
        FROM files
        GROUP BY extension
        ORDER BY count DESC
        LIMIT 10;
    )";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get file type distribution: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return distribution;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        std::string extension = PQgetvalue(res, i, 0);
        int count = std::stoi(PQgetvalue(res, i, 1));
        distribution.emplace_back(extension, count);
    }

    PQclear(res);
    return distribution;
}

// Get top users by storage usage
std::vector<std::tuple<std::string, std::string, long long>> DB::getTopUsersByStorage(int limit)
{
    std::vector<std::tuple<std::string, std::string, long long>> topUsers;
    if (!conn_) return topUsers;

    // Build query with the requested limit
    std::string query = R"(
        SELECT
            u.user_id,
            u.email,
            COALESCE(SUM(f.file_size), 0) as total_storage
        FROM
            users u
        LEFT JOIN
            files f ON u.user_id = f.user_id
        GROUP BY
            u.user_id, u.email
        ORDER BY
            total_storage DESC
        LIMIT
    )" + std::to_string(limit) + ";";

    PGresult* res = PQexec(conn_, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::cerr << "Failed to get top users by storage: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return topUsers;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i)
    {
        std::string userId = PQgetvalue(res, i, 0);
        std::string email = PQgetvalue(res, i, 1);
        long long storageBytes = std::stoll(PQgetvalue(res, i, 2));

        topUsers.emplace_back(userId, email, storageBytes);
    }

    PQclear(res);
    return topUsers;
}
#include "user_db.h"
#include <iostream>
#include <utility>

DB::DB(std::string host, std::string port, std::string dbname, std::string user, std::string password)
        : host_(std::move(host)), port_(std::move(port)), dbname_(std::move(dbname)), user_(std::move(user)), password_(std::move(password)), conn_(nullptr)
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
        std::string connInfo = "host=" + host_ + " port=" + port_ + " dbname=" + dbname_ + " user=" + user_ + " password=" + password_;
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

    std::string query = R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id SERIAL PRIMARY KEY,
            email VARCHAR(100) NOT NULL UNIQUE,
            password_hash VARCHAR(255) NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            last_login TIMESTAMP,
            is_active BOOLEAN DEFAULT TRUE
        );
    )";

    PGresult *res = PQexec(conn_, query.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::cerr << "Failed to create table: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return false;
    }

    PQclear(res);
    return true;
}

#include <tuple>

std::tuple<std::string, std::string, UserFetchStatus> DB::getPasswordHashByLogin(const std::string& login)
{
    if (!conn_) return {"", "", UserFetchStatus::QueryFailed};

    // Запрашиваем id и password_hash
    std::string query = "SELECT user_id, password_hash FROM users WHERE email = '" + login + "';";
    PGresult *res = PQexec(conn_, query.c_str());

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

    std::string query = "INSERT INTO users (email, password_hash) VALUES ('" + login + "', '" + password_hash + "');";
    PGresult *res = PQexec(conn_, query.c_str());

    if (PQresultStatus(res) == PGRES_FATAL_ERROR)
    {
        // Проверяем, что ошибка вызвана дубликатом
        std::string sqlState = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        if (sqlState == "23505")
        { // 23505 - код ошибки дубликата
            PQclear(res);
            return CreateUserStatus::UserAlreadyExists;
        }

        std::cerr << "Insert failed: " << PQerrorMessage(conn_) << std::endl;
        PQclear(res);
        return CreateUserStatus::QueryFailed;
    }

    PQclear(res);
    return CreateUserStatus::Success;
}

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

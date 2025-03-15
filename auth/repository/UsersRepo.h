#ifndef AUTH_USER_DB_H
#define AUTH_USER_DB_H

#include <string>
#include <utility>
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
    std::vector<std::string> getUserRoles(const std::string& userId);

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
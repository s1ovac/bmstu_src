#include "service.h"
#include "bcrypt/BCrypt.hpp"
#include "user_db.h"
#include "vector"

AuthService::AuthService(DB& db) : db_(db) {}

std::pair<std::string, bool> AuthService::login(const std::string& login, const std::string& password)
{
    auto [userId, stored_hash, status] = db_.getPasswordHashByLogin(login);

    if (status == UserFetchStatus::UserNotFound)
    {
        throw std::runtime_error("User not found");
    }

    if (stored_hash.empty())
    {
        return {"", false};
    }

    bool password_valid = BCrypt::validatePassword(password, stored_hash);

    if (!password_valid)
    {
        return {"", false};
    }

    return {userId, true};
}


bool AuthService::signup(const std::string& login, const std::string& password)
{
    std::string password_hash = BCrypt::generateHash(password);
    CreateUserStatus status = db_.createUser(login, password_hash);

    if (status == CreateUserStatus::UserAlreadyExists)
    {
        throw std::runtime_error("User already exists");
    }

    return status == CreateUserStatus::Success;
}

std::vector<std::string> AuthService::getUserRoles(const std::string& userId)
{
    return db_.getUserRoles(userId);
}
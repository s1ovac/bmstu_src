#include "AuthController.h"
#include <drogon/drogon.h>

AuthController::AuthController()
{
    authService_ = AuthService::instance();
}

void AuthController::login(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    LOG_INFO << "Login attempt received";
    auto json = req->getJsonObject();

    if (!json || !json->isMember("login") || !json->isMember("password"))
    {
        LOG_WARN << "Invalid input received for login";
        Json::Value jsonResponse;
        jsonResponse["error"] = "Invalid input";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto login = (*json)["login"].asString();
    auto password = (*json)["password"].asString();

    try
    {
        LOG_INFO << "Authenticating user: " << login;

        auto [userId, isAuthenticated] = authService_->login(login, password);
        if (isAuthenticated)
        {
            auto token = JwtUtils::generateToken(userId);

            LOG_INFO << "Login successful for user: " << login;
            Json::Value jsonResponse;
            jsonResponse["message"] = "Login successful";
            jsonResponse["token"] = token;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            callback(resp);
        }
        else
        {
            LOG_WARN << "Invalid credentials for user: " << login;
            Json::Value jsonResponse;
            jsonResponse["error"] = "Invalid credentials";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
        }
    }
    catch(const std::runtime_error& e)
    {
        if (std::string(e.what()) == "User not found")
        {
            LOG_WARN << "User not found: " << login;
            Json::Value jsonResponse;
            jsonResponse["error"] = "User not found";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            resp->setStatusCode(drogon::k404NotFound);
            callback(resp);
        }
        else
        {
            LOG_ERROR << "Internal server error: " << e.what();
            Json::Value jsonResponse;
            jsonResponse["error"] = "Internal server error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    }
}

void AuthController::signup(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    LOG_INFO << "Signup attempt received";
    auto json = req->getJsonObject();

    if (!json || !json->isMember("login") || !json->isMember("password"))
    {
        LOG_WARN << "Invalid input received for signup";
        Json::Value jsonResponse;
        jsonResponse["error"] = "Invalid input";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto login = (*json)["login"].asString();
    auto password = (*json)["password"].asString();

    LOG_INFO << "Signing up user: " << login;
    try {
        if (authService_->signup(login, password))
        {
            LOG_INFO << "Signup successful for user: " << login;
            Json::Value jsonResponse;
            jsonResponse["message"] = "Signup successful";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            callback(resp);
        }
    }
    catch (const std::runtime_error& e)
    {
        if (std::string(e.what()) == "User already exists")
        {
            LOG_WARN << "Signup failed, user already exists: " << login;
            Json::Value jsonResponse;
            jsonResponse["error"] = "User already exists";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            resp->setStatusCode(drogon::k409Conflict);
            callback(resp);
        }
        else
        {
            LOG_ERROR << "Signup failed with error: " << e.what();
            Json::Value jsonResponse;
            jsonResponse["error"] = "Internal server error";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    }
}

void AuthController::changePassword(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    LOG_INFO << "Password change attempt received";

    std::string userId = req->attributes()->get<std::string>("user_id");

    auto json = req->getJsonObject();

    if (!json || !json->isMember("currentPassword") || !json->isMember("newPassword"))
    {
        LOG_WARN << "Invalid input received for password change";
        Json::Value jsonResponse;
        jsonResponse["error"] = "Invalid input. Required fields: currentPassword, newPassword";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto currentPassword = (*json)["currentPassword"].asString();
    auto newPassword = (*json)["newPassword"].asString();

    if (currentPassword.empty() || newPassword.empty())
    {
        LOG_WARN << "Empty passwords received for change";
        Json::Value jsonResponse;
        jsonResponse["error"] = "Passwords cannot be empty";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    if (newPassword.length() < 6)
    {
        LOG_WARN << "New password too short (min 6 characters)";
        Json::Value jsonResponse;
        jsonResponse["error"] = "New password is too short. Minimum 6 characters required";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    try
    {
        LOG_INFO << "Changing password for user: " << userId;

        bool success = authService_->changePassword(userId, currentPassword, newPassword);

        if (success)
        {
            LOG_INFO << "Password change successful for user: " << userId;
            Json::Value jsonResponse;
            jsonResponse["message"] = "Password changed successfully";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            callback(resp);
        }
        else
        {
            LOG_WARN << "Invalid current password for user: " << userId;
            Json::Value jsonResponse;
            jsonResponse["error"] = "Invalid current password";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
            resp->setStatusCode(drogon::k401Unauthorized);
            callback(resp);
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "Password change failed with error: " << e.what();
        Json::Value jsonResponse;
        jsonResponse["error"] = "Internal server error";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(jsonResponse);
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    }
}

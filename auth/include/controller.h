#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <drogon/HttpController.h>
#include <drogon/drogon.h>
#include "service.h"

class AuthController{
public:
    explicit AuthController(AuthService& authService) : authService_(authService) {}

    void login(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void signup(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    static void registerRoutes(drogon::HttpAppFramework &app, const std::shared_ptr<AuthController>& controller)
    {
        app.registerHandler("/api/v1/login",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                                         {
                                controller->login(req, std::move(callback));
                            },
                            {drogon::Post});

        app.registerHandler("/api/v1/signup",
                            [controller](const drogon::HttpRequestPtr& req,
                                         std::function<void(const drogon::HttpResponsePtr&)>&& callback)
                                         {
                                controller->signup(req, std::move(callback));
                            },
                            {drogon::Post});
    }

private:
    AuthService& authService_;
};

#endif //AUTH_CONTROLLER_H

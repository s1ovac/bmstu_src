#ifndef AUTH_CONTROLLER_H
#define AUTH_CONTROLLER_H

#include <drogon/HttpController.h>
#include "services/AuthService.h"
#include "utils/JWT.h"

using namespace drogon;

class AuthController  : public drogon::HttpController<AuthController>{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::login, "/api/v1/login", Post);
        ADD_METHOD_TO(AuthController::signup, "/api/v1/login", Post);
    METHOD_LIST_END

    AuthController();

    void login(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    void signup(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
private:
    std::shared_ptr<AuthService> authService_;

};

#endif //AUTH_CONTROLLER_H

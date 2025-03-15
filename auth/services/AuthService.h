#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <string>
#include "repository/DB.h"

class AuthService {
public:
    static std::shared_ptr<AuthService> instance();

    std::pair<std::string, bool> login(const std::string& login, const std::string& password);
    bool signup(const std::string& login, const std::string& password);
    std::vector<std::string> getUserRoles(const std::string& userId);

private:
    AuthService();
    std::shared_ptr<DB> db_;
};

#endif // AUTH_SERVICE_H

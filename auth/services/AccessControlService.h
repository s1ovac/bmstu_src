#pragma once

#include <string>
#include <memory>
#include "../repository/UsersRepo.h"

class AccessControlService
{
public:
    static std::shared_ptr<AccessControlService> instance();
    explicit AccessControlService(DB& db);

    bool hasPermission(const std::string &user_id, const std::string &permission_name);

private:
    DB& db_;
};
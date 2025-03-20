#pragma once

#include <string>
#include <memory>
#include "repository/DB.h"

class AccessControlService
{
public:
    static std::shared_ptr<AccessControlService> instance();

    bool hasPermission(const std::string &user_id, const std::string &permission_name);

    std::vector<std::string> getUserPermissions(const std::string &user_id);
private:
    AccessControlService();
    std::shared_ptr<DB> db_;
};
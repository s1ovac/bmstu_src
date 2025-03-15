#pragma once

#include <string>
#include <memory>
#include "iostream"
#include "db.h"
#include <drogon/HttpRequest.h>

class RoleService
{
public:
    static std::shared_ptr<RoleService> instance();

    bool createRole(const std::string &role_name, const std::string &description);
    bool deleteRole(int role_id);
    bool assignPermissionsToRole(int role_id, const std::vector<int> &permission_ids);
    bool assignRolesToUser(int user_id, const std::vector<int> &role_ids);
    std::vector<std::pair<int, std::string>> getUserRoles(int user_id);
    std::vector<std::pair<int, std::string>> getAllRoles();

private:
    RoleService();
    std::shared_ptr<DB> db_;
};

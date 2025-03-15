#include "RoleService.h"

std::shared_ptr<RoleService> RoleService::instance()
{
    static std::shared_ptr<RoleService> instance(new RoleService());
    return instance;
}

RoleService::RoleService()
{
    db_ = DB::instance();

    if (!db_)
    {
        throw std::runtime_error("Database instance is not initialized");
    }
}

bool RoleService::createRole(const std::string &role_name, const std::string &description)
{
    return db_.createRole(role_name, description);
}

bool RoleService::deleteRole(int role_id)
{
    return db_.deleteRole(role_id);
}

bool RoleService::assignPermissionsToRole(int role_id, const std::vector<int> &permission_ids)
{
    return db_.assignPermissionsToRole(role_id, permission_ids);
}

bool RoleService::assignRolesToUser(int user_id, const std::vector<int> &role_ids)
{
    return db_.assignRolesToUser(user_id, role_ids);
}

std::vector<std::pair<int, std::string>> RoleService::getUserRoles(int user_id)
{
    return db_.getUserRolesWithIds(user_id);
}

std::vector<std::pair<int, std::string>> RoleService::getAllRoles()
{
    return db_.getAllRoles();
}
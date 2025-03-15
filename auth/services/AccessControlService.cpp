#include "AccessControlService.h"

std::shared_ptr<AccessControlService> AccessControlService::instance()
{
    static std::shared_ptr<AccessControlService> instance(new AccessControlService());
    return instance;
}

AccessControlService::AccessControlService()
{
    db_ = DB::instance();

    if (!db_)
    {
        throw std::runtime_error("Database instance is not initialized");
    }
}

bool AccessControlService::hasPermission(const std::string &user_id, const std::string &permission_name)
{
    auto roles = db_->getUserRoles(user_id);
    for (const auto &role : roles)
    {
        if (db_->roleHasPermission(role, permission_name))
        {
            return true;
        }
    }
    return false;
}
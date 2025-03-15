#include "AccessControlService.h"

std::shared_ptr<AccessControlService> AccessControlService::instance()
{
    static std::shared_ptr<AccessControlService> instance(nullptr);
    if (!instance) {
        throw std::runtime_error("AccessControlService not initialized. Use constructor to create an instance first.");
    }
    return instance;
}

AccessControlService::AccessControlService(DB& db) : db_(db)
{
}

bool AccessControlService::hasPermission(const std::string &user_id, const std::string &permission_name)
{
    auto roles = db_.getUserRoles(user_id);
    for (const auto &role : roles)
    {
        if (db_.roleHasPermission(role, permission_name))
        {
            return true;
        }
    }
    return false;
}
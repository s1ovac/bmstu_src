#include "GroupService.h"
#include "../pkg/db.h"
#include <drogon/drogon.h> // Для LOG_ERROR, если нужно

std::shared_ptr<GroupService> GroupService::instance()
{
    static std::shared_ptr<GroupService> instance(new GroupService());
    return instance;
}

GroupService::GroupService()
{
    auto db = DB::instance();
    if (!db)
    {
        LOG_ERROR << "Database instance is not initialized";
        throw std::runtime_error("Database instance is not initialized");
    }
}

bool GroupService::createGroup(const std::string &group_name)
{
    auto db = DB::instance();
    return db->createGroup(group_name);
}

bool GroupService::deleteGroup(int group_id)
{
    auto db = DB::instance();
    return db->deleteGroup(group_id);
}

bool GroupService::renameGroup(int group_id, const std::string &new_name)
{
    auto db = DB::instance();
    return db->renameGroup(group_id, new_name);
}

bool GroupService::addUserToGroup(int user_id, int group_id)
{
    auto db = DB::instance();
    return db->addUserToGroup(user_id, group_id);
}

bool GroupService::removeUserFromGroup(int user_id, int group_id)
{
    auto db = DB::instance();
    return db->removeUserFromGroup(user_id, group_id);
}

std::vector<std::pair<int, std::string>> GroupService::getUserGroups(int user_id)
{
    auto db = DB::instance();
    return db->getUserGroups(user_id);
}

std::vector<std::pair<int, std::string>> GroupService::getAllGroups()
{
    auto db = DB::instance();
    return db->getAllGroups();
}
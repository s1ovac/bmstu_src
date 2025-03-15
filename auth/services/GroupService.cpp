#include "GroupService.h"

std::shared_ptr<GroupService> GroupService::instance()
{
    static std::shared_ptr<GroupService> instance(new GroupService());
    return instance;
}

GroupService::GroupService()
{
    db_ = DB::instance();

    if (!db_)
    {
        throw std::runtime_error("Database instance is not initialized");
    }
}

bool GroupService::createGroup(const std::string &group_name)
{
    return db_->createGroup(group_name);
}

bool GroupService::deleteGroup(int group_id)
{
    return db_->deleteGroup(group_id);
}

bool GroupService::renameGroup(int group_id, const std::string &new_name)
{
    return db_->renameGroup(group_id, new_name);
}

bool GroupService::addUserToGroup(int user_id, int group_id)
{
    return db_->addUserToGroup(user_id, group_id);
}

bool GroupService::removeUserFromGroup(int user_id, int group_id)
{
    return db_->removeUserFromGroup(user_id, group_id);
}

std::vector<std::pair<int, std::string>> GroupService::getUserGroups(int user_id)
{
    return db_->getUserGroups(user_id);
}

std::vector<std::pair<int, std::string>> GroupService::getAllGroups()
{
    return db_->getAllGroups();
}
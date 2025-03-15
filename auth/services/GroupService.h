#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../repository/UsersRepo.h"

class GroupService
{
public:
    static std::shared_ptr<GroupService> instance();
    explicit GroupService(DB& db);

    // Create a new group
    bool createGroup(const std::string &group_name);

    // Delete a group by ID
    bool deleteGroup(int group_id);

    // Rename a group
    bool renameGroup(int group_id, const std::string &new_name);

    // Add a user to a group
    bool addUserToGroup(int user_id, int group_id);

    // Remove a user from a group
    bool removeUserFromGroup(int user_id, int group_id);

    // Get groups a user belongs to (returns pair of group_id, group_name)
    std::vector<std::pair<int, std::string>> getUserGroups(int user_id);

    // Get all groups
    std::vector<std::pair<int, std::string>> getAllGroups();

private:
    DB& db_;
};
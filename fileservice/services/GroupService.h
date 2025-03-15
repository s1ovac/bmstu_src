#pragma once

#include <memory>
#include <string>
#include <vector>

class GroupService
{
public:
    static std::shared_ptr<GroupService> instance();

    // Создание новой группы
    bool createGroup(const std::string &group_name);

    // Удаление группы по её ID
    bool deleteGroup(int group_id);

    // Переименование группы
    bool renameGroup(int group_id, const std::string &new_name);

    // Добавить пользователя в группу
    bool addUserToGroup(int user_id, int group_id);

    // Удалить пользователя из группы
    bool removeUserFromGroup(int user_id, int group_id);

    // Получить группы, в которых состоит пользователь (возвращаем пару (group_id, group_name))
    std::vector<std::pair<int, std::string>> getUserGroups(int user_id);

    std::vector<std::pair<int, std::string>> getAllGroups();

private:
    GroupService();  // приватный конструктор
};

<script>
    import {onMount} from 'svelte';
    import {userRoles} from '../lib/stores'; // Здесь хранится список ролей текущего пользователя

    // Импортируем нужные методы из api.js
    import {
        getAllUsers,
        getAllGroups,
        createGroup,
        renameGroup,
        deleteGroup,
        addUserToGroup,
        removeUserFromGroup
    } from '../lib/api';

    let errorMessage = '';
    let successMessage = '';

    // Список всех пользователей
    let users = [];
    // Список всех групп
    let groups = [];

    // Токен берём из localStorage
    let token = '';

    // Параметры для работы с выбранными группами и пользователями
    let selectedGroup = null;
    let newGroupName = '';
    let selectedUser = null;
    let groupNameForCreate = '';

    onMount(async () => {
        try {
            const storedUser = JSON.parse(localStorage.getItem("user"));
            token = storedUser?.token || '';

            // Загружаем пользователей
            users = await getAllUsers(token);

            // Загружаем группы
            groups = await getAllGroups(token);

        } catch (err) {
            console.error(err);
            errorMessage = 'Ошибка при загрузке данных: ' + err.message;
        }
    });

    function onSelectGroup(group) {
        selectedGroup = group;
        newGroupName = group.group_name;
        successMessage = '';
        errorMessage = '';
    }

    async function onCreateGroup() {
        try {
            if (!groupNameForCreate) {
                errorMessage = 'Введите название группы';
                return;
            }
            await createGroup(token, groupNameForCreate);
            successMessage = `Группа "${groupNameForCreate}" успешно создана!`;
            errorMessage = '';

            groups = await getAllGroups(token);
            groupNameForCreate = '';
        } catch (err) {
            errorMessage = 'Ошибка при создании группы: ' + err.message;
        }
    }

    async function onRenameGroup() {
        if (!selectedGroup) {
            errorMessage = 'Сначала выберите группу';
            return;
        }
        if (!newGroupName) {
            errorMessage = 'Введите новое название группы';
            return;
        }

        try {
            await renameGroup(token, selectedGroup.group_id, newGroupName);
            successMessage = `Группа переименована в "${newGroupName}"`;
            errorMessage = '';

            groups = await getAllGroups(token);
            selectedGroup = groups.find(g => g.group_id === selectedGroup.group_id);
        } catch (err) {
            errorMessage = 'Ошибка при переименовании группы: ' + err.message;
        }
    }

    async function onDeleteGroup() {
        if (!selectedGroup) {
            errorMessage = 'Сначала выберите группу';
            return;
        }
        try {
            await deleteGroup(token, selectedGroup.group_id);
            successMessage = `Группа "${selectedGroup.group_name}" удалена`;
            errorMessage = '';

            groups = await getAllGroups(token);
            selectedGroup = null;
            newGroupName = '';
        } catch (err) {
            errorMessage = 'Ошибка при удалении группы: ' + err.message;
        }
    }

    function onSelectUser(user) {
        selectedUser = user;
        successMessage = '';
        errorMessage = '';
    }

    async function onAddUserToGroup() {
        if (!selectedGroup || !selectedUser) {
            errorMessage = 'Выберите группу и пользователя';
            return;
        }
        try {
            await addUserToGroup(token, selectedUser.user_id, selectedGroup.group_id);
            successMessage = `Пользователь "${selectedUser.email}" добавлен в группу "${selectedGroup.group_name}"`;
            errorMessage = '';
        } catch (err) {
            errorMessage = 'Ошибка при добавлении пользователя в группу: ' + err.message;
        }
    }

    async function onRemoveUserFromGroup() {
        if (!selectedGroup || !selectedUser) {
            errorMessage = 'Выберите группу и пользователя';
            return;
        }
        try {
            await removeUserFromGroup(token, selectedUser.user_id, selectedGroup.group_id);
            successMessage = `Пользователь "${selectedUser.email}" удалён из группы "${selectedGroup.group_name}"`;
            errorMessage = '';
        } catch (err) {
            errorMessage = 'Ошибка при удалении пользователя из группы: ' + err.message;
        }
    }

    // Метод, который отправляет на домашнюю страницу ("/")
    function goHome() {
        window.location.href = '/';
    }
</script>

<!-- Проверяем, есть ли у пользователя роль admin -->
{#if $userRoles && $userRoles.find(r => r.role_name === 'admin')}
    <div class="admin-container">
        <h1>Административная панель (Управление группами)</h1>

        <!-- Сообщения -->
        {#if errorMessage}
            <div class="alert alert-error">{errorMessage}</div>
        {/if}
        {#if successMessage}
            <div class="alert alert-success">{successMessage}</div>
        {/if}

        <div class="admin-content" style="display: flex; gap: 30px;">
            <!-- Левая колонка: список групп -->
            <div class="groups-panel" style="flex: 1;">
                <h2>Список групп</h2>
                {#if groups.length === 0}
                    <p>Нет групп</p>
                {:else}
                    <table class="groups-table">
                        <thead>
                        <tr>
                            <th>ID</th>
                            <th>Название</th>
                        </tr>
                        </thead>
                        <tbody>
                        {#each groups as group}
                            <tr
                                    class:selectedRow={selectedGroup && selectedGroup.group_id === group.group_id}
                                    on:click={() => onSelectGroup(group)}
                            >
                                <td>{group.group_id}</td>
                                <td>{group.group_name}</td>
                            </tr>
                        {/each}
                        </tbody>
                    </table>
                {/if}

                <!-- Создать новую группу -->
                <div style="margin-top: 1rem;">
                    <h3>Создать новую группу</h3>
                    <input
                            type="text"
                            bind:value={groupNameForCreate}
                            placeholder="Название группы..."
                    />
                    <button on:click={onCreateGroup}>Создать</button>
                </div>
            </div>

            <!-- Средняя колонка: список пользователей -->
            <div class="users-panel" style="flex: 1;">
                <h2>Список пользователей</h2>
                {#if users.length === 0}
                    <p>Нет данных о пользователях</p>
                {:else}
                    <table class="users-table">
                        <thead>
                        <tr>
                            <th>ID</th>
                            <th>Email</th>
                        </tr>
                        </thead>
                        <tbody>
                        {#each users as user}
                            <tr
                                    class:selectedRow={selectedUser && selectedUser.user_id === user.user_id}
                                    on:click={() => onSelectUser(user)}
                            >
                                <td>{user.user_id}</td>
                                <td>{user.email}</td>
                            </tr>
                        {/each}
                        </tbody>
                    </table>
                {/if}
            </div>

            <!-- Правая колонка: действия над группами -->
            <div class="group-actions" style="flex: 1;">
                <h2>Операции с группой</h2>
                <p>Выберите группу и пользователя, чтобы добавить/удалить из группы.</p>

                <div class="form-group">
                    <label>Выбранная группа:</label>
                    <div class="selected-group">
                        {#if selectedGroup}
                            ID={selectedGroup.group_id}, {selectedGroup.group_name}
                        {:else}
                            <em>не выбрана</em>
                        {/if}
                    </div>
                </div>

                <div class="form-group">
                    <label>Выбранный пользователь:</label>
                    <div class="selected-user">
                        {#if selectedUser}
                            ID={selectedUser.user_id}, {selectedUser.email}
                        {:else}
                            <em>не выбран</em>
                        {/if}
                    </div>
                </div>

                <button on:click={onAddUserToGroup}>Добавить пользователя в группу</button>
                <button on:click={onRemoveUserFromGroup}>Удалить пользователя из группы</button>

                <hr/>

                <h3>Переименовать выбранную группу</h3>
                <input type="text" bind:value={newGroupName} placeholder="Новое название..."/>
                <button on:click={onRenameGroup}>Переименовать</button>

                <h3>Удалить группу</h3>
                <button on:click={onDeleteGroup}>Удалить выбранную группу</button>

                <!-- Кнопка для перехода на домашнюю страницу -->
                <hr/>
                <button class="go-home-button" on:click={goHome}>На главную</button>
            </div>
        </div>
    </div>
{:else}
    <div class="no-access">
        <h2>У вас нет прав доступа к этой странице</h2>
        <button on:click={goHome}>Перейти на главную</button>
    </div>
{/if}

<style>
    .admin-container {
        max-width: 1200px;
        margin: 0 auto;
        padding: 20px;
        font-family: Arial, sans-serif;
    }

    .alert {
        padding: 10px 15px;
        margin-bottom: 15px;
        border-radius: 4px;
        font-weight: bold;
    }

    .alert-error {
        background: #f8d7da;
        color: #721c24;
    }

    .alert-success {
        background: #d4edda;
        color: #155724;
    }

    table {
        width: 100%;
        border-collapse: collapse;
    }

    table th, table td {
        padding: 8px 12px;
        border: 1px solid #ccc;
    }

    table th {
        background-color: #e0e0e0;
    }

    tr.selectedRow {
        background-color: #cce5ff;
        cursor: pointer;
    }

    tr:hover {
        background-color: #f1f1f1;
        cursor: pointer;
    }

    .no-access {
        text-align: center;
        margin-top: 50px;
    }

    .form-group {
        margin-bottom: 10px;
    }

    input[type="text"] {
        padding: 6px 8px;
        border: 1px solid #aaa;
        border-radius: 4px;
        font-size: 14px;
        margin-right: 8px;
    }

    button {
        padding: 6px 10px;
        background-color: #4285f4;
        color: white;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        margin-top: 6px;
    }

    button:hover {
        background-color: #3367d6;
    }
</style>

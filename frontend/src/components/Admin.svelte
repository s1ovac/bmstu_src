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
    let loading = false;
    let showDeleteConfirm = false;

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

    // Состояние для отображения сообщений
    let messageTimeout;

    // Переключатель видимости панелей на мобильных устройствах
    let activeMobilePanel = 'groups'; // 'groups', 'users', 'actions'

    onMount(async () => {
        loading = true;
        try {
            const storedUser = JSON.parse(localStorage.getItem("user"));
            token = storedUser?.token || '';

            // Загружаем пользователей и группы параллельно
            const [usersData, groupsData] = await Promise.all([
                getAllUsers(token),
                getAllGroups(token)
            ]);

            users = usersData;
            groups = groupsData;

        } catch (err) {
            console.error(err);
            showError('Ошибка при загрузке данных: ' + err.message);
        } finally {
            loading = false;
        }
    });

    function onSelectGroup(group) {
        selectedGroup = group;
        newGroupName = group.group_name;
        clearMessages();

        // На мобильных устройствах переключаемся на панель действий
        if (window.innerWidth < 768) {
            activeMobilePanel = 'actions';
        }
    }

    function onSelectUser(user) {
        selectedUser = user;
        clearMessages();

        // На мобильных устройствах переключаемся на панель действий
        if (window.innerWidth < 768) {
            activeMobilePanel = 'actions';
        }
    }

    function clearMessages() {
        errorMessage = '';
        successMessage = '';
        if (messageTimeout) {
            clearTimeout(messageTimeout);
        }
    }

    function showSuccess(message) {
        successMessage = message;
        errorMessage = '';

        // Автоматически скрываем сообщение через 5 секунд
        if (messageTimeout) {
            clearTimeout(messageTimeout);
        }
        messageTimeout = setTimeout(() => {
            successMessage = '';
        }, 5000);
    }

    function showError(message) {
        errorMessage = message;
        successMessage = '';

        // Автоматически скрываем сообщение через 5 секунд
        if (messageTimeout) {
            clearTimeout(messageTimeout);
        }
        messageTimeout = setTimeout(() => {
            errorMessage = '';
        }, 5000);
    }

    async function onCreateGroup() {
        if (!groupNameForCreate) {
            showError('Введите название группы');
            return;
        }

        loading = true;
        try {
            await createGroup(token, groupNameForCreate);
            showSuccess(`Группа "${groupNameForCreate}" успешно создана!`);

            groups = await getAllGroups(token);
            groupNameForCreate = '';
        } catch (err) {
            showError('Ошибка при создании группы: ' + err.message);
        } finally {
            loading = false;
        }
    }

    async function onRenameGroup() {
        if (!selectedGroup) {
            showError('Сначала выберите группу');
            return;
        }
        if (!newGroupName) {
            showError('Введите новое название группы');
            return;
        }

        loading = true;
        try {
            await renameGroup(token, selectedGroup.group_id, newGroupName);
            showSuccess(`Группа переименована в "${newGroupName}"`);

            groups = await getAllGroups(token);
            selectedGroup = groups.find(g => g.group_id === selectedGroup.group_id);
        } catch (err) {
            showError('Ошибка при переименовании группы: ' + err.message);
        } finally {
            loading = false;
        }
    }

    function confirmDeleteGroup() {
        if (!selectedGroup) {
            showError('Сначала выберите группу');
            return;
        }
        showDeleteConfirm = true;
    }

    async function onDeleteGroup() {
        if (!selectedGroup) {
            showError('Сначала выберите группу');
            return;
        }

        showDeleteConfirm = false;
        loading = true;
        try {
            await deleteGroup(token, selectedGroup.group_id);
            showSuccess(`Группа "${selectedGroup.group_name}" удалена`);

            groups = await getAllGroups(token);
            selectedGroup = null;
            newGroupName = '';
        } catch (err) {
            showError('Ошибка при удалении группы: ' + err.message);
        } finally {
            loading = false;
        }
    }

    async function onAddUserToGroup() {
        if (!selectedGroup || !selectedUser) {
            showError('Выберите группу и пользователя');
            return;
        }

        loading = true;
        try {
            await addUserToGroup(token, selectedUser.user_id, selectedGroup.group_id);
            showSuccess(`Пользователь "${selectedUser.email}" добавлен в группу "${selectedGroup.group_name}"`);
        } catch (err) {
            showError('Ошибка при добавлении пользователя в группу: ' + err.message);
        } finally {
            loading = false;
        }
    }

    async function onRemoveUserFromGroup() {
        if (!selectedGroup || !selectedUser) {
            showError('Выберите группу и пользователя');
            return;
        }

        loading = true;
        try {
            await removeUserFromGroup(token, selectedUser.user_id, selectedGroup.group_id);
            showSuccess(`Пользователь "${selectedUser.email}" удалён из группы "${selectedGroup.group_name}"`);
        } catch (err) {
            showError('Ошибка при удалении пользователя из группы: ' + err.message);
        } finally {
            loading = false;
        }
    }

    // Метод, который отправляет на домашнюю страницу ("/")
    function goHome() {
        window.location.href = '/';
    }

    function cancelDeleteGroup() {
        showDeleteConfirm = false;
    }

    function setActiveMobilePanel(panel) {
        activeMobilePanel = panel;
    }
</script>

<!-- Проверяем, есть ли у пользователя роль admin -->
{#if $userRoles && $userRoles.find(r => r.role_name === 'admin')}
    <div class="admin-container">
        <header class="admin-header">
            <h1>Панель администратора</h1>
            <button class="home-button" on:click={goHome}>
                <i class="material-icons">home</i>
                <span>На главную</span>
            </button>
        </header>

        <!-- Мобильная навигация -->
        <div class="mobile-tabs">
            <button
                    class:active={activeMobilePanel === 'groups'}
                    on:click={() => setActiveMobilePanel('groups')}
            >
                <i class="material-icons">folder</i>
                <span>Группы</span>
            </button>
            <button
                    class:active={activeMobilePanel === 'users'}
                    on:click={() => setActiveMobilePanel('users')}
            >
                <i class="material-icons">people</i>
                <span>Пользователи</span>
            </button>
            <button
                    class:active={activeMobilePanel === 'actions'}
                    on:click={() => setActiveMobilePanel('actions')}
            >
                <i class="material-icons">settings</i>
                <span>Действия</span>
            </button>
        </div>

        <!-- Сообщения -->
        {#if errorMessage}
            <div class="message error-message">
                <i class="material-icons">error</i>
                <span>{errorMessage}</span>
                <button on:click={() => errorMessage = ''}>
                    <i class="material-icons">close</i>
                </button>
            </div>
        {/if}
        {#if successMessage}
            <div class="message success-message">
                <i class="material-icons">check_circle</i>
                <span>{successMessage}</span>
                <button on:click={() => successMessage = ''}>
                    <i class="material-icons">close</i>
                </button>
            </div>
        {/if}

        <div class="admin-content">
            <!-- Левая колонка: список групп -->
            <div class="panel groups-panel" class:mobile-active={activeMobilePanel === 'groups'}>
                <div class="panel-header">
                    <h2><i class="material-icons">folder</i> Группы</h2>
                </div>

                <div class="panel-content">
                    {#if loading && groups.length === 0}
                        <div class="loading-container">
                            <div class="loader"></div>
                            <span>Загрузка групп...</span>
                        </div>
                    {:else if groups.length === 0}
                        <div class="empty-state">
                            <i class="material-icons">folder_off</i>
                            <p>Нет доступных групп</p>
                        </div>
                    {:else}
                        <div class="table-container">
                            <table class="data-table">
                                <thead>
                                <tr>
                                    <th>ID</th>
                                    <th>Название</th>
                                </tr>
                                </thead>
                                <tbody>
                                {#each groups as group}
                                    <tr
                                            class:selected={selectedGroup && selectedGroup.group_id === group.group_id}
                                            on:click={() => onSelectGroup(group)}
                                    >
                                        <td>{group.group_id}</td>
                                        <td>{group.group_name}</td>
                                    </tr>
                                {/each}
                                </tbody>
                            </table>
                        </div>
                    {/if}

                    <!-- Создать новую группу -->
                    <div class="create-section">
                        <h3>Создать новую группу</h3>
                        <div class="input-group">
                            <input
                                    type="text"
                                    bind:value={groupNameForCreate}
                                    placeholder="Название группы..."
                                    disabled={loading}
                            />
                            <button
                                    on:click={onCreateGroup}
                                    class="primary-button"
                                    disabled={loading || !groupNameForCreate}
                            >
                                <i class="material-icons">add</i>
                                <span>Создать</span>
                            </button>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Средняя колонка: список пользователей -->
            <div class="panel users-panel" class:mobile-active={activeMobilePanel === 'users'}>
                <div class="panel-header">
                    <h2><i class="material-icons">people</i> Пользователи</h2>
                </div>

                <div class="panel-content">
                    {#if loading && users.length === 0}
                        <div class="loading-container">
                            <div class="loader"></div>
                            <span>Загрузка пользователей...</span>
                        </div>
                    {:else if users.length === 0}
                        <div class="empty-state">
                            <i class="material-icons">person_off</i>
                            <p>Нет пользователей</p>
                        </div>
                    {:else}
                        <div class="table-container">
                            <table class="data-table">
                                <thead>
                                <tr>
                                    <th>ID</th>
                                    <th>Email</th>
                                </tr>
                                </thead>
                                <tbody>
                                {#each users as user}
                                    <tr
                                            class:selected={selectedUser && selectedUser.user_id === user.user_id}
                                            on:click={() => onSelectUser(user)}
                                    >
                                        <td>{user.user_id}</td>
                                        <td>{user.email}</td>
                                    </tr>
                                {/each}
                                </tbody>
                            </table>
                        </div>
                    {/if}
                </div>
            </div>

            <!-- Правая колонка: действия над группами -->
            <div class="panel actions-panel" class:mobile-active={activeMobilePanel === 'actions'}>
                <div class="panel-header">
                    <h2><i class="material-icons">settings</i> Управление</h2>
                </div>

                <div class="panel-content">
                    <div class="selection-info">
                        <div class="info-card">
                            <h3>Выбранная группа</h3>
                            {#if selectedGroup}
                                <div class="selected-item">
                                    <div class="item-icon">
                                        <i class="material-icons">folder</i>
                                    </div>
                                    <div class="item-details">
                                        <span class="item-name">{selectedGroup.group_name}</span>
                                        <span class="item-id">ID: {selectedGroup.group_id}</span>
                                    </div>
                                </div>
                            {:else}
                                <p class="not-selected">Группа не выбрана</p>
                            {/if}
                        </div>

                        <div class="info-card">
                            <h3>Выбранный пользователь</h3>
                            {#if selectedUser}
                                <div class="selected-item">
                                    <div class="item-icon">
                                        <i class="material-icons">person</i>
                                    </div>
                                    <div class="item-details">
                                        <span class="item-name">{selectedUser.email}</span>
                                        <span class="item-id">ID: {selectedUser.user_id}</span>
                                    </div>
                                </div>
                            {:else}
                                <p class="not-selected">Пользователь не выбран</p>
                            {/if}
                        </div>
                    </div>

                    <div class="actions-section">
                        <h3>Управление участниками</h3>
                        <div class="action-buttons">
                            <button
                                    on:click={onAddUserToGroup}
                                    class="action-button add-button"
                                    disabled={loading || !selectedGroup || !selectedUser}
                            >
                                <i class="material-icons">person_add</i>
                                <span>Добавить пользователя в группу</span>
                            </button>

                            <button
                                    on:click={onRemoveUserFromGroup}
                                    class="action-button remove-button"
                                    disabled={loading || !selectedGroup || !selectedUser}
                            >
                                <i class="material-icons">person_remove</i>
                                <span>Удалить пользователя из группы</span>
                            </button>
                        </div>
                    </div>

                    <div class="actions-section">
                        <h3>Управление группой</h3>
                        <div class="rename-form">
                            <div class="input-group">
                                <input
                                        type="text"
                                        bind:value={newGroupName}
                                        placeholder="Новое название..."
                                        disabled={loading || !selectedGroup}
                                />
                                <button
                                        on:click={onRenameGroup}
                                        class="primary-button"
                                        disabled={loading || !selectedGroup || !newGroupName}
                                >
                                    <i class="material-icons">edit</i>
                                    <span>Переименовать</span>
                                </button>
                            </div>
                        </div>
                        <div class="delete-section">
                            <button
                                    on:click={confirmDeleteGroup}
                                    class="danger-button"
                                    disabled={loading || !selectedGroup}
                            >
                                <i class="material-icons">delete</i>
                                <span>Удалить группу</span>
                            </button>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <!-- Диалог подтверждения удаления -->
    {#if showDeleteConfirm}
        <div class="modal-overlay">
            <div class="modal-dialog">
                <div class="modal-header">
                    <h3>Подтверждение удаления</h3>
                    <button on:click={cancelDeleteGroup} class="close-button">
                        <i class="material-icons">close</i>
                    </button>
                </div>
                <div class="modal-body">
                    <p>Вы уверены, что хотите удалить группу "{selectedGroup.group_name}"?</p>
                    <p class="warning-text">Это действие нельзя отменить.</p>
                </div>
                <div class="modal-footer">
                    <button on:click={cancelDeleteGroup} class="secondary-button">
                        <i class="material-icons">cancel</i>
                        <span>Отмена</span>
                    </button>
                    <button on:click={onDeleteGroup} class="danger-button">
                        <i class="material-icons">delete_forever</i>
                        <span>Удалить</span>
                    </button>
                </div>
            </div>
        </div>
    {/if}

    <!-- Оверлей загрузки -->
    {#if loading}
        <div class="loading-overlay">
            <div class="loader"></div>
        </div>
    {/if}
{:else}
    <div class="no-access">
        <i class="material-icons">lock</i>
        <h2>У вас нет прав доступа к этой странице</h2>
        <button on:click={goHome} class="primary-button">
            <i class="material-icons">home</i>
            <span>Перейти на главную</span>
        </button>
    </div>
{/if}

<style>
    /* Подключаем Material Icons */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap');

    /* Основные стили */
    .admin-container {
        font-family: 'Roboto', sans-serif;
        max-width: 1400px;
        margin: 0 auto;
        padding: 20px;
        color: #333;
        min-height: 100vh;
        display: flex;
        flex-direction: column;
    }

    .admin-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 20px;
        padding-bottom: 15px;
        border-bottom: 1px solid #e0e0e0;
    }

    .admin-header h1 {
        font-size: 28px;
        font-weight: 500;
        color: #1a73e8;
        margin: 0;
    }

    .admin-content {
        display: flex;
        gap: 20px;
        flex: 1;
    }

    /* Панели */
    .panel {
        flex: 1;
        border-radius: 8px;
        background-color: #fff;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.08);
        display: flex;
        flex-direction: column;
        overflow: hidden;
    }

    .panel-header {
        padding: 16px 20px;
        background-color: #f5f5f5;
        border-bottom: 1px solid #e0e0e0;
    }

    .panel-header h2 {
        margin: 0;
        font-size: 18px;
        font-weight: 500;
        display: flex;
        align-items: center;
        color: #333;
    }

    .panel-header h2 i {
        margin-right: 8px;
        color: #1a73e8;
    }

    .panel-content {
        padding: 20px;
        flex: 1;
        overflow-y: auto;
        display: flex;
        flex-direction: column;
    }

    /* Таблицы */
    .table-container {
        overflow-x: auto;
        margin-bottom: 20px;
        flex: 1;
    }

    .data-table {
        width: 100%;
        border-collapse: collapse;
        font-size: 14px;
    }

    .data-table th {
        background-color: #f5f5f5;
        text-align: left;
        padding: 12px 15px;
        font-weight: 500;
        border-bottom: 2px solid #e0e0e0;
    }

    .data-table td {
        padding: 12px 15px;
        border-bottom: 1px solid #e0e0e0;
    }

    .data-table tr:hover {
        background-color: #f5f8ff;
        cursor: pointer;
    }

    .data-table tr.selected {
        background-color: #e8f0fe;
    }

    /* Формы и инпуты */
    .input-group {
        display: flex;
        margin-bottom: 15px;
    }

    input[type="text"] {
        flex: 1;
        padding: 10px 12px;
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        font-size: 14px;
        transition: border-color 0.3s;
    }

    input[type="text"]:focus {
        border-color: #1a73e8;
        outline: none;
        box-shadow: 0 0 0 2px rgba(26, 115, 232, 0.2);
    }

    input[type="text"]:disabled {
        background-color: #f5f5f5;
        cursor: not-allowed;
    }

    /* Кнопки */
    button {
        cursor: pointer;
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 14px;
        transition: all 0.2s;
        border: none;
        padding: 0 15px;
        font-family: 'Roboto', sans-serif;
        border-radius: 4px;
    }

    button:disabled {
        opacity: 0.7;
        cursor: not-allowed;
    }

    button i {
        margin-right: 5px;
        font-size: 18px;
    }

    .primary-button {
        background-color: #1a73e8;
        color: white;
        height: 40px;
        margin-left: 8px;
    }

    .primary-button:hover:not(:disabled) {
        background-color: #0d62d6;
        box-shadow: 0 1px 3px rgba(26, 115, 232, 0.3);
    }

    .secondary-button {
        background-color: #f1f3f4;
        color: #3c4043;
        height: 40px;
    }

    .secondary-button:hover:not(:disabled) {
        background-color: #e8eaed;
    }

    .danger-button {
        background-color: #ea4335;
        color: white;
        height: 40px;
    }

    .danger-button:hover:not(:disabled) {
        background-color: #d93025;
        box-shadow: 0 1px 3px rgba(234, 67, 53, 0.3);
    }

    .action-button {
        height: 40px;
        width: 100%;
        margin-bottom: 10px;
        justify-content: flex-start;
        padding: 0 12px;
    }

    .add-button {
        background-color: #1e8e3e;
        color: white;
    }

    .add-button:hover:not(:disabled) {
        background-color: #188038;
        box-shadow: 0 1px 3px rgba(30, 142, 62, 0.3);
    }

    .remove-button {
        background-color: #ea4335;
        color: white;
    }

    .remove-button:hover:not(:disabled) {
        background-color: #d93025;
        box-shadow: 0 1px 3px rgba(234, 67, 53, 0.3);
    }

    .home-button {
        background-color: #f1f3f4;
        color: #3c4043;
        height: 40px;
    }

    .home-button:hover {
        background-color: #e8eaed;
    }

    /* Сообщения */
    .message {
        display: flex;
        align-items: center;
        padding: 12px 20px;
        border-radius: 4px;
        margin-bottom: 20px;
        font-size: 14px;
    }

    .message i {
        margin-right: 10px;
        font-size: 20px;
    }

    .message span {
        flex: 1;
    }

    .message button {
        background: transparent;
        border: none;
        color: inherit;
        cursor: pointer;
        padding: 5px;
    }

    .error-message {
        background-color: #fdeded;
        color: #ea4335;
        border-left: 4px solid #ea4335;
    }

    .success-message {
        background-color: #e6f4ea;
        color: #1e8e3e;
        border-left: 4px solid #1e8e3e;
    }

    /* Модальные окна */
    .modal-overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0, 0, 0, 0.5);
        display: flex;
        justify-content: center;
        align-items: center;
        z-index: 1000;
    }

    .modal-dialog {
        background-color: white;
        border-radius: 8px;
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.15);
        width: 400px;
        max-width: 90%;
        overflow: hidden;
    }

    .modal-header {
        padding: 16px 20px;
        border-bottom: 1px solid #e0e0e0;
        display: flex;
        justify-content: space-between;
        align-items: center;
    }

    .modal-header h3 {
        margin: 0;
        font-size: 18px;
        font-weight: 500;
    }

    .close-button {
        background: transparent;
        border: none;
        color: #5f6368;
        cursor: pointer;
        padding: 5px;
    }

    .close-button i {
        margin: 0;
    }

    .modal-body {
        padding: 20px;
    }

    .modal-footer {
        padding: 16px 20px;
        border-top: 1px solid #e0e0e0;
        display: flex;
        justify-content: flex-end;
        gap: 10px;
    }

    .warning-text {
        color: #ea4335;
        font-weight: 500;
    }

    /* Состояния пустые и загрузка */
    .empty-state {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        padding: 40px 20px;
        text-align: center;
        color: #5f6368;
        flex: 1;
    }

    .empty-state i {
        font-size: 48px;
        margin-bottom: 16px;
        color: #dadce0;
    }

    .empty-state p {
        margin: 0;
        font-size: 16px;
    }

    .loading-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        padding: 40px 20px;
        flex: 1;
    }

    .loading-overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(255, 255, 255, 0.8);
        display: flex;
        justify-content: center;
        align-items: center;
        z-index: 1000;
    }

    .loader {
        border: 3px solid #f3f3f3;
        border-radius: 50%;
        border-top: 3px solid #1a73e8;
        width: 30px;
        height: 30px;
        animation: spin 1s linear infinite;
        margin-bottom: 12px;
    }

    @keyframes spin {
        0% {
            transform: rotate(0deg);
        }
        100% {
            transform: rotate(360deg);
        }
    }

    /* Секции в панелях */
    .create-section, .actions-section {
        margin-top: 20px;
        padding-top: 20px;
        border-top: 1px solid #e0e0e0;
    }

    .actions-section h3, .create-section h3 {
        font-size: 16px;
        font-weight: 500;
        margin-bottom: 12px;
        color: #3c4043;
    }

    .action-buttons {
        display: flex;
        flex-direction: column;
    }

    .delete-section {
        margin-top: 15px;
    }

    /* Информационные карточки */
    .selection-info {
        display: flex;
        flex-direction: column;
        gap: 15px;
        margin-bottom: 20px;
    }

    .info-card {
        background-color: #f8f9fa;
        border-radius: 6px;
        padding: 15px;
        border: 1px solid #e0e0e0;
    }

    .info-card h3 {
        font-size: 14px;
        font-weight: 500;
        margin: 0 0 10px 0;
        color: #5f6368;
    }

    .selected-item {
        display: flex;
        align-items: center;
    }

    .item-icon {
        background-color: #e8f0fe;
        color: #1a73e8;
        width: 40px;
        height: 40px;
        border-radius: 20px;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 12px;
    }

    .item-details {
        display: flex;
        flex-direction: column;
    }

    .item-name {
        font-weight: 500;
    }

    .item-id {
        font-size: 12px;
        color: #5f6368;
    }

    .not-selected {
        color: #5f6368;
        font-style: italic;
        margin: 0;
    }

    /* Отсутствие доступа */
    .no-access {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        min-height: 100vh;
        text-align: center;
        padding: 20px;
    }

    .no-access i {
        font-size: 64px;
        color: #ea4335;
        margin-bottom: 20px;
    }

    .no-access h2 {
        font-size: 24px;
        margin-bottom: 30px;
        color: #3c4043;
    }

    /* Мобильная версия */
    .mobile-tabs {
        display: none;
        background-color: white;
        box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
        border-radius: 8px;
        margin-bottom: 20px;
        overflow: hidden;
    }

    .mobile-tabs button {
        flex: 1;
        padding: 12px 5px;
        border-radius: 0;
        background-color: white;
        color: #5f6368;
        border-bottom: 3px solid transparent;
        flex-direction: column;
    }

    .mobile-tabs button i {
        margin: 0 0 5px 0;
        font-size: 24px;
    }

    .mobile-tabs button.active {
        color: #1a73e8;
        border-bottom-color: #1a73e8;
    }

    /* Адаптивность */
    @media (max-width: 768px) {
        .admin-content {
            flex-direction: column;
        }

        .panel {
            display: none;
        }

        .panel.mobile-active {
            display: flex;
        }

        .mobile-tabs {
            display: flex;
        }

        .admin-header {
            flex-direction: column;
            align-items: flex-start;
            gap: 15px;
        }

        .home-button {
            width: 100%;
        }

        .action-button {
            height: auto;
            padding: 10px 15px;
        }
    }
</style>
<!-- src/components/AdminDashboard.svelte -->
<script>
    import { onMount } from 'svelte';
    import { loggedIn, userRoles } from '../lib/stores.js';
    import { getAllUsers, getSystemStats, getUserContent, getAllFilesAdmin, getAllFoldersAdmin } from '../lib/api.js';

    // Состояние компонента
    let loading = true;
    let errorMessage = '';
    let users = [];
    let statistics = null;
    let selectedUser = null;
    let userContent = null;
    let activeTab = 'users'; // 'users', 'statistics', 'userContent'
    let fileFormat = {};

    // Форматирование размера файла
    const formatFileSize = (size) => {
        if (size < 1024) return `${size} B`;
        else if (size < 1024 * 1024) return `${(size / 1024).toFixed(1)} KB`;
        else if (size < 1024 * 1024 * 1024) return `${(size / (1024 * 1024)).toFixed(1)} MB`;
        else return `${(size / (1024 * 1024 * 1024)).toFixed(1)} GB`;
    };

    // Форматирование даты
    const formatDate = (dateString) => {
        try {
            const date = new Date(dateString);
            return date.toLocaleDateString('ru-RU', {
                day: '2-digit',
                month: '2-digit',
                year: 'numeric',
                hour: '2-digit',
                minute: '2-digit'
            });
        } catch(e) {
            return dateString;
        }
    };

    const getFileIcon = (fileName) => {
        if (!fileName) return 'insert_drive_file';

        const extension = fileName.split('.').pop().toLowerCase();

        // Создаем словарь распространенных расширений файлов
        const iconMap = {
            'pdf': 'picture_as_pdf',
            'doc': 'description',
            'docx': 'description',
            'txt': 'article',
            'xls': 'table_chart',
            'xlsx': 'table_chart',
            'ppt': 'slideshow',
            'pptx': 'slideshow',
            'jpg': 'image',
            'jpeg': 'image',
            'png': 'image',
            'gif': 'gif',
            'mp3': 'audio_file',
            'wav': 'audio_file',
            'mp4': 'video_file',
            'avi': 'video_file',
            'mov': 'video_file',
            'zip': 'folder_zip',
            'rar': 'folder_zip'
        };

        return iconMap[extension] || 'insert_drive_file';
    };

    // Загрузка данных пользователей
    const loadUsersData = async () => {
        try {
            loading = true;
            const storedUser = JSON.parse(localStorage.getItem("user"));
            if (!storedUser || !storedUser.token) {
                throw new Error("Не удалось получить токен авторизации");
            }

            const usersData = await getAllUsers(storedUser.token);
            users = usersData || [];
            loading = false;
        } catch (error) {
            errorMessage = `Ошибка при загрузке данных пользователей: ${error.message}`;
            loading = false;
        }
    };

    // Загрузка статистики системы
    const loadStatistics = async () => {
        try {
            loading = true;
            const storedUser = JSON.parse(localStorage.getItem("user"));
            if (!storedUser || !storedUser.token) {
                throw new Error("Не удалось получить токен авторизации");
            }

            statistics = await getSystemStats(storedUser.token);

            // Формируем объект с частотой форматов файлов
            if (statistics && statistics.files_by_type) {
                fileFormat = statistics.files_by_type.reduce((acc, curr) => {
                    acc[curr.type] = curr.count;
                    return acc;
                }, {});
            }

            loading = false;
        } catch (error) {
            errorMessage = `Ошибка при загрузке статистики: ${error.message}`;
            loading = false;
        }
    };

    // Просмотр содержимого пользователя
    const viewUserContent = async (user) => {
        try {
            loading = true;
            selectedUser = user;
            activeTab = 'userContent';

            const storedUser = JSON.parse(localStorage.getItem("user"));
            if (!storedUser || !storedUser.token) {
                throw new Error("Не удалось получить токен авторизации");
            }

            userContent = await getUserContent(storedUser.token, user.user_id);
            loading = false;
        } catch (error) {
            errorMessage = `Ошибка при загрузке данных пользователя: ${error.message}`;
            loading = false;
        }
    };

    const goToHome = () => {
        window.location.href = '/';
    };

    const goToUsers = () => {
        activeTab = 'users';
        selectedUser = null;
        userContent = null;
    };

    const goToStatistics = () => {
        activeTab = 'statistics';
        selectedUser = null;
        userContent = null;
        loadStatistics();
    };

    onMount(async () => {
        await loadUsersData();
    });
</script>

<!-- Проверяем, есть ли у пользователя роль admin -->
{#if $userRoles && ($userRoles.find(r => r.role_name === 'admin') || $userRoles.find(r => r.role_name === 'manage_files'))}
    <div class="admin-dashboard">
        <header class="dashboard-header">
            <h1>Панель администратора</h1>
            <button class="home-button" on:click={goToHome}>
                <i class="material-icons">home</i>
                <span>На главную</span>
            </button>
        </header>

        <!-- Навигационные вкладки -->
        <div class="dashboard-tabs">
            <button
                    class="tab-button {activeTab === 'users' ? 'active' : ''}"
                    on:click={goToUsers}
            >
                <i class="material-icons">people</i>
                <span>Пользователи</span>
            </button>
            <button
                    class="tab-button {activeTab === 'statistics' ? 'active' : ''}"
                    on:click={goToStatistics}
            >
                <i class="material-icons">bar_chart</i>
                <span>Статистика</span>
            </button>
            {#if selectedUser}
                <button
                        class="tab-button {activeTab === 'userContent' ? 'active' : ''}"
                        on:click={() => activeTab = 'userContent'}
                >
                    <i class="material-icons">folder_shared</i>
                    <span>Файлы {selectedUser.email}</span>
                </button>
            {/if}
        </div>

        {#if errorMessage}
            <div class="error-message">
                <i class="material-icons">error</i>
                <span>{errorMessage}</span>
                <button class="close-button" on:click={() => errorMessage = ''}>
                    <i class="material-icons">close</i>
                </button>
            </div>
        {/if}

        <!-- Основной контент в зависимости от активной вкладки -->
        <div class="dashboard-content">
            {#if loading}
                <div class="loading-container">
                    <div class="spinner"></div>
                    <p>Загрузка данных...</p>
                </div>
            {:else if activeTab === 'users'}
                <!-- Список пользователей -->
                <div class="panel">
                    <div class="panel-header">
                        <h2><i class="material-icons">people</i> Пользователи системы</h2>
                    </div>
                    <div class="panel-content">
                        {#if users.length === 0}
                            <div class="empty-state">
                                <i class="material-icons">person_off</i>
                                <p>Нет пользователей в системе</p>
                            </div>
                        {:else}
                            <div class="table-container">
                                <table class="data-table">
                                    <thead>
                                    <tr>
                                        <th>ID</th>
                                        <th>Email</th>
                                        <th>Роли</th>
                                        <th>Действия</th>
                                    </tr>
                                    </thead>
                                    <tbody>
                                    {#each users as user}
                                        <tr>
                                            <td>{user.user_id}</td>
                                            <td>{user.email}</td>
                                            <td>
                                                {#if user.roles && user.roles.length > 0}
                                                    <div class="roles-list">
                                                        {#each user.roles as role}
                                                            <span class="role-tag">{role}</span>
                                                        {/each}
                                                    </div>
                                                {:else}
                                                    <span class="no-roles">Нет ролей</span>
                                                {/if}
                                            </td>
                                            <td class="actions-cell">
                                                <button
                                                        class="action-button"
                                                        on:click={() => viewUserContent(user)}
                                                        title="Просмотр содержимого"
                                                >
                                                    <i class="material-icons">visibility</i>
                                                    <span>Просмотр файлов</span>
                                                </button>
                                            </td>
                                        </tr>
                                    {/each}
                                    </tbody>
                                </table>
                            </div>
                        {/if}
                    </div>
                </div>
            {:else if activeTab === 'statistics'}
                <!-- Статистика системы -->
                <div class="statistics-container">
                    {#if statistics}
                        <div class="stats-grid">
                            <!-- Основные показатели -->
                            <div class="stats-card">
                                <div class="stats-icon users">
                                    <i class="material-icons">people</i>
                                </div>
                                <div class="stats-info">
                                    <h3>Пользователи</h3>
                                    <div class="stats-value">{statistics.total_users}</div>
                                </div>
                            </div>

                            <div class="stats-card">
                                <div class="stats-icon files">
                                    <i class="material-icons">description</i>
                                </div>
                                <div class="stats-info">
                                    <h3>Файлы</h3>
                                    <div class="stats-value">{statistics.total_files}</div>
                                </div>
                            </div>

                            <div class="stats-card">
                                <div class="stats-icon folders">
                                    <i class="material-icons">folder</i>
                                </div>
                                <div class="stats-info">
                                    <h3>Папки</h3>
                                    <div class="stats-value">{statistics.total_folders}</div>
                                </div>
                            </div>

                            <div class="stats-card">
                                <div class="stats-icon storage">
                                    <i class="material-icons">storage</i>
                                </div>
                                <div class="stats-info">
                                    <h3>Общий объём</h3>
                                    <div class="stats-value">{statistics.total_storage_readable || formatFileSize(statistics.total_storage_bytes)}</div>
                                </div>
                            </div>
                        </div>

                        <!-- Распределение типов файлов -->
                        <div class="panel">
                            <div class="panel-header">
                                <h2><i class="material-icons">pie_chart</i> Распределение типов файлов</h2>
                            </div>
                            <div class="panel-content">
                                {#if statistics.files_by_type && statistics.files_by_type.length > 0}
                                    <div class="file-types-table">
                                        <table class="data-table">
                                            <thead>
                                            <tr>
                                                <th>Тип файла</th>
                                                <th>Количество</th>
                                                <th>Визуализация</th>
                                            </tr>
                                            </thead>
                                            <tbody>
                                            {#each statistics.files_by_type as fileType}
                                                <tr>
                                                    <td>
                                                        <div class="file-type">
                                                            <i class="material-icons">{getFileIcon(`.${fileType.type}`)}</i>
                                                            <span>.{fileType.type}</span>
                                                        </div>
                                                    </td>
                                                    <td>{fileType.count}</td>
                                                    <td>
                                                        <div class="progress-bar-container">
                                                            <div
                                                                    class="progress-bar"
                                                                    style="width: {Math.min(fileType.count / Math.max(...statistics.files_by_type.map(f => f.count)) * 100, 100)}%"
                                                            ></div>
                                                        </div>
                                                    </td>
                                                </tr>
                                            {/each}
                                            </tbody>
                                        </table>
                                    </div>
                                {:else}
                                    <div class="empty-state">
                                        <i class="material-icons">bar_chart</i>
                                        <p>Нет данных о типах файлов</p>
                                    </div>
                                {/if}
                            </div>
                        </div>

                        <!-- Топ пользователей по объему хранилища -->
                        <div class="panel">
                            <div class="panel-header">
                                <h2><i class="material-icons">leaderboard</i> Топ пользователей по использованию хранилища</h2>
                            </div>
                            <div class="panel-content">
                                {#if statistics.top_users_by_storage && statistics.top_users_by_storage.length > 0}
                                    <div class="top-users-table">
                                        <table class="data-table">
                                            <thead>
                                            <tr>
                                                <th>Пользователь</th>
                                                <th>Объем</th>
                                                <th>Визуализация</th>
                                            </tr>
                                            </thead>
                                            <tbody>
                                            {#each statistics.top_users_by_storage as user}
                                                <tr>
                                                    <td>{user[0]}</td>
                                                    <td>{formatFileSize(user[1])}</td>
                                                    <td>
                                                        <div class="progress-bar-container">
                                                            <div
                                                                    class="progress-bar"
                                                                    style="width: {Math.min(user[1] / Math.max(...statistics.top_users_by_storage.map(u => u[1])) * 100, 100)}%"
                                                            ></div>
                                                        </div>
                                                    </td>
                                                </tr>
                                            {/each}
                                            </tbody>
                                        </table>
                                    </div>
                                {:else}
                                    <div class="empty-state">
                                        <i class="material-icons">person_off</i>
                                        <p>Нет данных о пользователях</p>
                                    </div>
                                {/if}
                            </div>
                        </div>
                    {:else}
                        <div class="empty-state">
                            <i class="material-icons">analytics</i>
                            <p>Нет статистических данных</p>
                        </div>
                    {/if}
                </div>
            {:else if activeTab === 'userContent' && selectedUser}
                <!-- Содержимое пользователя (файлы и папки) -->
                <div class="user-content-container">
                    {#if userContent}
                        <div class="user-header">
                            <div class="user-avatar">
                                <i class="material-icons">account_circle</i>
                            </div>
                            <div class="user-details">
                                <h2>{userContent.email}</h2>
                                <p>ID пользователя: {userContent.user_id}</p>
                                <div class="user-stats">
                                    <span><i class="material-icons">folder</i> {userContent.folders_count} папок</span>
                                    <span><i class="material-icons">insert_drive_file</i> {userContent.files_count} файлов</span>
                                    <span><i class="material-icons">sd_storage</i> {userContent.total_storage_readable}</span>
                                </div>
                            </div>
                        </div>

                        <div class="content-tabs">
                            <button class="tab-button active">Все содержимое</button>
                        </div>

                        <div class="panel">
                            <div class="panel-header">
                                <h3><i class="material-icons">folder</i> Папки</h3>
                            </div>
                            <div class="panel-content">
                                {#if userContent.folders && userContent.folders.length > 0}
                                    <div class="folders-grid">
                                        {#each userContent.folders as folder}
                                            <div class="folder-item">
                                                <div class="item-icon">
                                                    <i class="material-icons">folder</i>
                                                </div>
                                                <div class="item-details">
                                                    <div class="item-name" title={folder.folder_name}>{folder.folder_name}</div>
                                                    <div class="item-meta">
                                                        <span>{formatDate(folder.created_at)}</span>
                                                        <span>ID: {folder.folder_id}</span>
                                                        {#if folder.parent_folder_id > 0}
                                                            <span>Родитель: {folder.parent_folder_id}</span>
                                                        {:else}
                                                            <span>Корневая папка</span>
                                                        {/if}
                                                    </div>
                                                </div>
                                            </div>
                                        {/each}
                                    </div>
                                {:else}
                                    <div class="empty-state">
                                        <i class="material-icons">folder_off</i>
                                        <p>У пользователя нет папок</p>
                                    </div>
                                {/if}
                            </div>
                        </div>

                        <div class="panel">
                            <div class="panel-header">
                                <h3><i class="material-icons">insert_drive_file</i> Файлы</h3>
                            </div>
                            <div class="panel-content">
                                {#if userContent.files && userContent.files.length > 0}
                                    <div class="table-container">
                                        <table class="data-table">
                                            <thead>
                                            <tr>
                                                <th>Название</th>
                                                <th>Размер</th>
                                                <th>Папка</th>
                                                <th>Дата создания</th>
                                            </tr>
                                            </thead>
                                            <tbody>
                                            {#each userContent.files as file}
                                                <tr>
                                                    <td>
                                                        <div class="file-name">
                                                            <i class="material-icons">{getFileIcon(file.file_name)}</i>
                                                            {file.file_name}
                                                        </div>
                                                    </td>
                                                    <td>{formatFileSize(file.file_size)}</td>
                                                    <td>
                                                        {#if file.folder_id > 0}
                                                            ID: {file.folder_id}
                                                        {:else}
                                                            Корневая папка
                                                        {/if}
                                                    </td>
                                                    <td>{formatDate(file.created_at)}</td>
                                                </tr>
                                            {/each}
                                            </tbody>
                                        </table>
                                    </div>
                                {:else}
                                    <div class="empty-state">
                                        <i class="material-icons">description_off</i>
                                        <p>У пользователя нет файлов</p>
                                    </div>
                                {/if}
                            </div>
                        </div>
                    {:else}
                        <div class="empty-state">
                            <i class="material-icons">person_off</i>
                            <p>Данные пользователя не найдены</p>
                        </div>
                    {/if}
                </div>
            {/if}
        </div>
    </div>
{:else}
    <div class="no-access">
        <i class="material-icons">lock</i>
        <h2>У вас нет прав доступа к этой странице</h2>
        <p>Для доступа к панели администратора необходимы соответствующие права.</p>
        <button on:click={goToHome} class="primary-button">
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
    .admin-dashboard {
        font-family: 'Roboto', sans-serif;
        max-width: 1400px;
        margin: 0 auto;
        padding: 20px;
        color: var(--text-primary, #333);
    }

    .dashboard-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 20px;
        padding-bottom: 15px;
        border-bottom: 1px solid #e0e0e0;
    }

    .dashboard-header h1 {
        font-size: 28px;
        font-weight: 500;
        color: #1a73e8;
        margin: 0;
    }

    .home-button {
        background-color: #f1f3f4;
        color: #3c4043;
        border: none;
        border-radius: 4px;
        display: flex;
        align-items: center;
        padding: 8px 16px;
        font-size: 14px;
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .home-button:hover {
        background-color: #e8eaed;
    }

    .home-button i {
        margin-right: 8px;
    }

    /* Вкладки */
    .dashboard-tabs {
        display: flex;
        margin-bottom: 20px;
        border-bottom: 1px solid #e0e0e0;
        overflow-x: auto;
        white-space: nowrap;
    }

    .tab-button {
        display: flex;
        align-items: center;
        background: none;
        border: none;
        color: #5f6368;
        padding: 12px 16px;
        font-size: 14px;
        position: relative;
        cursor: pointer;
        transition: color 0.2s;
    }

    .tab-button i {
        margin-right: 8px;
        font-size: 20px;
    }

    .tab-button.active {
        color: #1a73e8;
        font-weight: 500;
    }

    .tab-button.active::after {
        content: '';
        position: absolute;
        bottom: -1px;
        left: 0;
        right: 0;
        height: 3px;
        background-color: #1a73e8;
    }

    /* Панели */
    .panel {
        background-color: #fff;
        border-radius: 8px;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
        margin-bottom: 20px;
        overflow: hidden;
    }

    .panel-header {
        background-color: #f5f5f5;
        padding: 16px 20px;
        border-bottom: 1px solid #e0e0e0;
    }

    .panel-header h2, .panel-header h3 {
        margin: 0;
        font-size: 18px;
        display: flex;
        align-items: center;
        color: #202124;
    }

    .panel-header i {
        margin-right: 8px;
        color: #1a73e8;
    }

    .panel-content {
        padding: 20px;
    }

    /* Таблицы */
    .table-container {
        overflow-x: auto;
    }

    .data-table {
        width: 100%;
        border-collapse: collapse;
        font-size: 14px;
    }

    .data-table th, .data-table td {
        padding: 12px 16px;
        text-align: left;
        border-bottom: 1px solid #e0e0e0;
    }

    .data-table th {
        background-color: #f5f5f5;
        font-weight: 500;
        color: #5f6368;
    }

    .data-table tr:hover {
        background-color: #f8f9fa;
    }

    /* Сетка для статистики */
    .stats-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
        gap: 20px;
        margin-bottom: 20px;
    }

    .stats-card {
        background-color: #fff;
        border-radius: 8px;
        padding: 20px;
        display: flex;
        align-items: center;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .stats-icon {
        width: 56px;
        height: 56px;
        border-radius: 28px;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 16px;
    }

    .stats-icon i {
        font-size: 28px;
        color: white;
    }

    .stats-icon.users {
        background-color: #4285f4;
    }

    .stats-icon.files {
        background-color: #0f9d58;
    }

    .stats-icon.folders {
        background-color: #f4b400;
    }

    .stats-icon.storage {
        background-color: #db4437;
    }

    .stats-info h3 {
        margin: 0 0 8px 0;
        font-size: 14px;
        color: #5f6368;
        font-weight: 500;
    }

    .stats-value {
        font-size: 24px;
        font-weight: 500;
        color: #202124;
    }

    /* Сетка папок */
    .folders-grid {
        display: grid;
        grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
        gap: 16px;
    }

    .folder-item {
        background-color: #fff;
        border-radius: 8px;
        padding: 16px;
        display: flex;
        flex-direction: column;
        align-items: center;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .item-icon {
        width: 48px;
        height: 48px;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-bottom: 12px;
    }

    .item-icon i {
        font-size: 32px;
        color: #f4b400;
    }

    .item-details {
        width: 100%;
        text-align: center;
    }

    .item-name {
        font-weight: 500;
        margin-bottom: 8px;
        word-break: break-word;
    }

    .item-meta {
        font-size: 12px;
        color: #5f6368;
        display: flex;
        flex-direction: column;
        gap: 4px;
    }

    /* Список ролей пользователя */
    .roles-list {
        display: flex;
        flex-wrap: wrap;
        gap: 4px;
    }

    .role-tag {
        background-color: #e8f0fe;
        color: #1a73e8;
        padding: 2px 8px;
        border-radius: 12px;
        font-size: 12px;
        font-weight: 500;
    }

    .no-roles {
        color: #9aa0a6;
        font-style: italic;
    }

    /* Информация о пользователе */
    .user-header {
        display: flex;
        background-color: #fff;
        border-radius: 8px;
        padding: 20px;
        margin-bottom: 20px;
        box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
    }

    .user-avatar {
        width: 64px;
        height: 64px;
        border-radius: 32px;
        background-color: #e8f0fe;
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 20px;
    }

    .user-avatar i {
        font-size: 40px;
        color: #1a73e8;
    }

    .user-details {
        flex: 1;
    }

    .user-details h2 {
        margin: 0 0 4px 0;
        font-size: 20px;
        color: #202124;
    }

    .user-details p {
        margin: 0 0 12px 0;
        font-size: 14px;
        color: #5f6368;
    }

    .user-stats {
        display: flex;
        gap: 16px;
    }

    .user-stats span {
        display: flex;
        align-items: center;
        font-size: 14px;
        color: #3c4043;
    }

    .user-stats i {
        font-size: 18px;
        margin-right: 4px;
        color: #1a73e8;
    }

    /* Стили для прогресс-бара */
    .progress-bar-container {
        width: 100%;
        height: 8px;
        background-color: #f1f3f4;
        border-radius: 4px;
        overflow: hidden;
    }

    .progress-bar {
        height: 100%;
        background-color: #1a73e8;
        border-radius: 4px;
    }

    /* Название файла с иконкой */
    .file-name {
        display: flex;
        align-items: center;
    }

    .file-name i {
        margin-right: 8px;
        font-size: 20px;
        color: #4285f4;
    }

    .file-type {
        display: flex;
        align-items: center;
    }

    .file-type i {
        margin-right: 8px;
        font-size: 20px;
        color: #4285f4;
    }

    /* Кнопки действий */
    .action-button {
        display: flex;
        align-items: center;
        background-color: #e8f0fe;
        color: #1a73e8;
        border: none;
        border-radius: 4px;
        padding: 6px 12px;
        font-size: 14px;
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .action-button:hover {
        background-color: #d2e3fc;
    }

    .action-button i {
        margin-right: 4px;
        font-size: 18px;
    }

    .actions-cell {
        white-space: nowrap;
    }

    /* Состояния загрузки и отсутствия данных */
    .loading-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        padding: 40px;
        text-align: center;
    }

    .spinner {
        width: 40px;
        height: 40px;
        border: 3px solid #f3f3f3;
        border-top: 3px solid #1a73e8;
        border-radius: 50%;
        animation: spin 1s linear infinite;
        margin-bottom: 16px;
    }

    @keyframes spin {
        0% { transform: rotate(0deg); }
        100% { transform: rotate(360deg); }
    }

    .empty-state {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        padding: 40px;
        text-align: center;
        color: #5f6368;
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

    /* Сообщение об ошибке */
    .error-message {
        display: flex;
        align-items: center;
        background-color: #fdeded;
        color: #d93025;
        padding: 12px 16px;
        border-radius: 4px;
        margin-bottom: 20px;
        font-size: 14px;
    }

    .error-message i {
        margin-right: 8px;
        font-size: 20px;
    }

    .close-button {
        margin-left: auto;
        background: none;
        border: none;
        color: inherit;
        cursor: pointer;
        padding: 4px;
    }

    .close-button i {
        font-size: 18px;
    }

    /* Страница отказа в доступе */
    .no-access {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        text-align: center;
        min-height: 100vh;
        padding: 20px;
    }

    .no-access i {
        font-size: 64px;
        color: #d93025;
        margin-bottom: 16px;
    }

    .no-access h2 {
        font-size: 24px;
        margin-bottom: 8px;
        color: #202124;
    }

    .no-access p {
        font-size: 16px;
        color: #5f6368;
        margin-bottom: 24px;
    }

    .primary-button {
        display: flex;
        align-items: center;
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 4px;
        padding: 8px 16px;
        font-size: 14px;
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .primary-button:hover {
        background-color: #1765cc;
    }

    .primary-button i {
        margin-right: 8px;
    }

    /* Адаптивный дизайн */
    @media (max-width: 768px) {
        .stats-grid {
            grid-template-columns: 1fr;
        }

        .folders-grid {
            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
        }

        .user-header {
            flex-direction: column;
            align-items: center;
            text-align: center;
        }

        .user-avatar {
            margin-right: 0;
            margin-bottom: 16px;
        }

        .user-stats {
            flex-direction: column;
            gap: 8px;
            align-items: center;
        }

        .dashboard-tabs {
            flex-wrap: wrap;
        }

        .tab-button {
            flex: 1;
            min-width: 120px;
            justify-content: center;
        }
    }
</style>
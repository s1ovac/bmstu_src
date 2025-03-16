<script>
    import { loggedIn, email as emailStore, userRoles } from '../lib/stores.js';
    import FileTree from './FileTree.svelte';
    import { onMount, tick } from 'svelte';
    import { getUserRoles } from '../lib/api.js'

    let userEmail = '';
    let sidebarCollapsed = false;
    let mobileMenuOpen = false;
    let storageInfo = { used: 0, total: 0, percentage: 0 };
    let fileTreeComponent;
    let searchQuery = '';
    let isDarkMode = false;
    let showQuickActions = false;
    let isUploadMenuOpen = false;

    // Check for browser environment to prevent SSR issues
    const isBrowser = typeof window !== 'undefined';

    function getSavedSidebarState() {
        if (isBrowser) {
            return localStorage.getItem('sidebarCollapsed') === 'true';
        }
        return false;
    }

    // Обработчик нажатия клавиш для сочетаний клавиш
    function handleKeyDown(e) {
        // Ctrl+F для поиска
        if (e.ctrlKey && e.key === 'f') {
            e.preventDefault();
            document.querySelector('.search-input')?.focus();
        }

        // Ctrl+B для сворачивания/разворачивания сайдбара
        if (e.ctrlKey && e.key === 'b') {
            e.preventDefault();
            toggleSidebar();
        }

        // Ctrl+R для обновления файлов
        if (e.ctrlKey && e.key === 'r') {
            e.preventDefault();
            refreshFiles();
        }
    }

    onMount(async () => {
        console.log("Home component mounted");

        // Check authentication status immediately
        if (isBrowser) {
            try {
                const storedUser = localStorage.getItem("user");
                if (!storedUser) {
                    console.log("No user data found, redirecting to login");
                    window.location.href = '/login';
                    return;
                }

                const userData = JSON.parse(storedUser);
                if (!userData || !userData.token) {
                    console.log("Invalid user data, redirecting to login");
                    window.location.href = '/login';
                    return;
                }

                console.log("User is authenticated");

                // Continue with the rest of the initialization

                // Initialize sidebar state from localStorage
                sidebarCollapsed = getSavedSidebarState();

                document.addEventListener('keydown', handleKeyDown);

                userEmail = $emailStore || userData.email || '';

                // Check if dark mode is enabled in localStorage
                isDarkMode = localStorage.getItem('darkMode') === 'true';
                if (isDarkMode) {
                    document.body.classList.add('dark-mode');
                }

                try {
                    // Get user roles
                    const rolesResponse = await getUserRoles(userData.token, userData.user_id);
                    userRoles.set(rolesResponse.roles);
                    console.log("User roles loaded:", rolesResponse.roles);

                    // Симуляция получения информации о хранилище
                    storageInfo = {
                        used: 2.7,
                        total: 15,
                        percentage: 18
                    };
                } catch (err) {
                    console.error("Failed to get roles:", err);
                }

                // Update store values
                loggedIn.set(true);
                if (userEmail) {
                    emailStore.set(userEmail);
                }

            } catch (error) {
                console.error("Error during component initialization:", error);
                window.location.href = '/login';
                return;
            }
        }

        return () => {
            document.removeEventListener('keydown', handleKeyDown);
        };
    });

    function toggleSidebar() {
        sidebarCollapsed = !sidebarCollapsed;
        if (isBrowser) {
            localStorage.setItem('sidebarCollapsed', sidebarCollapsed);
        }
    }

    function toggleDarkMode() {
        isDarkMode = !isDarkMode;

        if (isBrowser) {
            localStorage.setItem('darkMode', isDarkMode);
            if (isDarkMode) {
                document.body.classList.add('dark-mode');
            } else {
                document.body.classList.remove('dark-mode');
            }
        }
    }

    function toggleMobileMenu() {
        mobileMenuOpen = !mobileMenuOpen;
    }

    function toggleQuickActions() {
        showQuickActions = !showQuickActions;
    }

    function toggleUploadMenu() {
        isUploadMenuOpen = !isUploadMenuOpen;
    }

    function onLogoutClick() {
        userRoles.set([]);
        if (isBrowser) {
            localStorage.removeItem("user");
        }
        loggedIn.set(false);
        emailStore.set('');
        window.location.href = '/login';
    }

    // Форматирование размера в ГБ с одним знаком после запятой
    function formatSize(gb) {
        return gb.toFixed(1);
    }

    // Обновление данных в FileTree компоненте
    function refreshFiles() {
        showQuickActions = false;
        if (fileTreeComponent) {
            fileTreeComponent.fetchData();
        }
    }

    function goToAdmin() {
        window.location.href = '/admin';
    }

    function openUploadDialog() {
        if (fileTreeComponent) {
            fileTreeComponent.triggerFileUpload();
        }
        showQuickActions = false;
    }

    function handleSearch() {
        if (fileTreeComponent && searchQuery.trim()) {
            fileTreeComponent.searchFiles(searchQuery);
        }
    }

    function clearSearch() {
        searchQuery = '';
        if (fileTreeComponent) {
            fileTreeComponent.clearSearch();
        }
    }

    // Define quick action buttons
    const quickActions = [
        { icon: 'upload_file', label: 'Загрузить файл', action: openUploadDialog },
        { icon: 'create_new_folder', label: 'Создать папку', action: () => fileTreeComponent?.openCreateFolderDialog() },
        { icon: 'refresh', label: 'Обновить', action: refreshFiles },
        { icon: 'admin_panel_settings', label: 'Администрирование', action: goToAdmin, showIf: () => $userRoles && $userRoles.find(r => r.role_name === 'admin') }
    ];
</script>

<svelte:head>
    <title>Cloud Storage</title>
    <link rel="stylesheet" href="https://fonts.googleapis.com/icon?family=Material+Icons" />
    <link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap" />
</svelte:head>

{#if loggedIn}
    <!-- Основной интерфейс хранилища для авторизованных пользователей -->
    <div class="app-container" class:dark-theme={isDarkMode}>
        <!-- Мобильная шапка -->
        <header class="mobile-header">
            <button class="icon-button" on:click={toggleMobileMenu} aria-label="Открыть меню">
                <i class="material-icons">menu</i>
            </button>
            <div class="mobile-logo">
                <img src="/bmstu_logo.png" alt="Cloud Logo" class="logo-image" />
                <span class="logo-text">Cloud</span>
            </div>
            <div class="mobile-actions">
                <button class="icon-button" on:click={toggleQuickActions} aria-label="Быстрые действия">
                    <i class="material-icons">add</i>
                </button>
                <button class="icon-button" aria-label="Профиль пользователя">
                    <i class="material-icons">account_circle</i>
                </button>
            </div>
        </header>

        <!-- Основной контейнер -->
        <div class="main-container">
            <!-- Сайдбар -->
            <nav class="sidebar" class:collapsed={sidebarCollapsed} class:mobile-open={mobileMenuOpen}>
                <div class="sidebar-header">
                    <div class="logo">
                        <img src="/bmstu_logo.png" alt="Cloud Logo" class="logo-image" />
                        <span class="logo-text">Cloud</span>
                    </div>
                    <button class="icon-button collapse-button" on:click={toggleSidebar} aria-label="Свернуть меню">
                        <i class="material-icons">{sidebarCollapsed ? 'chevron_right' : 'chevron_left'}</i>
                    </button>
                    <button class="icon-button close-mobile-menu" on:click={toggleMobileMenu} aria-label="Закрыть меню">
                        <i class="material-icons">close</i>
                    </button>
                </div>

                <!-- Навигация -->
                <div class="nav-container">
                    <ul class="nav-menu">
                        <li class="nav-item active">
                            <a href="#" class="nav-link">
                                <i class="material-icons nav-icon">cloud</i>
                                <span class="nav-text">Мой диск</span>
                            </a>
                        </li>
                        <li class="nav-item">
                            <a href="#" class="nav-link" on:click|preventDefault={refreshFiles}>
                                <i class="material-icons nav-icon">refresh</i>
                                <span class="nav-text">Обновить</span>
                            </a>
                        </li>
                        <li class="nav-item">
                            <a href="#" class="nav-link">
                                <i class="material-icons nav-icon">history</i>
                                <span class="nav-text">Недавние</span>
                            </a>
                        </li>
                        <li class="nav-item">
                            <a href="#" class="nav-link">
                                <i class="material-icons nav-icon">star</i>
                                <span class="nav-text">Избранное</span>
                            </a>
                        </li>
                        <li class="nav-item">
                            <a href="#" class="nav-link">
                                <i class="material-icons nav-icon">delete</i>
                                <span class="nav-text">Корзина</span>
                            </a>
                        </li>

                        {#if $userRoles && $userRoles.find(r => r.role_name === 'admin')}
                            <li class="nav-item">
                                <a href="/admin" class="nav-link">
                                    <i class="material-icons nav-icon">admin_panel_settings</i>
                                    <span class="nav-text">Администрирование</span>
                                </a>
                            </li>
                        {/if}
                    </ul>
                </div>

                <!-- Информация о хранилище -->
                <div class="storage-info">
                    <div class="storage-text">
                        <span class="storage-title">
                            <i class="material-icons">storage</i>
                            Хранилище
                        </span>
                        <span class="storage-details">
                            {formatSize(storageInfo.used)} ГБ из {storageInfo.total} ГБ ({storageInfo.percentage}%)
                        </span>
                    </div>
                    <div class="storage-progress">
                        <div
                                class="storage-progress-bar"
                                style="width: {storageInfo.percentage}%"
                                class:warning={storageInfo.percentage > 70}
                                class:danger={storageInfo.percentage > 90}
                        ></div>
                    </div>
                </div>

                <!-- Настройки и тема -->
                <div class="sidebar-settings">
                    <button class="theme-toggle" on:click={toggleDarkMode}>
                        <i class="material-icons">{isDarkMode ? 'light_mode' : 'dark_mode'}</i>
                        <span class="nav-text">{isDarkMode ? 'Светлая тема' : 'Тёмная тема'}</span>
                    </button>
                </div>

                <!-- Профиль пользователя -->
                <div class="user-profile">
                    <div class="user-avatar">
                        <i class="material-icons">account_circle</i>
                    </div>
                    <div class="user-info">
                        <span class="user-email">{userEmail}</span>
                        <button on:click={onLogoutClick} class="logout-button">
                            <i class="material-icons">exit_to_app</i>
                            <span>Выйти</span>
                        </button>
                    </div>
                </div>
            </nav>

            <!-- Основной контент -->
            <div class="content">
                <!-- Верхняя панель с поиском и кнопками -->
                <div class="toolbar">
                    <div class="search-box">
                        <div class="search-input-wrapper">
                            <i class="material-icons search-icon">search</i>
                            <input
                                    type="text"
                                    class="search-input"
                                    placeholder="Поиск в хранилище"
                                    bind:value={searchQuery}
                                    on:keydown={(e) => e.key === 'Enter' && handleSearch()}
                            />
                            {#if searchQuery}
                                <button class="clear-search" on:click={clearSearch}>
                                    <i class="material-icons">close</i>
                                </button>
                            {/if}
                        </div>
                    </div>

                    <div class="toolbar-actions">
                        <button class="action-button" on:click={toggleQuickActions}>
                            <i class="material-icons">add</i>
                            <span>Создать</span>
                        </button>

                        <!-- Выпадающее меню быстрых действий -->
                        {#if showQuickActions}
                            <div class="quick-actions-menu">
                                {#each quickActions as action}
                                    {#if !action.showIf || action.showIf()}
                                        <button class="quick-action-item" on:click={action.action}>
                                            <i class="material-icons">{action.icon}</i>
                                            <span>{action.label}</span>
                                        </button>
                                    {/if}
                                {/each}
                            </div>
                            <div class="overlay" on:click={toggleQuickActions}></div>
                        {/if}
                    </div>
                </div>

                <div class="file-manager-container">
                    <FileTree bind:this={fileTreeComponent} />
                </div>
            </div>
        </div>

        <!-- Мобильный оверлей для сайдбара -->
        {#if mobileMenuOpen}
            <div class="sidebar-overlay" on:click={toggleMobileMenu}></div>
        {/if}

        <!-- Мобильное меню быстрых действий -->
        {#if showQuickActions && window.innerWidth < 768}
            <div class="mobile-quick-actions">
                <div class="mobile-quick-actions-header">
                    <h3>Действия</h3>
                    <button class="icon-button" on:click={toggleQuickActions}>
                        <i class="material-icons">close</i>
                    </button>
                </div>
                <div class="mobile-quick-actions-content">
                    {#each quickActions as action}
                        {#if !action.showIf || action.showIf()}
                            <button class="mobile-quick-action-item" on:click={action.action}>
                                <i class="material-icons">{action.icon}</i>
                                <span>{action.label}</span>
                            </button>
                        {/if}
                    {/each}
                </div>
            </div>
        {/if}
    </div>
{/if}

<style>
    /* Общие стили */
    :global(body) {
        margin: 0;
        padding: 0;
        font-family: 'Roboto', sans-serif;
        color: #3c4043;
        background-color: #f8f9fa;
        overflow: hidden;
        transition: background-color 0.3s, color 0.3s;
    }

    :global(body.dark-mode) {
        background-color: #202124;
        color: #e8eaed;
    }

    .app-container {
        display: flex;
        flex-direction: column;
        height: 100vh;
        overflow: hidden;
    }

    .app-container.dark-theme {
        color: #e8eaed;
    }

    .main-container {
        display: flex;
        flex: 1;
        overflow: hidden;
    }

    /* Loading container */
    .loading-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        height: 100vh;
        background-color: #f8f9fa;
    }

    .spinner {
        width: 40px;
        height: 40px;
        border: 4px solid rgba(26, 115, 232, 0.2);
        border-radius: 50%;
        border-top-color: #1a73e8;
        animation: spin 1s linear infinite;
        margin-bottom: 16px;
    }

    @keyframes spin {
        to { transform: rotate(360deg); }
    }

    .loading-container span {
        font-family: 'Roboto', sans-serif;
        font-size: 16px;
        color: #5f6368;
    }

    /* Мобильная шапка */
    .mobile-header {
        display: none;
        padding: 8px 16px;
        background-color: #fff;
        border-bottom: 1px solid #e0e0e0;
        align-items: center;
        justify-content: space-between;
        height: 56px;
        z-index: 10;
    }

    .dark-theme .mobile-header {
        background-color: #292a2d;
        border-bottom-color: #3c4043;
    }

    .mobile-logo {
        display: flex;
        align-items: center;
    }

    .logo-image {
        width: 32px;
        height: 32px;
        object-fit: contain;
    }

    .logo-text {
        margin-left: 8px;
        font-size: 18px;
        font-weight: 500;
        color: #1a73e8;
        white-space: nowrap;
    }

    .mobile-actions {
        display: flex;
        gap: 8px;
    }

    /* Иконки и кнопки */
    .icon-button {
        background: transparent;
        border: none;
        border-radius: 50%;
        width: 40px;
        height: 40px;
        display: flex;
        align-items: center;
        justify-content: center;
        cursor: pointer;
        color: #5f6368;
        transition: background-color 0.2s;
    }

    .dark-theme .icon-button {
        color: #9aa0a6;
    }

    .icon-button:hover {
        background-color: rgba(95, 99, 104, 0.1);
    }

    .dark-theme .icon-button:hover {
        background-color: rgba(232, 234, 237, 0.08);
    }

    /* Сайдбар */
    .sidebar {
        width: 260px;
        height: 100%;
        background-color: #ffffff;
        border-right: 1px solid #e0e0e0;
        display: flex;
        flex-direction: column;
        transition: width 0.3s ease, transform 0.3s ease;
        overflow: hidden;
        position: relative;
        z-index: 10;
    }

    .dark-theme .sidebar {
        background-color: #292a2d;
        border-right-color: #3c4043;
    }

    .sidebar.collapsed {
        width: 72px;
    }

    .sidebar-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 0 16px;
        height: 64px;
        border-bottom: 1px solid #f1f3f4;
    }

    .dark-theme .sidebar-header {
        border-bottom-color: #3c4043;
    }

    .logo {
        display: flex;
        align-items: center;
        height: 100%;
    }

    .collapsed .logo-text {
        opacity: 0;
        width: 0;
        margin-left: 0;
    }

    .collapse-button {
        display: flex;
    }

    .close-mobile-menu {
        display: none;
    }

    /* Навигация */
    .nav-container {
        flex: 1;
        overflow-y: auto;
        padding: 8px 4px;
    }

    .nav-container::-webkit-scrollbar {
        width: 8px;
    }

    .nav-container::-webkit-scrollbar-track {
        background-color: transparent;
    }

    .nav-container::-webkit-scrollbar-thumb {
        background-color: #dadce0;
        border-radius: 4px;
    }

    .dark-theme .nav-container::-webkit-scrollbar-thumb {
        background-color: #5f6368;
    }

    .nav-menu {
        list-style: none;
        padding: 0;
        margin: 0;
    }

    .nav-item {
        margin-bottom: 4px;
    }

    .nav-link {
        display: flex;
        align-items: center;
        padding: 0 12px;
        height: 48px;
        border-radius: 0 24px 24px 0;
        text-decoration: none;
        color: #3c4043;
        transition: background-color 0.2s;
    }

    .dark-theme .nav-link {
        color: #e8eaed;
    }

    .nav-link:hover {
        background-color: #f1f3f4;
    }

    .dark-theme .nav-link:hover {
        background-color: rgba(232, 234, 237, 0.08);
    }

    .nav-item.active .nav-link {
        background-color: #e8f0fe;
        color: #1a73e8;
        font-weight: 500;
    }

    .dark-theme .nav-item.active .nav-link {
        background-color: rgba(26, 115, 232, 0.2);
    }

    .nav-icon {
        margin-right: 16px;
        font-size: 20px;
    }

    .collapsed .nav-icon {
        margin-right: 0;
    }

    .nav-text {
        font-size: 14px;
        transition: opacity 0.2s ease, width 0.2s ease;
        white-space: nowrap;
    }

    .collapsed .nav-text {
        opacity: 0;
        width: 0;
        overflow: hidden;
    }

    /* Информация о хранилище */
    .storage-info {
        padding: 16px;
        border-top: 1px solid #f1f3f4;
        transition: padding 0.3s ease;
    }

    .dark-theme .storage-info {
        border-top-color: #3c4043;
    }

    .collapsed .storage-info {
        padding: 16px 8px;
    }

    .storage-text {
        display: flex;
        flex-direction: column;
        margin-bottom: 12px;
        transition: opacity 0.2s ease;
    }

    .storage-title {
        display: flex;
        align-items: center;
        font-size: 14px;
        font-weight: 500;
        margin-bottom: 4px;
    }

    .storage-title i {
        font-size: 18px;
        margin-right: 8px;
        color: #1a73e8;
    }

    .collapsed .storage-text {
        opacity: 0;
        height: 0;
        margin-bottom: 0;
        overflow: hidden;
    }

    .storage-details {
        font-size: 12px;
        color: #5f6368;
    }

    .dark-theme .storage-details {
        color: #9aa0a6;
    }

    .storage-progress {
        height: 6px;
        background-color: #f1f3f4;
        border-radius: 3px;
        overflow: hidden;
    }

    .dark-theme .storage-progress {
        background-color: #3c4043;
    }

    .storage-progress-bar {
        height: 100%;
        background-color: #1a73e8;
        border-radius: 3px;
        transition: width 0.5s ease;
    }

    .storage-progress-bar.warning {
        background-color: #fbbc04;
    }

    .storage-progress-bar.danger {
        background-color: #ea4335;
    }

    /* Настройки темы */
    .sidebar-settings {
        padding: 8px 16px;
        border-top: 1px solid #f1f3f4;
    }

    .dark-theme .sidebar-settings {
        border-top-color: #3c4043;
    }

    .theme-toggle {
        display: flex;
        align-items: center;
        width: 100%;
        background: transparent;
        border: none;
        padding: 8px 0;
        cursor: pointer;
        color: #3c4043;
        text-align: left;
    }

    .dark-theme .theme-toggle {
        color: #e8eaed;
    }

    .theme-toggle i {
        margin-right: 16px;
        font-size: 20px;
        color: #5f6368;
    }

    .dark-theme .theme-toggle i {
        color: #9aa0a6;
    }

    .collapsed .theme-toggle i {
        margin-right: 0;
    }

    /* Профиль пользователя */
    .user-profile {
        display: flex;
        align-items: center;
        padding: 16px;
        margin-top: auto;
        border-top: 1px solid #f1f3f4;
    }

    .dark-theme .user-profile {
        border-top-color: #3c4043;
    }

    .user-avatar {
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 12px;
        transition: margin 0.3s ease;
    }

    .user-avatar i {
        font-size: 36px;
        color: #5f6368;
    }

    .dark-theme .user-avatar i {
        color: #9aa0a6;
    }

    .collapsed .user-avatar {
        margin-right: 0;
    }

    .user-info {
        display: flex;
        flex-direction: column;
        flex: 1;
        transition: opacity 0.2s ease, width 0.2s ease, overflow 0.1s ease;
        overflow: hidden;
    }

    .collapsed .user-info {
        opacity: 0;
        width: 0;
    }

    .user-email {
        font-size: 14px;
        font-weight: 500;
        white-space: nowrap;
        text-overflow: ellipsis;
        overflow: hidden;
        max-width: 180px;
    }

    .logout-button {
        display: flex;
        align-items: center;
        background-color: transparent;
        border: none;
        color: #1a73e8;
        padding: 4px 0;
        margin-top: 4px;
        cursor: pointer;
        font-size: 14px;
    }

    .dark-theme .logout-button {
        color: #8ab4f8;
    }

    .logout-button:hover {
        text-decoration: underline;
    }

    .logout-button i {
        font-size: 16px;
        margin-right: 4px;
    }

    /* Toolbar и поиск */
    .toolbar {
        height: 64px;
        background-color: #fff;
        border-bottom: 1px solid #e0e0e0;
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 0 16px;
        position: relative;
        z-index: 5;
    }

    .dark-theme .toolbar {
        background-color: #292a2d;
        border-bottom-color: #3c4043;
    }

    .search-box {
        flex: 1;
        max-width: 720px;
    }

    .search-input-wrapper {
        display: flex;
        align-items: center;
        background-color: #f1f3f4;
        border-radius: 8px;
        padding: 0 16px;
        height: 44px;
        transition: background-color 0.2s, box-shadow 0.2s;
        position: relative;
    }

    .dark-theme .search-input-wrapper {
        background-color: #3c4043;
    }

    .search-input-wrapper:focus-within {
        background-color: #fff;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.12);
    }

    .dark-theme .search-input-wrapper:focus-within {
        background-color: #3c4043;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.4);
    }

    .search-icon {
        color: #5f6368;
        font-size: 20px;
        margin-right: 8px;
    }

    .dark-theme .search-icon {
        color: #9aa0a6;
    }

    .search-input {
        flex: 1;
        border: none;
        background: transparent;
        font-size: 14px;
        color: #3c4043;
        outline: none;
        width: 100%;
    }

    .dark-theme .search-input {
        color: #e8eaed;
    }

    .search-input::placeholder {
        color: #5f6368;
    }

    .dark-theme .search-input::placeholder {
        color: #9aa0a6;
    }

    .clear-search {
        background: transparent;
        border: none;
        color: #5f6368;
        cursor: pointer;
        width: 24px;
        height: 24px;
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 0;
    }

    .dark-theme .clear-search {
        color: #9aa0a6;
    }

    .toolbar-actions {
        display: flex;
        position: relative;
    }

    .action-button {
        display: flex;
        align-items: center;
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 8px;
        height: 44px;
        padding: 0 16px;
        font-size: 14px;
        font-weight: 500;
        cursor: pointer;
        transition: background-color 0.2s;
    }

    .action-button:hover {
        background-color: #1765cc;
    }

    .action-button i {
        margin-right: 8px;
        font-size: 20px;
    }

    /* Выпадающее меню быстрых действий */
    .quick-actions-menu {
        position: absolute;
        top: 52px;
        right: 0;
        width: 220px;
        background-color: white;
        border-radius: 8px;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        padding: 8px 0;
        z-index: 100;
    }

    .dark-theme .quick-actions-menu {
        background-color: #292a2d;
        box-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
    }

    .quick-action-item {
        display: flex;
        align-items: center;
        width: 100%;
        padding: 0 16px;
        height: 48px;
        background: transparent;
        border: none;
        cursor: pointer;
        font-size: 14px;
        text-align: left;
        color: #3c4043;
        transition: background-color 0.2s;
    }

    .dark-theme .quick-action-item {
        color: #e8eaed;
    }

    .quick-action-item:hover {
        background-color: #f1f3f4;
    }

    .dark-theme .quick-action-item:hover {
        background-color: rgba(232, 234, 237, 0.08);
    }

    .quick-action-item i {
        margin-right: 12px;
        font-size: 20px;
        color: #5f6368;
    }

    .dark-theme .quick-action-item i {
        color: #9aa0a6;
    }

    .overlay {
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        z-index: 50;
    }

    /* Основной контент */
    .content {
        flex: 1;
        height: 100%;
        overflow: hidden;
        display: flex;
        flex-direction: column;
        background-color: #f8f9fa;
    }

    .dark-theme .content {
        background-color: #202124;
    }

    .file-manager-container {
        flex: 1;
        overflow: hidden;
        display: flex;
        flex-direction: column;
    }

    /* Мобильное меню быстрых действий */
    .mobile-quick-actions {
        position: fixed;
        bottom: 0;
        left: 0;
        right: 0;
        background-color: white;
        border-radius: 16px 16px 0 0;
        box-shadow: 0 -2px 10px rgba(0, 0, 0, 0.1);
        z-index: 100;
        overflow: hidden;
    }

    .dark-theme .mobile-quick-actions {
        background-color: #292a2d;
        box-shadow: 0 -2px 10px rgba(0, 0, 0, 0.3);
    }

    .mobile-quick-actions-header {
        display: flex;
        align-items: center;
        justify-content: space-between;
        padding: 16px;
        border-bottom: 1px solid #f1f3f4;
    }

    .dark-theme .mobile-quick-actions-header {
        border-bottom-color: #3c4043;
    }

    .mobile-quick-actions-header h3 {
        margin: 0;
        font-size: 16px;
        font-weight: 500;
    }

    .mobile-quick-actions-content {
        padding: 8px 0;
        max-height: 60vh;
        overflow-y: auto;
    }

    .mobile-quick-action-item {
        display: flex;
        align-items: center;
        width: 100%;
        padding: 0 16px;
        height: 56px;
        background: transparent;
        border: none;
        cursor: pointer;
        font-size: 16px;
        text-align: left;
        color: #3c4043;
    }

    .dark-theme .mobile-quick-action-item {
        color: #e8eaed;
    }

    .mobile-quick-action-item i {
        margin-right: 16px;
        font-size: 24px;
        color: #5f6368;
    }

    .dark-theme .mobile-quick-action-item i {
        color: #9aa0a6;
    }

    /* Адаптивность для мобильных устройств */
    @media (max-width: 768px) {
        .mobile-header {
            display: flex;
        }

        .main-container {
            position: relative;
            height: calc(100vh - 56px);
        }

        .sidebar {
            position: fixed;
            top: 56px;
            left: 0;
            height: calc(100% - 56px);
            z-index: 100;
            transform: translateX(-100%);
            width: 280px !important;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }

        .dark-theme .sidebar {
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
        }

        .sidebar.mobile-open {
            transform: translateX(0);
        }

        .close-mobile-menu {
            display: flex;
        }

        .collapse-button {
            display: none;
        }

        .sidebar-overlay {
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgba(0, 0, 0, 0.5);
            z-index: 90;
        }

        .toolbar {
            height: 56px;
        }

        .search-box {
            max-width: none;
        }

        .search-input-wrapper {
            height: 40px;
        }

        .action-button {
            height: 40px;
            padding: 0 12px;
        }

        .action-button span {
            display: none;
        }

        .action-button i {
            margin-right: 0;
        }
    }
</style>
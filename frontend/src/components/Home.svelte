<!-- src/components/Home.svelte -->
<script>
    import { loggedIn, email as emailStore, userRoles } from '../lib/stores.js';
    import FileTree from './FileTree.svelte';
    import LoginComponent from './Login.svelte';
    import { onMount } from 'svelte';
    import { getUserRoles } from '../lib/api.js'

    let userEmail = '';
    let isMobileSidebarOpen = false;
    let darkMode = false;
    let currentPath = '';
    let isSharedMode = false;

    onMount(async () => {
        userEmail = $emailStore;
        const storedUser = JSON.parse(localStorage.getItem("user"));
        if (storedUser && storedUser.token && storedUser.user_id) {
            try {
                const rolesResponse = await getUserRoles(storedUser.token, storedUser.user_id);
                userRoles.set(rolesResponse.roles);
            } catch (err) {
                console.error("Failed to get roles:", err);
            }
        }

        // Check for stored theme preference
        const storedTheme = localStorage.getItem("theme");
        if (storedTheme) {
            darkMode = storedTheme === "dark";
            updateTheme();
        }

        // Check current URL parameters
        checkCurrentPath();

        // Listen for URL changes
        window.addEventListener('popstate', checkCurrentPath);
    });

    const checkCurrentPath = () => {
        const urlParams = new URLSearchParams(window.location.search);
        isSharedMode = urlParams.get('shared') === 'true';
        currentPath = window.location.pathname + window.location.search;
    };

    const onLogoutClick = () => {
        userRoles.set([]);
        localStorage.removeItem("user");
        loggedIn.set(false);
        emailStore.set('');
        window.location.href = '/login';
    };

    const toggleSidebar = () => {
        isMobileSidebarOpen = !isMobileSidebarOpen;
    };

    const toggleDarkMode = () => {
        darkMode = !darkMode;
        localStorage.setItem("theme", darkMode ? "dark" : "light");
        updateTheme();
    };

    const updateTheme = () => {
        if (darkMode) {
            document.body.classList.add('dark-mode');
        } else {
            document.body.classList.remove('dark-mode');
        }
    };

    const navigateToPath = (path) => {
        window.history.pushState(null, '', path);
        checkCurrentPath();
        // Close mobile sidebar after navigation
        isMobileSidebarOpen = false;
    };
</script>

{#if !$loggedIn}
    <!-- Если пользователь не авторизован, показываем компонент логина -->
    <LoginComponent />
{:else}
    <!-- Основной интерфейс хранилища для авторизованных пользователей -->
    <div class="app-container {darkMode ? 'dark-theme' : 'light-theme'}">
        <!-- Мобильная панель навигации -->
        <div class="mobile-nav">
            <button class="menu-toggle" on:click={toggleSidebar}>
                <i class="material-icons">{isMobileSidebarOpen ? 'close' : 'menu'}</i>
            </button>
            <div class="mobile-logo">
                <img src="/bmstu_logo.png" alt="Cloud Logo" />
                <span>Cloud</span>
            </div>
            <div class="mobile-user">
                <i class="material-icons">account_circle</i>
            </div>
        </div>

        <!-- Сайдбар -->
        <nav class="sidebar {isMobileSidebarOpen ? 'mobile-open' : ''}">
            <!-- Логотип -->
            <div class="logo">
                <img src="/bmstu_logo.png" alt="Cloud Logo" />
                <span>Cloud</span>
            </div>

            <!-- Основная навигация -->
            <div class="sidebar-section">
                <h3>Хранилище</h3>
                <ul class="nav-items">
                    <li class="nav-item {currentPath === '/' ? 'active' : ''}">
                        <button class="nav-link" on:click={() => navigateToPath('/')}>
                            <i class="material-icons">cloud</i>
                            <span>Мой диск</span>
                        </button>
                    </li>
                    <li class="nav-item {isSharedMode ? 'active' : ''}">
                        <button class="nav-link" on:click={() => navigateToPath('/?shared=true')}>
                            <i class="material-icons">group</i>
                            <span>Общие файлы</span>
                        </button>
                    </li>
                    <li class="nav-item">
                        <button class="nav-link" on:click={() => navigateToPath('/?favorites=true')}>
                            <i class="material-icons">star</i>
                            <span>Избранное</span>
                        </button>
                    </li>
                    <li class="nav-item">
                        <button class="nav-link" on:click={() => navigateToPath('/?recent=true')}>
                            <i class="material-icons">access_time</i>
                            <span>Недавние</span>
                        </button>
                    </li>
                    <li class="nav-item">
                        <button class="nav-link" on:click={() => navigateToPath('/?trash=true')}>
                            <i class="material-icons">delete</i>
                            <span>Корзина</span>
                        </button>
                    </li>
                </ul>
            </div>

            <!-- Админ-панель (если пользователь - админ) -->
            {#if $userRoles && $userRoles.find(r => r.role_name === 'admin')}
                <div class="sidebar-section">
                    <h3>Администрирование</h3>
                    <ul class="nav-items">
                        <li class="nav-item">
                            <a href="/admin" class="nav-link">
                                <i class="material-icons">admin_panel_settings</i>
                                <span>Панель управления</span>
                            </a>
                        </li>
                        <li class="nav-item">
                            <a href="/admin-dashboard" class="nav-link">
                                <i class="material-icons">group</i>
                                <span>Пользователи</span>
                            </a>
                        </li>
                    </ul>
                </div>
            {/if}

            <!-- Настройки и тема -->
            <div class="sidebar-section">
                <h3>Настройки</h3>
                <ul class="nav-items">
                    <li class="nav-item">
                        <button class="nav-link theme-toggle" on:click={toggleDarkMode}>
                            <i class="material-icons">{darkMode ? 'light_mode' : 'dark_mode'}</i>
                            <span>{darkMode ? 'Светлая тема' : 'Темная тема'}</span>
                        </button>
                    </li>
                    <li class="nav-item">
                        <a href="/settings" class="nav-link">
                            <i class="material-icons">settings</i>
                            <span>Настройки</span>
                        </a>
                    </li>
                </ul>
            </div>

            <!-- Профиль пользователя -->
            <div class="user-profile">
                <div class="user-avatar">
                    <i class="material-icons">account_circle</i>
                </div>
                <div class="user-info">
                    <span class="user-email">{userEmail}</span>
                    <button on:click={onLogoutClick} class="logout-button">
                        <i class="material-icons">logout</i>
                        <span>Выйти</span>
                    </button>
                </div>
            </div>
        </nav>

        <!-- Основной контент -->
        <main class="main-content">
            <div class="content-container">
                <FileTree sharedMode={isSharedMode} />
            </div>
        </main>
    </div>
{/if}

<style>
    /* Подключаем шрифты */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap');

    /* Общие стили */
    :global(body) {
        margin: 0;
        padding: 0;
        font-family: 'Roboto', sans-serif;
        transition: background-color 0.3s ease, color 0.3s ease;
    }

    :global(body.dark-mode) {
        background-color: #1a1a1a;
        color: #f5f5f5;
    }

    .app-container {
        display: flex;
        height: 100vh;
        width: 100%;
        position: relative;
        overflow: hidden;
    }

    /* Темы */
    .light-theme {
        --bg-primary: #ffffff;
        --bg-secondary: #f5f8fa;
        --bg-tertiary: #eef2f6;
        --text-primary: #202124;
        --text-secondary: #5f6368;
        --accent-color: #1a73e8;
        --accent-hover: #1765cc;
        --border-color: #dadce0;
        --hover-bg: #f1f3f4;
        --shadow-color: rgba(0, 0, 0, 0.1);
    }

    .dark-theme {
        --bg-primary: #202124;
        --bg-secondary: #2d2e31;
        --bg-tertiary: #3c3d40;
        --text-primary: #e8eaed;
        --text-secondary: #9aa0a6;
        --accent-color: #8ab4f8;
        --accent-hover: #aecbfa;
        --border-color: #5f6368;
        --hover-bg: #3c4043;
        --shadow-color: rgba(0, 0, 0, 0.3);
    }

    /* Мобильная панель навигации */
    .mobile-nav {
        display: none;
        position: fixed;
        top: 0;
        left: 0;
        right: 0;
        height: 60px;
        background-color: var(--bg-primary);
        border-bottom: 1px solid var(--border-color);
        padding: 0 16px;
        align-items: center;
        justify-content: space-between;
        z-index: 1000;
        box-shadow: 0 2px 5px var(--shadow-color);
    }

    .menu-toggle {
        background: none;
        border: none;
        color: var(--text-primary);
        cursor: pointer;
        padding: 8px;
        border-radius: 50%;
        display: flex;
        align-items: center;
        justify-content: center;
    }

    .menu-toggle:hover {
        background-color: var(--hover-bg);
    }

    .mobile-logo {
        display: flex;
        align-items: center;
    }

    .mobile-logo img {
        width: 30px;
        height: 30px;
        margin-right: 8px;
    }

    .mobile-logo span {
        font-size: 18px;
        font-weight: 500;
        color: var(--accent-color);
    }

    .mobile-user {
        width: 36px;
        height: 36px;
        display: flex;
        align-items: center;
        justify-content: center;
        border-radius: 50%;
        cursor: pointer;
    }

    .mobile-user:hover {
        background-color: var(--hover-bg);
    }

    /* Сайдбар */
    .sidebar {
        width: 260px;
        height: 100vh;
        background-color: var(--bg-primary);
        border-right: 1px solid var(--border-color);
        display: flex;
        flex-direction: column;
        transition: transform 0.3s ease;
        z-index: 100;
        overflow-y: auto;
    }

    .logo {
        display: flex;
        align-items: center;
        padding: 24px 20px;
        margin-bottom: 8px;
    }

    .logo img {
        width: 40px;
        height: 40px;
        margin-right: 12px;
    }

    .logo span {
        font-size: 22px;
        font-weight: 500;
        color: var(--accent-color);
    }

    .sidebar-section {
        margin-bottom: 24px;
        padding: 0 12px;
    }

    .sidebar-section h3 {
        font-size: 12px;
        font-weight: 500;
        color: var(--text-secondary);
        text-transform: uppercase;
        margin: 0 0 8px 12px;
        letter-spacing: 0.8px;
    }

    .nav-items {
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
        padding: 10px 12px;
        border-radius: 8px;
        color: var(--text-primary);
        text-decoration: none;
        font-size: 14px;
        transition: background-color 0.2s;
        cursor: pointer;
        border: none;
        background: none;
        width: 100%;
        text-align: left;
        font-family: 'Roboto', sans-serif;
    }

    .nav-link:hover {
        background-color: var(--hover-bg);
    }

    .nav-item.active .nav-link {
        background-color: var(--hover-bg);
        color: var(--accent-color);
        font-weight: 500;
    }

    .nav-link i {
        margin-right: 16px;
        font-size: 20px;
    }

    /* Профиль пользователя */
    .user-profile {
        margin-top: auto;
        border-top: 1px solid var(--border-color);
        padding: 16px;
        display: flex;
        align-items: center;
    }

    .user-avatar {
        width: 40px;
        height: 40px;
        border-radius: 50%;
        background-color: var(--bg-tertiary);
        display: flex;
        align-items: center;
        justify-content: center;
        margin-right: 12px;
    }

    .user-avatar i {
        font-size: 24px;
        color: var(--accent-color);
    }

    .user-info {
        display: flex;
        flex-direction: column;
    }

    .user-email {
        font-size: 14px;
        margin-bottom: 4px;
        color: var(--text-primary);
        white-space: nowrap;
        overflow: hidden;
        text-overflow: ellipsis;
        max-width: 160px;
    }

    .logout-button {
        display: flex;
        align-items: center;
        background: none;
        border: none;
        padding: 0;
        font-size: 12px;
        color: var(--accent-color);
        cursor: pointer;
    }

    .logout-button:hover {
        color: var(--accent-hover);
        text-decoration: underline;
    }

    .logout-button i {
        font-size: 16px;
        margin-right: 4px;
    }

    /* Основной контент */
    .main-content {
        flex: 1;
        background-color: var(--bg-secondary);
        height: 100%;
        overflow-y: auto;
    }

    .content-container {
        padding: 24px;
        max-width: 1400px;
        margin: 0 auto;
    }

    /* Адаптивный дизайн */
    @media (max-width: 768px) {
        .mobile-nav {
            display: flex;
        }

        .sidebar {
            position: fixed;
            top: 60px;
            bottom: 0;
            transform: translateX(-100%);
            box-shadow: 2px 0 10px var(--shadow-color);
        }

        .sidebar.mobile-open {
            transform: translateX(0);
        }

        .main-content {
            margin-top: 60px;
        }

        .content-container {
            padding: 16px;
        }
    }
</style>
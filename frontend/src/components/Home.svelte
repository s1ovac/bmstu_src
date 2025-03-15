<!-- src/components/Home.svelte -->
<script>
    import { loggedIn, email as emailStore, userRoles } from '../lib/stores.js';
    import FileTree from './FileTree.svelte';
    import LoginComponent from './Login.svelte';
    import { onMount } from 'svelte';
    import { getUserRoles } from '../lib/api.js'

    let userEmail = '';

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
    });

    const onLogoutClick = () => {
        userRoles.set([]);
        localStorage.removeItem("user");
        loggedIn.set(false);
        emailStore.set('');
        window.location.href = '/login';
    };
</script>

{#if !$loggedIn}
    <!-- Если пользователь не авторизован, показываем компонент логина -->
    <LoginComponent />
{:else}
    <!-- Основной интерфейс хранилища для авторизованных пользователей -->
    <div class="mainContainer">
        <!-- Сайдбар -->
        <nav class="sidebar">
            <!-- Логотип -->
            <div class="logo">
                <img src="/bmstu_logo.png" alt="MyCloud Logo" />
                <span>Cloud</span>
            </div>
            <!-- Меню навигации -->
            <ul>
                <li>
                    <a href="#" class="navItem">
                        <i class="material-icons">cloud</i>
                        <span>Мой диск</span>
                    </a>
                </li>
                {#if $userRoles && $userRoles.find(r => r.role_name === 'admin')}
                    <li>
                        <a href="/admin" class="navItem">
                            <i class="material-icons">admin_panel_settings</i>
                            <span>Админка</span>
                        </a>
                    </li>
                {/if}
                <!-- Дополнительные элементы меню можно добавить здесь -->
            </ul>
            <!-- Профиль пользователя -->
            <div class="userProfile">
                <div class="userAvatar">
                    <i class="material-icons">account_circle</i>
                </div>
                <div class="userInfo">
                    <span class="userEmail">{userEmail}</span>
                    <button on:click={onLogoutClick} class="logoutButton">Выйти</button>
                </div>
            </div>
        </nav>

        <!-- Контент -->
        <div class="content">
            <!-- Верхняя панель управления -->
            <div class="topBar">
                <!-- Хлебные крошки будут отображаться в компоненте FileTree -->
            </div>

            <!-- Основной контент -->
            <div class="fileTreeContainer">
                <FileTree />
            </div>
        </div>
    </div>
{/if}

<style>
    /* Подключаем шрифт Material Icons */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');

    /* Общие стили */
    * {
        box-sizing: border-box;
    }

    body {
        margin: 0;
        font-family: 'Roboto', sans-serif;
    }

    /* Основной контейнер */
    .mainContainer {
        display: flex;
        height: 100vh;
        background-color: #f1f3f4;
    }

    /* Сайдбар */
    .sidebar {
        width: 250px;
        background-color: #202124;
        color: white;
        display: flex;
        flex-direction: column;
        padding: 20px;
    }

    .sidebar .logo {
        display: flex;
        align-items: center;
        margin-bottom: 30px;
    }

    .sidebar .logo img {
        width: 36px;
        height: 36px;
        margin-right: 10px;
    }

    .sidebar .logo span {
        font-size: 24px;
        font-weight: bold;
        color: #8ab4f8;
    }

    .sidebar ul {
        list-style: none;
        padding: 0;
    }

    .sidebar ul li {
        margin-bottom: 10px;
    }

    .navItem {
        display: flex;
        align-items: center;
        color: white;
        text-decoration: none;
        font-size: 16px;
        padding: 10px;
        border-radius: 8px;
        transition: background-color 0.3s;
    }

    .navItem:hover {
        background-color: #3c4043;
    }

    .navItem i.material-icons {
        margin-right: 15px;
    }

    /* Профиль пользователя */
    .userProfile {
        margin-top: auto;
        display: flex;
        align-items: center;
        border-top: 1px solid #5f6368;
        padding-top: 20px;
    }

    .userAvatar i.material-icons {
        font-size: 36px;
    }

    .userInfo {
        margin-left: 10px;
        display: flex;
        flex-direction: column;
    }

    .userEmail {
        font-size: 14px;
        margin-bottom: 5px;
    }

    .logoutButton {
        background-color: #4285f4;
        color: white;
        border: none;
        padding: 8px 12px;
        font-size: 14px;
        cursor: pointer;
        border-radius: 4px;
        transition: background-color 0.3s;
    }

    .logoutButton:hover {
        background-color: #3367d6;
    }

    /* Контент */
    .content {
        flex-grow: 1;
        display: flex;
        flex-direction: column;
    }

    .topBar {
        background-color: white;
        padding: 10px 20px;
        display: flex;
        justify-content: space-between;
        align-items: center;
        border-bottom: 1px solid #dadce0;
    }

    /* Основной контент */
    .fileTreeContainer {
        padding: 20px;
        background-color: #ffffff;
        flex-grow: 1;
        overflow-y: auto;
    }
</style>

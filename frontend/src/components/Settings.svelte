<script>
    import { onMount } from 'svelte';
    import { fade, slide } from 'svelte/transition';
    import { loggedIn, email as emailStore, userRoles } from '../lib/stores.js';
    import { changePassword as changePasswordApi } from '../lib/api.js';

    // Локальные переменные для настроек
    let userEmail = '';
    let darkMode = false;
    let viewMode = 'grid'; // grid или list
    let language = 'ru';
    let notifications = {
        email: true,
        browser: true
    };

    // Переменные для смены пароля
    let currentPassword = '';
    let newPassword = '';
    let confirmPassword = '';
    let isChangingPassword = false;

    // Переменные для формы
    let successMessage = '';
    let errorMessage = '';
    let passwordChangeSuccess = false;
    let passwordChangeError = '';
    let activeTab = 'appearance'; // appearance, security, notifications, profile

    // Мобильная навигация
    let isMobileMenuOpen = false;

    // Переменные для анимации
    let animatingTab = false;

    onMount(() => {
        // Проверяем, авторизован ли пользователь
        if (!$loggedIn) {
            window.location.href = '/login';
            return;
        }

        // Загружаем данные пользователя
        userEmail = $emailStore;

        // Загружаем сохраненные настройки из localStorage
        const storedTheme = localStorage.getItem("theme");
        if (storedTheme) {
            darkMode = storedTheme === "dark";
            updateTheme();
        } else {
            // Определяем системную тему пользователя
            const prefersDarkScheme = window.matchMedia("(prefers-color-scheme: dark)").matches;
            if (prefersDarkScheme) {
                darkMode = true;
                updateTheme();
            }
        }

        const storedViewMode = localStorage.getItem("viewMode");
        if (storedViewMode) {
            viewMode = storedViewMode;
        }

        const storedLanguage = localStorage.getItem("language");
        if (storedLanguage) {
            language = storedLanguage;
        }

        const storedNotifications = localStorage.getItem("notifications");
        if (storedNotifications) {
            try {
                notifications = JSON.parse(storedNotifications);
            } catch (e) {
                console.error("Error parsing notifications settings:", e);
            }
        }
    });

    // Функция для обновления темы
    const updateTheme = () => {
        if (darkMode) {
            document.body.classList.add('dark-mode');
        } else {
            document.body.classList.remove('dark-mode');
        }
    };

    // Функция для переключения темы
    const toggleDarkMode = () => {
        darkMode = !darkMode;
        localStorage.setItem("theme", darkMode ? "dark" : "light");
        updateTheme();
        showSuccess("Тема успешно изменена");
    };

    // Функция для изменения режима просмотра файлов
    const changeViewMode = (mode) => {
        viewMode = mode;
        localStorage.setItem("viewMode", viewMode);
        showSuccess("Режим просмотра успешно изменен");
    };

    // Функция для изменения языка
    const changeLanguage = (lang) => {
        language = lang;
        localStorage.setItem("language", language);
        showSuccess("Язык успешно изменен");
    };

    // Функция для сохранения настроек уведомлений
    const saveNotificationSettings = () => {
        localStorage.setItem("notifications", JSON.stringify(notifications));
        showSuccess("Настройки уведомлений сохранены");
    };

    // Функция для смены пароля
    const handlePasswordChange = async () => {
        // Сброс предыдущих сообщений
        passwordChangeSuccess = false;
        passwordChangeError = '';

        // Проверка валидности ввода
        if (!currentPassword) {
            passwordChangeError = "Введите текущий пароль";
            return;
        }

        if (!newPassword) {
            passwordChangeError = "Введите новый пароль";
            return;
        }

        if (newPassword.length < 6) {
            passwordChangeError = "Новый пароль должен содержать не менее 6 символов";
            return;
        }

        if (newPassword !== confirmPassword) {
            passwordChangeError = "Пароли не совпадают";
            return;
        }

        try {
            // Получаем JWT токен из localStorage
            const storedUser = JSON.parse(localStorage.getItem("user"));
            if (!storedUser || !storedUser.token) {
                passwordChangeError = "Необходимо авторизоваться";
                return;
            }

            // Отображаем индикатор загрузки
            isChangingPassword = true;

            // Вызываем API для смены пароля
            await changePasswordApi(storedUser.token, currentPassword, newPassword);

            // Очищаем поля и показываем успех
            passwordChangeSuccess = true;
            currentPassword = '';
            newPassword = '';
            confirmPassword = '';
        } catch (error) {
            // Отображаем сообщение об ошибке
            passwordChangeError = error.message || "Ошибка при смене пароля";
        } finally {
            // Скрываем индикатор загрузки
            isChangingPassword = false;
        }
    };

    // Функция для отображения сообщения об успехе
    const showSuccess = (message) => {
        successMessage = message;
        setTimeout(() => {
            successMessage = '';
        }, 3000);
    };

    // Функция для переключения вкладки с анимацией
    const switchTab = (tab) => {
        if (activeTab === tab) return;

        // Включаем анимацию
        animatingTab = true;

        // Меняем вкладку
        setTimeout(() => {
            activeTab = tab;
            // Выключаем анимацию после перехода
            setTimeout(() => {
                animatingTab = false;
            }, 300);
        }, 150);

        // Закрываем мобильное меню при переключении вкладки
        isMobileMenuOpen = false;
    };

    // Функция для возврата на главную
    const goHome = () => {
        window.location.href = '/';
    };

    // Функция для переключения мобильного меню
    const toggleMobileMenu = () => {
        isMobileMenuOpen = !isMobileMenuOpen;
    };
</script>

<div class="settings-container {darkMode ? 'dark-theme' : 'light-theme'}">
    <!-- Заголовок с кнопкой возврата -->
    <header class="settings-header">
        <button class="back-button" on:click={goHome}>
            <i class="material-icons">arrow_back</i>
            <span>На главную</span>
        </button>
        <h1>Настройки</h1>

        <!-- Мобильное меню -->
        <button class="mobile-menu-toggle" on:click={toggleMobileMenu}>
            <i class="material-icons">{isMobileMenuOpen ? 'close' : 'menu'}</i>
        </button>
    </header>

    <!-- Сообщения о результате операций -->
    {#if successMessage}
        <div class="message success-message" transition:slide={{ duration: 300 }}>
            <div class="message-content">
                <i class="material-icons">check_circle</i>
                <span>{successMessage}</span>
            </div>
        </div>
    {/if}

    {#if errorMessage}
        <div class="message error-message" transition:slide={{ duration: 300 }}>
            <div class="message-content">
                <i class="material-icons">error</i>
                <span>{errorMessage}</span>
            </div>
        </div>
    {/if}

    <div class="settings-layout">
        <!-- Боковая навигация по разделам настроек -->
        <nav class="settings-nav {isMobileMenuOpen ? 'mobile-open' : ''}">
            <div class="nav-items">
                <button
                        class="nav-item {activeTab === 'appearance' ? 'active' : ''}"
                        on:click={() => switchTab('appearance')}
                >
                    <i class="material-icons">palette</i>
                    <span>Внешний вид</span>
                </button>

                <button
                        class="nav-item {activeTab === 'security' ? 'active' : ''}"
                        on:click={() => switchTab('security')}
                >
                    <i class="material-icons">security</i>
                    <span>Безопасность</span>
                </button>

                <button
                        class="nav-item {activeTab === 'notifications' ? 'active' : ''}"
                        on:click={() => switchTab('notifications')}
                >
                    <i class="material-icons">notifications</i>
                    <span>Уведомления</span>
                </button>

                <button
                        class="nav-item {activeTab === 'profile' ? 'active' : ''}"
                        on:click={() => switchTab('profile')}
                >
                    <i class="material-icons">person</i>
                    <span>Профиль</span>
                </button>
            </div>
        </nav>

        <!-- Содержимое настроек -->
        <div class="settings-content {animatingTab ? 'animating' : ''}">
            <!-- Настройки внешнего вида -->
            {#if activeTab === 'appearance'}
                <div class="settings-section" transition:fade={{ duration: 200 }}>
                    <h2>Настройки внешнего вида</h2>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Тема</h3>
                            <p>Выберите светлую или темную тему оформления</p>
                        </div>
                        <div class="setting-control theme-control">
                            <button
                                    class="theme-button {!darkMode ? 'active' : ''}"
                                    on:click={() => {
                                    if (darkMode) toggleDarkMode();
                                }}
                            >
                                <i class="material-icons">light_mode</i>
                                <span>Светлая</span>
                            </button>

                            <button
                                    class="theme-button {darkMode ? 'active' : ''}"
                                    on:click={() => {
                                    if (!darkMode) toggleDarkMode();
                                }}
                            >
                                <i class="material-icons">dark_mode</i>
                                <span>Темная</span>
                            </button>
                        </div>
                    </div>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Вид файлов по умолчанию</h3>
                            <p>Выберите режим отображения файлов</p>
                        </div>
                        <div class="setting-control view-control">
                            <button
                                    class="view-button {viewMode === 'grid' ? 'active' : ''}"
                                    on:click={() => changeViewMode('grid')}
                            >
                                <i class="material-icons">grid_view</i>
                                <span>Сетка</span>
                            </button>

                            <button
                                    class="view-button {viewMode === 'list' ? 'active' : ''}"
                                    on:click={() => changeViewMode('list')}
                            >
                                <i class="material-icons">view_list</i>
                                <span>Список</span>
                            </button>
                        </div>
                    </div>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Язык</h3>
                            <p>Выберите язык интерфейса</p>
                        </div>
                        <div class="setting-control">
                            <div class="select-wrapper">
                                <select
                                        bind:value={language}
                                        on:change={() => changeLanguage(language)}
                                        class="language-select"
                                >
                                    <option value="ru">Русский</option>
                                    <option value="en">English</option>
                                    <option value="de">Deutsch</option>
                                    <option value="fr">Français</option>
                                </select>
                                <i class="material-icons select-icon">expand_more</i>
                            </div>
                        </div>
                    </div>
                </div>
            {/if}

            <!-- Настройки безопасности -->
            {#if activeTab === 'security'}
                <div class="settings-section" transition:fade={{ duration: 200 }}>
                    <h2>Настройки безопасности</h2>

                    <div class="setting-item password-change">
                        <div class="setting-info">
                            <h3>Изменить пароль</h3>
                            <p>Для смены пароля введите текущий пароль и новый пароль дважды</p>
                        </div>

                        <div class="password-form">
                            {#if passwordChangeSuccess}
                                <div class="success-message password-message" transition:slide={{ duration: 300 }}>
                                    <i class="material-icons">check_circle</i>
                                    <span>Пароль успешно изменен</span>
                                </div>
                            {/if}

                            {#if passwordChangeError}
                                <div class="error-message password-message" transition:slide={{ duration: 300 }}>
                                    <i class="material-icons">error</i>
                                    <span>{passwordChangeError}</span>
                                </div>
                            {/if}

                            <div class="form-group">
                                <label for="currentPassword">Текущий пароль</label>
                                <div class="input-wrapper">
                                    <i class="material-icons input-icon">lock</i>
                                    <input
                                            type="password"
                                            id="currentPassword"
                                            bind:value={currentPassword}
                                            placeholder="Введите текущий пароль"
                                    />
                                </div>
                            </div>

                            <div class="form-group">
                                <label for="newPassword">Новый пароль</label>
                                <div class="input-wrapper">
                                    <i class="material-icons input-icon">lock_open</i>
                                    <input
                                            type="password"
                                            id="newPassword"
                                            bind:value={newPassword}
                                            placeholder="Введите новый пароль"
                                    />
                                </div>
                            </div>

                            <div class="form-group">
                                <label for="confirmPassword">Подтверждение пароля</label>
                                <div class="input-wrapper">
                                    <i class="material-icons input-icon">lock_outline</i>
                                    <input
                                            type="password"
                                            id="confirmPassword"
                                            bind:value={confirmPassword}
                                            placeholder="Подтвердите новый пароль"
                                    />
                                </div>
                            </div>

                            <button
                                    class="change-password-button"
                                    on:click={handlePasswordChange}
                                    disabled={isChangingPassword}
                            >
                                {#if isChangingPassword}
                                    <div class="button-spinner"></div>
                                    <span>Изменение пароля...</span>
                                {:else}
                                    <i class="material-icons">save</i>
                                    <span>Изменить пароль</span>
                                {/if}
                            </button>
                        </div>
                    </div>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Двухфакторная аутентификация</h3>
                            <p>Повысьте безопасность вашего аккаунта с помощью двухфакторной аутентификации</p>
                        </div>
                        <div class="setting-control">
                            <button class="two-factor-button">
                                <i class="material-icons">smartphone</i>
                                <span>Настроить 2FA</span>
                            </button>
                        </div>
                    </div>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Активные сессии</h3>
                            <p>Просмотр и управление активными сессиями</p>
                        </div>
                        <div class="setting-control">
                            <button class="sessions-button">
                                <i class="material-icons">devices</i>
                                <span>Управление сессиями</span>
                            </button>
                        </div>
                    </div>
                </div>
            {/if}

            <!-- Настройки уведомлений -->
            {#if activeTab === 'notifications'}
                <div class="settings-section" transition:fade={{ duration: 200 }}>
                    <h2>Настройки уведомлений</h2>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Уведомления по электронной почте</h3>
                            <p>Получайте уведомления о важных событиях на вашу электронную почту</p>
                        </div>
                        <div class="setting-control">
                            <label class="toggle-switch">
                                <input
                                        type="checkbox"
                                        bind:checked={notifications.email}
                                        on:change={saveNotificationSettings}
                                >
                                <span class="switch-slider"></span>
                            </label>
                        </div>
                    </div>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Уведомления в браузере</h3>
                            <p>Получайте уведомления от приложения в браузере</p>
                        </div>
                        <div class="setting-control">
                            <label class="toggle-switch">
                                <input
                                        type="checkbox"
                                        bind:checked={notifications.browser}
                                        on:change={saveNotificationSettings}
                                >
                                <span class="switch-slider"></span>
                            </label>
                        </div>
                    </div>

                    <div class="setting-item">
                        <div class="setting-info">
                            <h3>Звуковые уведомления</h3>
                            <p>Включить звуковое сопровождение уведомлений</p>
                        </div>
                        <div class="setting-control">
                            <label class="toggle-switch">
                                <input
                                        type="checkbox"
                                        bind:checked={notifications.sound}
                                        on:change={saveNotificationSettings}
                                >
                                <span class="switch-slider"></span>
                            </label>
                        </div>
                    </div>

                    <div class="notification-types">
                        <h3>Типы уведомлений</h3>

                        <div class="notification-type-item">
                            <div class="notification-type-info">
                                <h4>Обновления системы</h4>
                                <p>Уведомления о новых функциях и обновлениях</p>
                            </div>
                            <label class="toggle-switch">
                                <input type="checkbox" checked on:change={saveNotificationSettings}>
                                <span class="switch-slider"></span>
                            </label>
                        </div>

                        <div class="notification-type-item">
                            <div class="notification-type-info">
                                <h4>Безопасность</h4>
                                <p>Уведомления о входах в аккаунт и изменении параметров безопасности</p>
                            </div>
                            <label class="toggle-switch">
                                <input type="checkbox" checked on:change={saveNotificationSettings}>
                                <span class="switch-slider"></span>
                            </label>
                        </div>
                    </div>
                </div>
            {/if}

            <!-- Информация о профиле -->
            {#if activeTab === 'profile'}
                <div class="settings-section" transition:fade={{ duration: 200 }}>
                    <h2>Информация о профиле</h2>

                    <div class="profile-card">
                        <div class="profile-header">
                            <div class="profile-avatar">
                                <div class="avatar-circle">
                                    {userEmail.charAt(0).toUpperCase()}
                                </div>
                            </div>
                            <div class="profile-info">
                                <h3>{userEmail}</h3>
                                <p>Учетная запись</p>
                            </div>
                        </div>

                        <div class="profile-details">
                            <div class="detail-item">
                                <span class="detail-label">Email</span>
                                <span class="detail-value">{userEmail}</span>
                            </div>

                            <div class="detail-item">
                                <span class="detail-label">Роли</span>
                                <div class="detail-value roles-list">
                                    {#if $userRoles && $userRoles.length > 0}
                                        {#each $userRoles as role}
                                            <span class="role-tag">{role.role_name}</span>
                                        {/each}
                                    {:else}
                                        <span class="role-tag standard-role">Стандартный пользователь</span>
                                    {/if}
                                </div>
                            </div>

                            <div class="detail-item">
                                <span class="detail-label">Статус аккаунта</span>
                                <div class="detail-value account-status">
                                    <span class="status-indicator active"></span>
                                    <span>Активен</span>
                                </div>
                            </div>
                        </div>
                    </div>

                    <div class="profile-actions">
                        <button class="edit-profile-button">
                            <i class="material-icons">edit</i>
                            <span>Редактировать профиль</span>
                        </button>

                        <button class="upload-avatar-button">
                            <i class="material-icons">add_a_photo</i>
                            <span>Загрузить фото</span>
                        </button>
                    </div>
                </div>
            {/if}
        </div>
    </div>
</div>

<style>
    /* Импорт шрифтов */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap');

    /* Темы */
    .light-theme {
        --bg-primary: #ffffff;
        --bg-secondary: #f5f8fa;
        --bg-tertiary: #eef2f6;
        --text-primary: #202124;
        --text-secondary: #5f6368;
        --accent-color: #1a73e8;
        --accent-hover: #1765cc;
        --accent-light: rgba(26, 115, 232, 0.1);
        --border-color: #dadce0;
        --hover-bg: #f1f3f4;
        --shadow-color: rgba(0, 0, 0, 0.1);
        --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.05);
        --shadow-md: 0 4px 6px rgba(0, 0, 0, 0.05), 0 1px 3px rgba(0, 0, 0, 0.1);
        --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.05), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
        --success-color: #0f9d58;
        --success-bg: rgba(15, 157, 88, 0.1);
        --error-color: #d93025;
        --error-bg: rgba(217, 48, 37, 0.1);
        --card-border-radius: 16px;
        --button-border-radius: 8px;
        --input-border-radius: 8px;
        --transition-all: all 0.3s ease;
    }

    /* Переменные для темной темы */
    .dark-theme {
        --bg-primary: #202124;
        --bg-secondary: #2d2e31;
        --bg-tertiary: #3c3d40;
        --text-primary: #e8eaed;
        --text-secondary: #9aa0a6;
        --accent-color: #8ab4f8;
        --accent-hover: #aecbfa;
        --accent-light: rgba(138, 180, 248, 0.15);
        --border-color: #5f6368;
        --hover-bg: #3c4043;
        --shadow-color: rgba(0, 0, 0, 0.3);
        --shadow-sm: 0 1px 2px rgba(0, 0, 0, 0.2);
        --shadow-md: 0 4px 6px rgba(0, 0, 0, 0.2), 0 1px 3px rgba(0, 0, 0, 0.3);
        --shadow-lg: 0 10px 15px -3px rgba(0, 0, 0, 0.3), 0 4px 6px -2px rgba(0, 0, 0, 0.2);
        --success-color: #81c995;
        --success-bg: rgba(129, 201, 149, 0.15);
        --error-color: #f28b82;
        --error-bg: rgba(242, 139, 130, 0.15);
        --card-border-radius: 16px;
        --button-border-radius: 8px;
        --input-border-radius: 8px;
        --transition-all: all 0.3s ease;
    }

    /* Основные стили */
    :global(body) {
        margin: 0;
        padding: 0;
        font-family: 'Roboto', sans-serif;
        transition: background-color 0.3s ease;
    }

    /* Основной контейнер */
    .settings-container {
        font-family: 'Roboto', sans-serif;
        color: var(--text-primary);
        background-color: var(--bg-secondary);
        min-height: 100vh;
        width: 100%;
        display: flex;
        flex-direction: column;
        transition: var(--transition-all);
    }

    /* Заголовок */
    .settings-header {
        display: flex;
        align-items: center;
        padding: 16px 24px;
        background-color: var(--bg-primary);
        border-bottom: 1px solid var(--border-color);
        position: sticky;
        top: 0;
        z-index: 100;
    }

    .settings-header h1 {
        margin: 0;
        font-size: 24px;
        font-weight: 600;
        color: var(--accent-color);
        flex: 1;
        text-align: center;
    }

    .back-button {
        display: flex;
        align-items: center;
        background: transparent;
        border: none;
        color: var(--text-primary);
        padding: 10px 16px;
        border-radius: var(--button-border-radius);
        cursor: pointer;
        font-size: 16px;
        font-weight: 500;
        transition: var(--transition-all);
    }

    .back-button:hover {
        background-color: var(--hover-bg);
    }

    .back-button i {
        margin-right: 8px;
    }

    .mobile-menu-toggle {
        display: none;
        background: var(--bg-secondary);
        border: none;
        color: var(--text-primary);
        height: 40px;
        width: 40px;
        border-radius: 50%;
        cursor: pointer;
        box-shadow: var(--shadow-sm);
        transition: var(--transition-all);
    }

    .mobile-menu-toggle:hover {
        background-color: var(--hover-bg);
        transform: translateY(-1px);
        box-shadow: var(--shadow-md);
    }

    /* Сообщения */
    .message {
        position: fixed;
        top: 80px;
        left: 50%;
        transform: translateX(-50%);
        z-index: 1000;
        border-radius: var(--card-border-radius);
        box-shadow: var(--shadow-md);
        backdrop-filter: blur(8px);
        padding: 4px;
        max-width: 400px;
        width: 90%;
    }

    .message-content {
        display: flex;
        align-items: center;
        padding: 12px 20px;
        border-radius: calc(var(--card-border-radius) - 2px);
        font-size: 15px;
        font-weight: 500;
    }

    .message i {
        margin-right: 12px;
        font-size: 20px;
    }

    .success-message {
        background: var(--success-bg);
    }

    .success-message .message-content {
        background: var(--success-bg);
        color: var(--success-color);
    }

    .error-message {
        background: var(--error-bg);
    }

    .error-message .message-content {
        background: var(--error-bg);
        color: var(--error-color);
    }

    /* Расположение элементов */
    .settings-layout {
        display: flex;
        flex: 1;
    }

    /* Навигация */
    .settings-nav {
        width: 280px;
        background-color: var(--bg-secondary);
        padding: 24px 0;
        z-index: 90;
        transition: var(--transition-all);
    }

    .nav-items {
        display: flex;
        flex-direction: column;
        gap: 8px;
        padding: 0 16px;
    }

    .nav-item {
        display: flex;
        align-items: center;
        padding: 14px 20px;
        border: none;
        background: none;
        font-size: 15px;
        color: var(--text-secondary);
        text-align: left;
        cursor: pointer;
        transition: var(--transition-all);
        border-radius: 8px;
        font-weight: 500;
    }

    .nav-item:hover {
        background-color: var(--hover-bg);
        color: var(--text-primary);
    }

    .nav-item.active {
        color: var(--accent-color);
        background-color: var(--accent-light);
        font-weight: 600;
    }

    .nav-item.active {
        background-color: var(--accent-color);
        color: white;
    }

    .nav-item i {
        font-size: 20px;
        margin-right: 15px;
        color: var(--accent-color);
    }

    .nav-item.active i {
        color: var(--accent-color);
    }

    /* Содержимое настроек */
    .settings-content {
        flex: 1;
        padding: 30px;
        transition: opacity 0.3s ease;
    }

    .settings-content.animating {
        opacity: 0.5;
    }

    .settings-section {
        background-color: var(--bg-primary);
        border-radius: 16px;
        box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
        padding: 30px;
        margin-bottom: 24px;
        transition: var(--transition-all);
    }

    .settings-section h2 {
        margin-top: 0;
        margin-bottom: 30px;
        font-size: 22px;
        font-weight: 600;
        color: var(--text-primary);
        padding-bottom: 16px;
        border-bottom: 1px solid var(--border-color);
    }

    /* Элементы настроек */
    .setting-item {
        display: flex;
        align-items: flex-start;
        justify-content: space-between;
        padding: 20px 0;
        border-bottom: 1px solid var(--border-color);
    }

    .setting-item:last-child {
        border-bottom: none;
    }

    .setting-info {
        flex: 1;
        margin-right: 20px;
    }

    .setting-info h3 {
        margin: 0 0 10px 0;
        font-size: 17px;
        font-weight: 600;
        color: var(--text-primary);
    }

    .setting-info p {
        margin: 0;
        font-size: 14px;
        color: var(--text-secondary);
        line-height: 1.5;
    }

    .setting-control {
        display: flex;
        align-items: center;
        min-width: 150px;
    }

    /* Кнопки тем и вид просмотра */
    .theme-control, .view-control {
        display: flex;
        gap: 10px;
    }

    .theme-button, .view-button {
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 10px 16px;
        border: 1px solid var(--border-color);
        background-color: var(--bg-primary);
        color: var(--text-secondary);
        border-radius: 50px;
        cursor: pointer;
        transition: var(--transition-all);
        font-size: 14px;
        font-weight: 500;
    }

    .theme-button:hover, .view-button:hover {
        background-color: var(--hover-bg);
    }

    .theme-button.active, .view-button.active {
        background-color: var(--accent-color);
        color: white;
        border-color: var(--accent-color);
    }

    .theme-button i, .view-button i {
        margin-right: 8px;
        font-size: 18px;
    }

    /* Выбор языка */
    .select-wrapper {
        position: relative;
        width: 100%;
    }

    .language-select {
        width: 100%;
        padding: 12px 16px;
        padding-right: 40px;
        border: 1px solid var(--border-color);
        border-radius: var(--input-border-radius);
        background-color: var(--bg-primary);
        color: var(--text-primary);
        font-size: 14px;
        font-weight: 500;
        appearance: none;
        cursor: pointer;
        transition: var(--transition-all);
    }

    .language-select:focus {
        outline: none;
        border-color: var(--accent-color);
    }

    .select-icon {
        position: absolute;
        right: 12px;
        top: 50%;
        transform: translateY(-50%);
        color: var(--text-secondary);
        pointer-events: none;
    }

    /* Переключатель */
    .toggle-switch {
        position: relative;
        display: inline-block;
        width: 52px;
        height: 28px;
    }

    .toggle-switch input {
        opacity: 0;
        width: 0;
        height: 0;
    }

    .switch-slider {
        position: absolute;
        cursor: pointer;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: var(--bg-tertiary);
        transition: var(--transition-all);
        border-radius: 34px;
    }

    .switch-slider:before {
        position: absolute;
        content: "";
        height: 22px;
        width: 22px;
        left: 3px;
        bottom: 3px;
        background-color: var(--text-secondary);
        transition: var(--transition-all);
        border-radius: 50%;
    }

    input:checked + .switch-slider {
        background-color: var(--accent-color);
    }

    input:checked + .switch-slider:before {
        transform: translateX(24px);
        background-color: white;
    }

    /* Форма смены пароля */
    .password-change {
        flex-direction: column;
    }

    .password-form {
        margin-top: 20px;
        width: 100%;
    }

    .form-group {
        margin-bottom: 20px;
    }

    .form-group label {
        display: block;
        margin-bottom: 8px;
        font-size: 14px;
        font-weight: 500;
        color: var(--text-secondary);
    }

    .input-wrapper {
        position: relative;
        width: 100%;
    }

    .input-wrapper input {
        width: 100%;
        padding: 14px 16px;
        padding-left: 48px;
        border: 1px solid var(--border-color);
        border-radius: var(--input-border-radius);
        background-color: var(--bg-tertiary);
        color: var(--text-primary);
        font-size: 15px;
        transition: var(--transition-all);
    }

    .input-wrapper input:focus {
        outline: none;
        border-color: var(--accent-color);
        box-shadow: 0 0 0 3px var(--accent-light);
    }

    .input-wrapper input::placeholder {
        color: var(--text-tertiary);
    }

    .input-icon {
        position: absolute;
        left: 16px;
        top: 50%;
        transform: translateY(-50%);
        color: var(--text-tertiary);
    }

    .password-message {
        position: static;
        transform: none;
        left: auto;
        top: auto;
        width: 100%;
        max-width: 100%;
        margin-bottom: 20px;
        box-shadow: none;
    }

    .change-password-button, .two-factor-button, .sessions-button, .edit-profile-button, .upload-avatar-button {
        display: flex;
        align-items: center;
        justify-content: center;
        padding: 14px 24px;
        background: var(--accent-gradient);
        color: black;
        border: none;
        border-radius: var(--button-border-radius);
        font-size: 15px;
        font-weight: 600;
        cursor: pointer;
        transition: var(--transition-all);
        box-shadow: var(--shadow-sm);
    }

    .change-password-button:hover, .two-factor-button:hover, .sessions-button:hover, .edit-profile-button:hover, .upload-avatar-button:hover {
        transform: translateY(-2px);
        box-shadow: var(--shadow-md);
    }

    .change-password-button i, .two-factor-button i, .sessions-button i, .edit-profile-button i, .upload-avatar-button i {
        margin-right: 10px;
    }

    /* Типы уведомлений */
    .notification-types {
        margin-top: 30px;
        padding-top: 20px;
        border-top: 1px solid var(--border-color);
    }

    .notification-types h3 {
        margin: 0 0 20px 0;
        font-size: 17px;
        font-weight: 600;
        color: var(--text-primary);
    }

    .notification-type-item {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 16px;
        background-color: var(--bg-tertiary);
        border-radius: var(--card-border-radius);
        margin-bottom: 12px;
        transition: var(--transition-all);
    }

    .notification-type-item:hover {
        background-color: var(--hover-bg);
    }

    .notification-type-info {
        flex: 1;
    }

    .notification-type-info h4 {
        margin: 0 0 6px 0;
        font-size: 15px;
        font-weight: 500;
        color: var(--text-primary);
    }

    .notification-type-info p {
        margin: 0;
        font-size: 13px;
        color: var(--text-secondary);
    }

    /* Профиль */
    .profile-card {
        background-color: var(--bg-tertiary);
        border-radius: var(--card-border-radius);
        overflow: hidden;
        transition: var(--transition-all);
    }

    .profile-header {
        display: flex;
        align-items: center;
        padding: 24px;
        background: var(--accent-gradient);
        color: white;
    }

    .profile-avatar {
        margin-right: 20px;
    }

    .avatar-circle {
        width: 70px;
        height: 70px;
        border-radius: 50%;
        background-color: white;
        color: var(--accent-color);
        display: flex;
        align-items: center;
        justify-content: center;
        font-size: 28px;
        font-weight: 600;
        box-shadow: var(--shadow-md);
    }

    .profile-info h3 {
        margin: 0 0 4px 0;
        font-size: 20px;
        font-weight: 600;
    }

    .profile-info p {
        margin: 0;
        font-size: 14px;
        opacity: 0.8;
    }

    .profile-details {
        padding: 24px;
    }

    .detail-item {
        margin-bottom: 20px;
        display: flex;
        flex-direction: column;
    }

    .detail-item:last-child {
        margin-bottom: 0;
    }

    .detail-label {
        font-size: 13px;
        color: var(--text-tertiary);
        margin-bottom: 6px;
    }

    .detail-value {
        font-size: 16px;
        color: var(--text-primary);
        font-weight: 500;
    }

    .roles-list {
        display: flex;
        flex-wrap: wrap;
        gap: 8px;
    }

    .role-tag {
        display: inline-block;
        padding: 6px 12px;
        background-color: var(--accent-light);
        color: var(--accent-color);
        border-radius: 30px;
        font-size: 13px;
        font-weight: 600;
    }

    .standard-role {
        background-color: rgba(128, 128, 128, 0.1);
        color: var(--text-secondary);
    }

    .account-status {
        display: flex;
        align-items: center;
    }

    .status-indicator {
        width: 10px;
        height: 10px;
        border-radius: 50%;
        margin-right: 8px;
    }

    .status-indicator.active {
        background-color: var(--success-color);
        box-shadow: 0 0 0 3px var(--success-bg);
    }

    .profile-actions {
        display: flex;
        gap: 12px;
        margin-top: 24px;
    }

    .profile-actions button {
        flex: 1;
    }

    /* Адаптивный дизайн */
    @media (max-width: 960px) {
        .settings-layout {
            flex-direction: column;
        }

        .settings-nav {
            width: 100%;
            border-right: none;
            border-bottom: 1px solid var(--border-color);
            padding: 16px 0;
        }

        .nav-items {
            flex-direction: row;
            justify-content: space-around;
            gap: 0;
            padding: 0;
        }

        .nav-item {
            flex-direction: column;
            padding: 12px;
            border-radius: 8px;
            text-align: center;
            flex: 1;
        }

        .setting-item {
            flex-direction: column;
        }

        .setting-info {
            margin-right: 0;
            margin-bottom: 16px;
            width: 100%;
        }

        .setting-control {
            width: 100%;
        }

        .theme-control, .view-control {
            width: 100%;
        }
    }

    @media (max-width: 768px) {
        .settings-nav {
            position: fixed;
            top: 70px;
            left: 0;
            width: 250px;
            height: 100%;
            transform: translateX(-100%);
            z-index: 99;
            transition: transform 0.3s ease-in-out;
        }

        .settings-nav.mobile-open {
            transform: translateX(0);
        }

        .nav-items {
            flex-direction: column;
            padding: 16px;
            gap: 8px;
        }

        .nav-item {
            flex-direction: row;
            text-align: left;
        }

        .mobile-menu-toggle {
            display: flex;
            align-items: center;
            justify-content: center;
        }

        .settings-content {
            padding: 20px;
        }

        .settings-section {
            padding: 20px;
        }

        .profile-header {
            flex-direction: column;
            align-items: center;
            text-align: center;
        }

        .profile-avatar {
            margin-right: 0;
            margin-bottom: 16px;
        }

        .profile-actions {
            flex-direction: column;
        }
    }

    @media (max-width: 480px) {
        .settings-header h1 {
            font-size: 20px;
        }

        .back-button span {
            display: none;
        }

        .back-button i {
            margin-right: 0;
        }

        .theme-button span, .view-button span {
            display: none;
        }

        .theme-button i, .view-button i {
            margin-right: 0;
        }
    }

    /* Анимации и переходы */
    @keyframes fadeIn {
        from { opacity: 0; }
        to { opacity: 1; }
    }

    @keyframes slideUp {
        from { transform: translateY(20px); opacity: 0; }
        to { transform: translateY(0); opacity: 1; }
    }

    @keyframes pulse {
        0% { box-shadow: 0 0 0 0 rgba(67, 97, 238, 0.7); }
        70% { box-shadow: 0 0 0 10px rgba(67, 97, 238, 0); }
        100% { box-shadow: 0 0 0 0 rgba(67, 97, 238, 0); }
    }

    /* Дополнительные стили для кнопок и входов */
    input::placeholder, select, button {
        font-family: 'Roboto', sans-serif;
    }

    button {
        display: inline-flex;
        align-items: center;
        justify-content: center;
    }

    /* Дополнительный скролл */
    ::-webkit-scrollbar {
        width: 8px;
        height: 8px;
    }

    ::-webkit-scrollbar-track {
        background: var(--bg-secondary);
    }

    ::-webkit-scrollbar-thumb {
        background: var(--text-tertiary);
        border-radius: 4px;
    }

    ::-webkit-scrollbar-thumb:hover {
        background: var(--text-secondary);
    }

    /* Утилиты */
    @keyframes rotate {
        from { transform: rotate(0deg); }
        to { transform: rotate(360deg); }
    }

    @keyframes ripple {
        0% {
            transform: scale(0);
            opacity: 1;
        }
        100% {
            transform: scale(1.5);
            opacity: 0;
        }
    }
</style>
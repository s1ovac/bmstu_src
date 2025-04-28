// src/lib/localization.js
import { writable, derived } from 'svelte/store';

// Create a store for the active language
export const language = writable(localStorage.getItem('language') || 'ru');

// Watch for changes to the language store and update localStorage
language.subscribe(value => {
    if (typeof window !== 'undefined') {
        localStorage.setItem('language', value);
    }
});

// Translations dictionary
const translations = {
    // Navigation and common elements
    'My Drive': {
        'ru': 'Мой диск',
        'en': 'My Drive'
    },
    'Favorites': {
        'ru': 'Избранное',
        'en': 'Favorites'
    },
    'Recent': {
        'ru': 'Недавние',
        'en': 'Recent'
    },
    'Trash': {
        'ru': 'Корзина',
        'en': 'Trash'
    },
    'Administration': {
        'ru': 'Администрирование',
        'en': 'Administration'
    },
    'Admin Panel': {
        'ru': 'Панель управления',
        'en': 'Admin Panel'
    },
    'Users': {
        'ru': 'Пользователи',
        'en': 'Users'
    },
    'Settings': {
        'ru': 'Настройки',
        'en': 'Settings'
    },
    'Light Theme': {
        'ru': 'Светлая тема',
        'en': 'Light Theme'
    },
    'Dark Theme': {
        'ru': 'Темная тема',
        'en': 'Dark Theme'
    },
    'Logout': {
        'ru': 'Выйти',
        'en': 'Logout'
    },
    'Home': {
        'ru': 'На главную',
        'en': 'Home'
    },
    'Back': {
        'ru': 'Назад',
        'en': 'Back'
    },

    // File operations
    'Upload Files': {
        'ru': 'Загрузить файлы',
        'en': 'Upload Files'
    },
    'Create Folder': {
        'ru': 'Создать папку',
        'en': 'Create Folder'
    },
    'Delete': {
        'ru': 'Удалить',
        'en': 'Delete'
    },
    'Move': {
        'ru': 'Переместить',
        'en': 'Move'
    },
    'Download': {
        'ru': 'Скачать',
        'en': 'Download'
    },
    'Search': {
        'ru': 'Поиск',
        'en': 'Search'
    },
    'Select All': {
        'ru': 'Выбрать все',
        'en': 'Select All'
    },
    'Deselect': {
        'ru': 'Снять выделение',
        'en': 'Deselect'
    },
    'This folder is empty': {
        'ru': 'Эта папка пуста',
        'en': 'This folder is empty'
    },
    'No results found for': {
        'ru': 'По запросу',
        'en': 'No results found for'
    },
    'nothing found': {
        'ru': 'ничего не найдено',
        'en': 'nothing found'
    },
    'Drag files here or use the "Upload Files" button': {
        'ru': 'Перетащите файлы сюда или воспользуйтесь кнопкой "Загрузить файлы"',
        'en': 'Drag files here or use the "Upload Files" button'
    },
    'File uploaded successfully': {
        'ru': 'Файл успешно загружен',
        'en': 'File uploaded successfully'
    },
    'Files uploaded successfully': {
        'ru': 'Файлы успешно загружены',
        'en': 'Files uploaded successfully'
    },
    'Folder created successfully': {
        'ru': 'Папка создана',
        'en': 'Folder created successfully'
    },
    'Enter folder name': {
        'ru': 'Введите название папки',
        'en': 'Enter folder name'
    },
    'Folder name cannot be empty': {
        'ru': 'Имя папки не может быть пустым',
        'en': 'Folder name cannot be empty'
    },
    'Loading...': {
        'ru': 'Загрузка...',
        'en': 'Loading...'
    },
    'Failed to upload file': {
        'ru': 'Не удалось загрузить файл',
        'en': 'Failed to upload file'
    },
    'Delete selected items?': {
        'ru': 'Удалить выбранные элементы?',
        'en': 'Delete selected items?'
    },
    'This action cannot be undone!': {
        'ru': 'Это действие нельзя отменить!',
        'en': 'This action cannot be undone!'
    },
    'Cancel': {
        'ru': 'Отмена',
        'en': 'Cancel'
    },
    'Create': {
        'ru': 'Создать',
        'en': 'Create'
    },
    'OK': {
        'ru': 'ОК',
        'en': 'OK'
    },
    'Confirm': {
        'ru': 'Подтвердить',
        'en': 'Confirm'
    },

    // Login and authentication
    'Login': {
        'ru': 'Вход в систему',
        'en': 'Login'
    },
    'Sign In': {
        'ru': 'Войти',
        'en': 'Sign In'
    },
    'Signing in...': {
        'ru': 'Вход...',
        'en': 'Signing in...'
    },
    'Register': {
        'ru': 'Регистрация',
        'en': 'Register'
    },
    'Create Account': {
        'ru': 'Создать аккаунт',
        'en': 'Create Account'
    },
    'Password': {
        'ru': 'Пароль',
        'en': 'Password'
    },
    'Confirm Password': {
        'ru': 'Подтверждение пароля',
        'en': 'Confirm Password'
    },
    'Enter your email': {
        'ru': 'Введите ваш email',
        'en': 'Enter your email'
    },
    'Enter your password': {
        'ru': 'Введите ваш пароль',
        'en': 'Enter your password'
    },
    'Confirm your password': {
        'ru': 'Подтвердите ваш пароль',
        'en': 'Confirm your password'
    },
    'Please enter email': {
        'ru': 'Пожалуйста, введите email',
        'en': 'Please enter email'
    },
    'Please enter a valid email': {
        'ru': 'Пожалуйста, введите корректный email',
        'en': 'Please enter a valid email'
    },
    'Please enter password': {
        'ru': 'Пожалуйста, введите пароль',
        'en': 'Please enter password'
    },
    'Password must be at least 6 characters': {
        'ru': 'Пароль должен содержать не менее 6 символов',
        'en': 'Password must be at least 6 characters'
    },
    'Passwords do not match': {
        'ru': 'Пароли не совпадают',
        'en': 'Passwords do not match'
    },
    'Invalid email or password': {
        'ru': 'Неверный email или пароль',
        'en': 'Invalid email or password'
    },
    'User already exists': {
        'ru': 'Пользователь с таким email уже существует',
        'en': 'User already exists'
    },
    'User not found': {
        'ru': 'Пользователь с таким email не найден',
        'en': 'User not found'
    },
    'Forgot Password?': {
        'ru': 'Забыли пароль?',
        'en': 'Forgot Password?'
    },
    'Don\'t have an account?': {
        'ru': 'Нет аккаунта?',
        'en': 'Don\'t have an account?'
    },
    'Already have an account?': {
        'ru': 'Уже есть аккаунт?',
        'en': 'Already have an account?'
    },
    'Minimum 6 characters': {
        'ru': 'Минимум 6 символов',
        'en': 'Minimum 6 characters'
    },

    // Settings page
    'Appearance Settings': {
        'ru': 'Настройки внешнего вида',
        'en': 'Appearance Settings'
    },
    'Security Settings': {
        'ru': 'Настройки безопасности',
        'en': 'Security Settings'
    },
    'Notification Settings': {
        'ru': 'Настройки уведомлений',
        'en': 'Notification Settings'
    },
    'Profile': {
        'ru': 'Профиль',
        'en': 'Profile'
    },
    'Theme': {
        'ru': 'Тема',
        'en': 'Theme'
    },
    'Choose light or dark theme': {
        'ru': 'Выберите светлую или темную тему оформления',
        'en': 'Choose light or dark theme'
    },
    'Light': {
        'ru': 'Светлая',
        'en': 'Light'
    },
    'Dark': {
        'ru': 'Темная',
        'en': 'Dark'
    },
    'Language': {
        'ru': 'Язык',
        'en': 'Language'
    },
    'Choose interface language': {
        'ru': 'Выберите язык интерфейса',
        'en': 'Choose interface language'
    },
    'English': {
        'ru': 'English',
        'en': 'English'
    },
    'Russian': {
        'ru': 'Русский',
        'en': 'Russian'
    },
    'German': {
        'ru': 'Deutsch',
        'en': 'German'
    },
    'French': {
        'ru': 'Français',
        'en': 'French'
    },
    'Default File View': {
        'ru': 'Вид файлов по умолчанию',
        'en': 'Default File View'
    },
    'Choose default file display mode': {
        'ru': 'Выберите режим отображения файлов',
        'en': 'Choose default file display mode'
    },
    'Grid': {
        'ru': 'Сетка',
        'en': 'Grid'
    },
    'List': {
        'ru': 'Список',
        'en': 'List'
    },
    'Change Password': {
        'ru': 'Изменить пароль',
        'en': 'Change Password'
    },
    'Enter current password and new password twice to change': {
        'ru': 'Для смены пароля введите текущий пароль и новый пароль дважды',
        'en': 'Enter current password and new password twice to change'
    },
    'Current Password': {
        'ru': 'Текущий пароль',
        'en': 'Current Password'
    },
    'New Password': {
        'ru': 'Новый пароль',
        'en': 'New Password'
    },
    'Confirm New Password': {
        'ru': 'Подтверждение пароля',
        'en': 'Confirm New Password'
    },
    'Save': {
        'ru': 'Сохранить',
        'en': 'Save'
    },
    'Password changed successfully': {
        'ru': 'Пароль успешно изменен',
        'en': 'Password changed successfully'
    },
    'Incorrect current password': {
        'ru': 'Неверный текущий пароль',
        'en': 'Incorrect current password'
    },
    'Email Notifications': {
        'ru': 'Уведомления по электронной почте',
        'en': 'Email Notifications'
    },
    'Receive important event notifications via email': {
        'ru': 'Получайте уведомления о важных событиях на вашу электронную почту',
        'en': 'Receive important event notifications via email'
    },
    'Browser Notifications': {
        'ru': 'Уведомления в браузере',
        'en': 'Browser Notifications'
    },
    'Receive application notifications in browser': {
        'ru': 'Получайте уведомления от приложения в браузере',
        'en': 'Receive application notifications in browser'
    },
    'Sound Notifications': {
        'ru': 'Звуковые уведомления',
        'en': 'Sound Notifications'
    },
    'Enable sound for notifications': {
        'ru': 'Включить звуковое сопровождение уведомлений',
        'en': 'Enable sound for notifications'
    },
    'Notification Types': {
        'ru': 'Типы уведомлений',
        'en': 'Notification Types'
    },
    'System Updates': {
        'ru': 'Обновления системы',
        'en': 'System Updates'
    },
    'Notifications about new features and updates': {
        'ru': 'Уведомления о новых функциях и обновлениях',
        'en': 'Notifications about new features and updates'
    },
    'Security': {
        'ru': 'Безопасность',
        'en': 'Security'
    },
    'Notifications about login attempts and security changes': {
        'ru': 'Уведомления о входах в аккаунт и изменении параметров безопасности',
        'en': 'Notifications about login attempts and security changes'
    },
    'Account': {
        'ru': 'Учетная запись',
        'en': 'Account'
    },
    'Account Status': {
        'ru': 'Статус аккаунта',
        'en': 'Account Status'
    },
    'Active': {
        'ru': 'Активен',
        'en': 'Active'
    },
    'Edit Profile': {
        'ru': 'Редактировать профиль',
        'en': 'Edit Profile'
    },
    'Upload Photo': {
        'ru': 'Загрузить фото',
        'en': 'Upload Photo'
    },
    'Two-Factor Authentication': {
        'ru': 'Двухфакторная аутентификация',
        'en': 'Two-Factor Authentication'
    },
    'Enhance your account security with two-factor authentication': {
        'ru': 'Повысьте безопасность вашего аккаунта с помощью двухфакторной аутентификации',
        'en': 'Enhance your account security with two-factor authentication'
    },
    'Set up 2FA': {
        'ru': 'Настроить 2FA',
        'en': 'Set up 2FA'
    },
    'Active Sessions': {
        'ru': 'Активные сессии',
        'en': 'Active Sessions'
    },
    'View and manage active sessions': {
        'ru': 'Просмотр и управление активными сессиями',
        'en': 'View and manage active sessions'
    },
    'Manage Sessions': {
        'ru': 'Управление сессиями',
        'en': 'Manage Sessions'
    },
    'Settings saved': {
        'ru': 'Настройки сохранены',
        'en': 'Settings saved'
    },
    'Theme changed successfully': {
        'ru': 'Тема успешно изменена',
        'en': 'Theme changed successfully'
    },
    'View mode changed successfully': {
        'ru': 'Режим просмотра успешно изменен',
        'en': 'View mode changed successfully'
    },
    'Language changed successfully': {
        'ru': 'Язык успешно изменен',
        'en': 'Language changed successfully'
    },
    'Notification settings saved': {
        'ru': 'Настройки уведомлений сохранены',
        'en': 'Notification settings saved'
    },
    'Standard User': {
        'ru': 'Стандартный пользователь',
        'en': 'Standard User'
    },

    // Admin dashboard
    'Admin Dashboard': {
        'ru': 'Панель администратора',
        'en': 'Admin Dashboard'
    },
    'System Users': {
        'ru': 'Пользователи системы',
        'en': 'System Users'
    },
    'No users in system': {
        'ru': 'Нет пользователей в системе',
        'en': 'No users in system'
    },
    'ID': {
        'ru': 'ID',
        'en': 'ID'
    },
    'Email': {
        'ru': 'Email',
        'en': 'Email'
    },
    'Roles': {
        'ru': 'Роли',
        'en': 'Roles'
    },
    'Actions': {
        'ru': 'Действия',
        'en': 'Actions'
    },
    'No roles': {
        'ru': 'Нет ролей',
        'en': 'No roles'
    },
    'View Content': {
        'ru': 'Просмотр файлов',
        'en': 'View Content'
    },
    'Statistics': {
        'ru': 'Статистика',
        'en': 'Statistics'
    },
    'System Statistics': {
        'ru': 'Статистика системы',
        'en': 'System Statistics'
    },
    'No statistical data': {
        'ru': 'Нет статистических данных',
        'en': 'No statistical data'
    },
    'Total': {
        'ru': 'Всего',
        'en': 'Total'
    },
    'File Type Distribution': {
        'ru': 'Распределение типов файлов',
        'en': 'File Type Distribution'
    },
    'No data about file types': {
        'ru': 'Нет данных о типах файлов',
        'en': 'No data about file types'
    },
    'Top Users by Storage': {
        'ru': 'Топ пользователей по использованию хранилища',
        'en': 'Top Users by Storage'
    },
    'User': {
        'ru': 'Пользователь',
        'en': 'User'
    },
    'Storage': {
        'ru': 'Объем',
        'en': 'Storage'
    },
    'No data about users': {
        'ru': 'Нет данных о пользователях',
        'en': 'No data about users'
    },
    'User content not found': {
        'ru': 'Данные пользователя не найдены',
        'en': 'User content not found'
    },
    'Folders': {
        'ru': 'Папки',
        'en': 'Folders'
    },
    'Files': {
        'ru': 'Файлы',
        'en': 'Files'
    },
    'No folders': {
        'ru': 'У пользователя нет папок',
        'en': 'No folders'
    },
    'No files': {
        'ru': 'У пользователя нет файлов',
        'en': 'No files'
    },
    'Size': {
        'ru': 'Размер',
        'en': 'Size'
    },
    'Folder': {
        'ru': 'Папка',
        'en': 'Folder'
    },
    'Creation Date': {
        'ru': 'Дата создания',
        'en': 'Creation Date'
    },
    'Root folder': {
        'ru': 'Корневая папка',
        'en': 'Root folder'
    },
    'Name': {
        'ru': 'Название',
        'en': 'Name'
    },
    'You do not have permission to access this page': {
        'ru': 'У вас нет прав доступа к этой странице',
        'en': 'You do not have permission to access this page'
    },
    'You need admin privileges to access the admin panel': {
        'ru': 'Для доступа к панели администратора необходимы соответствующие права',
        'en': 'You need admin privileges to access the admin panel'
    },
    'Go to Home': {
        'ru': 'Перейти на главную',
        'en': 'Go to Home'
    },

    // Groups management
    'Group Name': {
        'ru': 'Название группы',
        'en': 'Group Name'
    },
    'Create Group': {
        'ru': 'Создать группу',
        'en': 'Create Group'
    },
    'Delete Group': {
        'ru': 'Удалить группу',
        'en': 'Delete Group'
    },
    'Rename Group': {
        'ru': 'Переименовать',
        'en': 'Rename Group'
    },
    'Rename to': {
        'ru': 'Переименовать в',
        'en': 'Rename to'
    },
    'New name': {
        'ru': 'Новое название',
        'en': 'New name'
    },
    'Add User to Group': {
        'ru': 'Добавить пользователя в группу',
        'en': 'Add User to Group'
    },
    'Remove User from Group': {
        'ru': 'Удалить пользователя из группы',
        'en': 'Remove User from Group'
    },
    'No groups available': {
        'ru': 'Нет доступных групп',
        'en': 'No groups available'
    },
    'Enter group name': {
        'ru': 'Введите название группы',
        'en': 'Enter group name'
    },
    'Selected group': {
        'ru': 'Выбранная группа',
        'en': 'Selected group'
    },
    'Selected user': {
        'ru': 'Выбранный пользователь',
        'en': 'Selected user'
    },
    'Group not selected': {
        'ru': 'Группа не выбрана',
        'en': 'Group not selected'
    },
    'User not selected': {
        'ru': 'Пользователь не выбран',
        'en': 'User not selected'
    },
    'Confirm deletion': {
        'ru': 'Подтверждение удаления',
        'en': 'Confirm deletion'
    },
    'Are you sure you want to delete the group': {
        'ru': 'Вы уверены, что хотите удалить группу',
        'en': 'Are you sure you want to delete the group'
    },
    'This action cannot be undone': {
        'ru': 'Это действие нельзя отменить',
        'en': 'This action cannot be undone'
    },
    'Group created successfully': {
        'ru': 'Группа успешно создана',
        'en': 'Group created successfully'
    },
    'Group renamed successfully': {
        'ru': 'Группа переименована',
        'en': 'Group renamed successfully'
    },
    'Group deleted successfully': {
        'ru': 'Группа удалена',
        'en': 'Group deleted successfully'
    },
    'User added to group successfully': {
        'ru': 'Пользователь добавлен в группу',
        'en': 'User added to group successfully'
    },
    'User removed from group successfully': {
        'ru': 'Пользователь удалён из группы',
        'en': 'User removed from group successfully'
    },
    'Groups': {
        'ru': 'Группы',
        'en': 'Groups'
    },
    'Management': {
        'ru': 'Управление',
        'en': 'Management'
    },
    'Group Members': {
        'ru': 'Участники группы',
        'en': 'Group Members'
    },

    // Error messages
    'Error': {
        'ru': 'Ошибка',
        'en': 'Error'
    },
    'Authentication error': {
        'ru': 'Ошибка аутентификации',
        'en': 'Authentication error'
    },
    'Server error': {
        'ru': 'Ошибка сервера',
        'en': 'Server error'
    },
    'Network error': {
        'ru': 'Ошибка сети',
        'en': 'Network error'
    },
    'Permission denied': {
        'ru': 'Доступ запрещен',
        'en': 'Permission denied'
    },
    'File not found': {
        'ru': 'Файл не найден',
        'en': 'File not found'
    },
    'Folder not found': {
        'ru': 'Папка не найдена',
        'en': 'Folder not found'
    }
};

// Function to translate text
export function t(key, lang = null) {
    // If no language specified, use the active language from the store
    let activeLang;

    if (lang) {
        activeLang = lang;
    } else {
        // Get current value from the store without subscribing
        // This is safe because we're not in a reactive context
        let currentLang;
        if (typeof window !== 'undefined') { // Check if we're in a browser environment
            activeLang = localStorage.getItem('language') || 'ru';
        } else {
            const unsubscribe = language.subscribe(value => {
                currentLang = value;
            });
            unsubscribe();
            activeLang = currentLang;
        }
    }

    // If the key exists in translations
    if (translations[key]) {
        // Return the translation for the active language, or fallback to English
        return translations[key][activeLang] || translations[key]['en'] || key;
    }

    // If the key doesn't exist, return the key itself
    return key;
}

// Derived store that provides the current translations
export const currentTranslations = derived(
    language,
    ($language) => {
        const result = {};
        for (const [key, translations] of Object.entries(translations)) {
            result[key] = translations[$language] || translations['en'] || key;
        }
        return result;
    }
);
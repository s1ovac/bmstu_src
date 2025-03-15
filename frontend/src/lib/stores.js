import { writable } from 'svelte/store';

// Проверяем, что мы находимся в браузере, перед доступом к localStorage
const isBrowser = typeof window !== 'undefined' && typeof localStorage !== 'undefined';

// Функция, которая создает хранилище с поддержкой localStorage
function persist(key, initialValue) {
    // Если мы не в браузере, просто возвращаем обычное хранилище без localStorage
    const storedValue = isBrowser ? localStorage.getItem(key) : null;
    const data = writable(storedValue ? JSON.parse(storedValue) : initialValue);

    if (isBrowser) {
        // Подписываемся на изменения в хранилище и сохраняем в localStorage
        data.subscribe(value => {
            if (value === undefined || value === null || value === "") {
                localStorage.removeItem(key); // Удаляем, если значение пустое
            } else {
                localStorage.setItem(key, JSON.stringify(value)); // Сохраняем новое значение
            }
        });
    }

    return data;
}

// Экспортируем хранилища с сохранением в localStorage
export const loggedIn = persist("loggedIn", false);
export const email = persist("email", "");
export const userRoles = persist("userRoles", []);
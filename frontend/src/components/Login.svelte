<script>
    import { onMount } from 'svelte';
    import { login } from '../lib/api.js';
    import { loggedIn, email as emailStore } from '../lib/stores.js';

    let email = "";
    let password = "";
    let formError = "";
    let passwordVisible = false;
    let isLoggingIn = false;

    // Check for browser environment to prevent SSR issues
    const isBrowser = typeof window !== 'undefined';

    onMount(() => {
        // Check if already logged in
        if (isBrowser) {
            const storedUser = JSON.parse(localStorage.getItem("user") || "null");
            if (storedUser && storedUser.token) {
                loggedIn.set(true);

                // Redirect to home if already logged in
                window.location.href = '/';
            }
        }
    });

    const togglePasswordVisibility = () => {
        passwordVisible = !passwordVisible;
    };

    const onButtonClick = async () => {
        formError = "";

        if (email === "") {
            formError = "Пожалуйста, введите email";
            return;
        }

        if (!validateEmail(email)) {
            formError = "Пожалуйста, введите корректный email";
            return;
        }

        if (password === "") {
            formError = "Пожалуйста, введите пароль";
            return;
        }

        isLoggingIn = true;

        try {
            console.log("Attempting login with:", { email });
            const response = await login(email, password);
            console.log("Login response:", response);

            if (!response || !response.token) {
                formError = "Неверный ответ сервера аутентификации";
                isLoggingIn = false;
                return;
            }

            // Parse token to get user ID
            let userId = "";
            try {
                const decoded = parseJwt(response.token);
                userId = decoded.sub;
                console.log("Decoded user ID:", userId);
            } catch (e) {
                console.error("Error parsing JWT token:", e);
                // Continue even if token parsing fails
            }

            // Store user data in localStorage
            if (isBrowser) {
                const userData = {
                    token: response.token,
                    user_id: userId
                };

                localStorage.setItem("user", JSON.stringify(userData));
                console.log("User data saved to localStorage");
            }

            // Update stores
            loggedIn.set(true);
            emailStore.set(email);

            console.log("Login successful, redirecting to homepage...");

            // Force redirect to homepage
            window.location.href = '/';

        } catch (error) {
            console.error("Login error:", error);
            if (error.message === "User not found") {
                formError = "Пользователь с таким email не найден";
            } else if (error.message === "Invalid credentials") {
                formError = "Неверный email или пароль";
            } else {
                formError = "Ошибка входа: " + error.message;
            }
            isLoggingIn = false;
        }
    };

    function handleKeyPress(event) {
        if (event.key === 'Enter') {
            onButtonClick();
        }
    }

    function validateEmail(email) {
        const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return re.test(String(email).toLowerCase());
    }

    function parseJwt(token) {
        var base64Url = token.split('.')[1];
        var base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
        var jsonPayload = decodeURIComponent(atob(base64).split('').map(function(c) {
            return '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2);
        }).join(''));

        return JSON.parse(jsonPayload);
    }
</script>

<div class="login-container">
    <div class="login-card">
        <div class="login-header">
            <div class="logo">
                <img src="/bmstu_logo.png" alt="Cloud Logo" class="logo-image" />
                <h1>Cloud Storage</h1>
            </div>
            <h2>Вход в систему</h2>
        </div>

        <div class="login-form">
            <div class="form-group">
                <label for="email">Email</label>
                <div class="input-wrapper">
                    <i class="material-icons">email</i>
                    <input
                            id="email"
                            type="email"
                            bind:value={email}
                            placeholder="Введите ваш email"
                            on:keypress={handleKeyPress}
                            disabled={isLoggingIn}
                    />
                </div>
            </div>

            <div class="form-group">
                <label for="password">Пароль</label>
                <div class="input-wrapper">
                    <i class="material-icons">lock</i>
                    {#if passwordVisible}
                        <input
                                id="password"
                                type="text"
                                bind:value={password}
                                placeholder="Введите ваш пароль"
                                on:keypress={handleKeyPress}
                                disabled={isLoggingIn}
                        />
                    {:else}
                        <input
                                id="password"
                                type="password"
                                bind:value={password}
                                placeholder="Введите ваш пароль"
                                on:keypress={handleKeyPress}
                                disabled={isLoggingIn}
                        />
                    {/if}
                    <button
                            type="button"
                            class="toggle-password"
                            on:click={togglePasswordVisibility}
                            aria-label="Показать/скрыть пароль"
                            disabled={isLoggingIn}
                    >
                        <i class="material-icons">{passwordVisible ? 'visibility_off' : 'visibility'}</i>
                    </button>
                </div>
            </div>

            {#if formError}
                <div class="error-message">
                    <i class="material-icons">error</i>
                    <span>{formError}</span>
                </div>
            {/if}

            <button
                    class="login-button"
                    on:click={onButtonClick}
                    disabled={isLoggingIn}
            >
                {#if isLoggingIn}
                    <div class="spinner"></div>
                    <span>Вход...</span>
                {:else}
                    <i class="material-icons">login</i>
                    <span>Войти</span>
                {/if}
            </button>

            <div class="form-footer">
                <a href="/signup" class="signup-link">Создать аккаунт</a>
                <a href="#" class="forgot-link">Забыли пароль?</a>
            </div>
        </div>
    </div>
</div>

<style>
    /* Import Material Icons and Roboto font */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap');

    /* Container styles */
    .login-container {
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
        background-color: #f8f9fa;
        padding: 20px;
        font-family: 'Roboto', sans-serif;
    }

    /* Card styles */
    .login-card {
        background-color: white;
        border-radius: 16px;
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
        width: 100%;
        max-width: 400px;
        overflow: hidden;
    }

    /* Header styles */
    .login-header {
        padding: 24px;
        text-align: center;
        border-bottom: 1px solid #f1f3f4;
    }

    .logo {
        display: flex;
        flex-direction: column;
        align-items: center;
        margin-bottom: 16px;
    }

    .logo-image {
        width: 60px;
        height: 60px;
        margin-bottom: 8px;
    }

    .login-header h1 {
        font-size: 24px;
        font-weight: 500;
        color: #1a73e8;
        margin: 0 0 8px 0;
    }

    .login-header h2 {
        font-size: 20px;
        font-weight: 400;
        color: #3c4043;
        margin: 0;
    }

    /* Form styles */
    .login-form {
        padding: 24px;
    }

    .form-group {
        margin-bottom: 20px;
    }

    .form-group label {
        display: block;
        font-size: 14px;
        font-weight: 500;
        color: #5f6368;
        margin-bottom: 8px;
    }

    .input-wrapper {
        display: flex;
        align-items: center;
        border: 1px solid #dadce0;
        border-radius: 8px;
        padding: 0 12px;
        transition: border-color 0.2s, box-shadow 0.2s;
    }

    .input-wrapper:focus-within {
        border-color: #1a73e8;
        box-shadow: 0 0 0 2px rgba(26, 115, 232, 0.2);
    }

    .input-wrapper i {
        color: #5f6368;
        margin-right: 12px;
        font-size: 20px;
    }

    .input-wrapper input {
        flex: 1;
        border: none;
        outline: none;
        padding: 14px 0;
        font-size: 16px;
        color: #3c4043;
        background: transparent;
        width: 100%;
    }

    .input-wrapper input:disabled {
        background-color: transparent;
        color: #9aa0a6;
    }

    .input-wrapper input::placeholder {
        color: #9aa0a6;
    }

    .toggle-password {
        background: transparent;
        border: none;
        color: #5f6368;
        cursor: pointer;
        padding: 0;
    }

    .toggle-password:disabled {
        opacity: 0.6;
        cursor: not-allowed;
    }

    /* Error message styles */
    .error-message {
        display: flex;
        align-items: center;
        background-color: #fdeded;
        color: #d93025;
        padding: 12px;
        border-radius: 8px;
        margin-bottom: 20px;
        font-size: 14px;
    }

    .error-message i {
        font-size: 18px;
        margin-right: 8px;
    }

    /* Button styles */
    .login-button {
        display: flex;
        align-items: center;
        justify-content: center;
        width: 100%;
        background-color: #1a73e8;
        color: white;
        border: none;
        border-radius: 8px;
        padding: 14px 24px;
        font-size: 16px;
        font-weight: 500;
        cursor: pointer;
        transition: background-color 0.2s;
        margin-bottom: 20px;
    }

    .login-button:hover:not(:disabled) {
        background-color: #1765cc;
    }

    .login-button:disabled {
        background-color: #1a73e8;
        opacity: 0.7;
        cursor: not-allowed;
    }

    .login-button i {
        margin-right: 8px;
        font-size: 20px;
    }

    /* Loading spinner */
    .spinner {
        width: 20px;
        height: 20px;
        border: 3px solid rgba(255,255,255,0.3);
        border-radius: 50%;
        border-top-color: #fff;
        animation: spin 1s linear infinite;
        margin-right: 8px;
    }

    @keyframes spin {
        to { transform: rotate(360deg); }
    }

    /* Footer styles */
    .form-footer {
        display: flex;
        justify-content: space-between;
        font-size: 14px;
    }

    .signup-link, .forgot-link {
        color: #1a73e8;
        text-decoration: none;
        transition: color 0.2s;
    }

    .signup-link:hover, .forgot-link:hover {
        color: #1765cc;
        text-decoration: underline;
    }

    /* Responsive adjustments */
    @media (max-width: 480px) {
        .login-card {
            max-width: 100%;
            border-radius: 0;
            box-shadow: none;
            height: 100vh;
            display: flex;
            flex-direction: column;
        }

        .login-header {
            padding: 20px;
        }

        .login-form {
            padding: 20px;
            flex: 1;
            display: flex;
            flex-direction: column;
        }

        .form-footer {
            margin-top: auto;
            padding-top: 20px;
        }
    }
</style>
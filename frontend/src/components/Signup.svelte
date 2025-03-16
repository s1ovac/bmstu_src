<script>
    import { onMount } from 'svelte';
    import { signup } from '../lib/api.js';
    import { loggedIn, email as emailStore } from '../lib/stores.js';

    let email = "";
    let password = "";
    let confirmPassword = "";
    let formError = "";
    let passwordVisible = false;
    let confirmPasswordVisible = false;
    let isSigningUp = false;

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

    const toggleConfirmPasswordVisibility = () => {
        confirmPasswordVisible = !confirmPasswordVisible;
    };

    const onButtonClick = async () => {
        formError = "";

        // Form validation
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

        if (password.length < 6) {
            formError = "Пароль должен содержать не менее 6 символов";
            return;
        }

        if (confirmPassword === "") {
            formError = "Пожалуйста, подтвердите пароль";
            return;
        }

        if (password !== confirmPassword) {
            formError = "Пароли не совпадают";
            return;
        }

        isSigningUp = true;

        try {
            console.log("Attempting signup with:", { email });
            const response = await signup(email, password);
            console.log("Signup response:", response);

            // Store email for login page
            emailStore.set(email);

            console.log("Signup successful, redirecting to login page...");

            // Show success message
            formError = "";
            setTimeout(() => {
                // Redirect to login page
                window.location.href = '/login';
            }, 1000);

        } catch (error) {
            console.error("Signup error:", error);
            if (error.message === "User already exists") {
                formError = "Пользователь с таким email уже существует";
            } else {
                formError = "Ошибка регистрации: " + error.message;
            }
            isSigningUp = false;
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
</script>

<div class="signup-container">
    <div class="signup-card">
        <div class="signup-header">
            <div class="logo">
                <img src="/bmstu_logo.png" alt="Cloud Logo" class="logo-image" />
                <h1>Cloud Storage</h1>
            </div>
            <h2>Регистрация</h2>
        </div>

        <div class="signup-form">
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
                            disabled={isSigningUp}
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
                                disabled={isSigningUp}
                        />
                    {:else}
                        <input
                                id="password"
                                type="password"
                                bind:value={password}
                                placeholder="Введите ваш пароль"
                                on:keypress={handleKeyPress}
                                disabled={isSigningUp}
                        />
                    {/if}
                    <button
                            type="button"
                            class="toggle-password"
                            on:click={togglePasswordVisibility}
                            aria-label="Показать/скрыть пароль"
                            disabled={isSigningUp}
                    >
                        <i class="material-icons">{passwordVisible ? 'visibility_off' : 'visibility'}</i>
                    </button>
                </div>
                <p class="password-hint">Минимум 6 символов</p>
            </div>

            <div class="form-group">
                <label for="confirm-password">Подтверждение пароля</label>
                <div class="input-wrapper">
                    <i class="material-icons">lock_outline</i>
                    {#if confirmPasswordVisible}
                        <input
                                id="confirm-password"
                                type="text"
                                bind:value={confirmPassword}
                                placeholder="Подтвердите ваш пароль"
                                on:keypress={handleKeyPress}
                                disabled={isSigningUp}
                        />
                    {:else}
                        <input
                                id="confirm-password"
                                type="password"
                                bind:value={confirmPassword}
                                placeholder="Подтвердите ваш пароль"
                                on:keypress={handleKeyPress}
                                disabled={isSigningUp}
                        />
                    {/if}
                    <button
                            type="button"
                            class="toggle-password"
                            on:click={toggleConfirmPasswordVisibility}
                            aria-label="Показать/скрыть пароль"
                            disabled={isSigningUp}
                    >
                        <i class="material-icons">{confirmPasswordVisible ? 'visibility_off' : 'visibility'}</i>
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
                    class="signup-button"
                    on:click={onButtonClick}
                    disabled={isSigningUp}
            >
                {#if isSigningUp}
                    <div class="spinner"></div>
                    <span>Регистрация...</span>
                {:else}
                    <i class="material-icons">person_add</i>
                    <span>Зарегистрироваться</span>
                {/if}
            </button>

            <div class="form-footer">
                <span>Уже есть аккаунт?</span>
                <a href="/login" class="login-link">Войти</a>
            </div>
        </div>
    </div>
</div>

<style>
    /* Import Material Icons and Roboto font */
    @import url('https://fonts.googleapis.com/icon?family=Material+Icons');
    @import url('https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500;700&display=swap');

    /* Container styles */
    .signup-container {
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
        background-color: #f8f9fa;
        padding: 20px;
        font-family: 'Roboto', sans-serif;
    }

    /* Card styles */
    .signup-card {
        background-color: white;
        border-radius: 16px;
        box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
        width: 100%;
        max-width: 400px;
        overflow: hidden;
    }

    /* Header styles */
    .signup-header {
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

    .signup-header h1 {
        font-size: 24px;
        font-weight: 500;
        color: #1a73e8;
        margin: 0 0 8px 0;
    }

    .signup-header h2 {
        font-size: 20px;
        font-weight: 400;
        color: #3c4043;
        margin: 0;
    }

    /* Form styles */
    .signup-form {
        padding: 24px;
    }

    .form-group {
        margin-bottom: 20px;
        position: relative;
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

    .password-hint {
        margin: 6px 0 0 0;
        font-size: 12px;
        color: #5f6368;
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
    .signup-button {
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

    .signup-button:hover:not(:disabled) {
        background-color: #1765cc;
    }

    .signup-button:disabled {
        background-color: #1a73e8;
        opacity: 0.7;
        cursor: not-allowed;
    }

    .signup-button i {
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
        text-align: center;
        font-size: 14px;
        color: #5f6368;
    }

    .login-link {
        color: #1a73e8;
        text-decoration: none;
        margin-left: 8px;
        font-weight: 500;
        transition: color 0.2s;
    }

    .login-link:hover {
        color: #1765cc;
        text-decoration: underline;
    }

    /* Responsive adjustments */
    @media (max-width: 480px) {
        .signup-card {
            max-width: 100%;
            border-radius: 0;
            box-shadow: none;
            min-height: 100vh;
            display: flex;
            flex-direction: column;
        }

        .signup-header {
            padding: 20px;
        }

        .signup-form {
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
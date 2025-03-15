<script>
    import { writable } from 'svelte/store';
    import { login } from '../lib/api.js';
    import { loggedIn, email as emailStore } from '../lib/stores.js';
    import { validateEmail, parseJwt} from '../lib/utils.js';

    const email = writable("");
    const password = writable("");
    const formError = writable(""); // Одна переменная для ошибок
    const passwordVisible = writable(false); // состояние видимости пароля

    const togglePasswordVisibility = () => {
        passwordVisible.update(visible => !visible); // переключаем видимость
    };

    const setCookie = (name, value, options = {}) => {
        const { expires, path = "/", secure = true, httpOnly = false } = options;
        let cookieString = `${encodeURIComponent(name)}=${encodeURIComponent(value)}`;

        if (expires) {
            const expiryDate = new Date(Date.now() + expires * 1000).toUTCString();
            cookieString += `; expires=${expiryDate}`;
        }
        cookieString += `; path=${path}`;
        if (secure) cookieString += "; Secure";
        if (httpOnly) cookieString += "; HttpOnly";

        document.cookie = cookieString;
    };

    const onButtonClick = async () => {
        formError.set("");

        if ($email === "") {
            formError.set("Please enter your email");
            return;
        }

        if (!validateEmail($email)) {
            formError.set("Please enter a valid email address");
            return;
        }

        if ($password === "") {
            formError.set("Please enter a password");
            return;
        }

        try {
            const response = await login($email, $password);
            const decoded = parseJwt(response.token);
            const userId = decoded.sub;

            localStorage.setItem("user", JSON.stringify({
                token: response.token,
                user_id: userId
            }));

            loggedIn.set(true);
            emailStore.set($email);
            window.location.href = '/';
        } catch (error) {
            if (error.message === "User not found") {
                formError.set("User with this email was not found.");
            } else if (error.message === "Invalid credentials") {
                formError.set("Provided email or password is incorrect.");
            } else {
                window.alert("Login failed: " + error.message);
            }
        }
    };

    const updatePassword = (event) => {
        password.set(event.target.value);
    };
</script>

<div class="mainContainer">
    <div class="titleContainer">
        <div>Login</div>
    </div>
    <br/>
    <div class="inputContainer emailContainer">
        <input
                id="email"
                bind:value={$email}
                placeholder="Enter your email here"
                class="inputBox" />
    </div>
    <br/>
    <div class="inputContainer">
        <div class="passwordContainer">
            <input
                    id="password"
                    value={$password}
                    on:input={updatePassword}
                    type={$passwordVisible ? 'text' : 'password'}
                    placeholder="Enter your password here"
                    class="inputBox" />
            <button
                    type="button"
                    class="togglePassword"
                    on:click={togglePasswordVisibility}
                    aria-label="Toggle password visibility"
            >
                <img
                        src={$passwordVisible ? '/open_eye.png' : '/closed_eye.png'}
                        alt="Toggle password visibility"
                        class="eyeIcon"
                />
            </button>
        </div>
    </div>
    <div class="errorLabel">{$formError}</div>
    <br/>
    <div class="inputContainer">
        <input
                class="inputButton"
                type="button"
                on:click={onButtonClick}
                value="Log in"/>
    </div>
    <br/>
    <div class="signupLinkContainer">
        <span>Don't have an account? </span>
        <a href={'/signup'}>Sign up here</a>
    </div>
</div>

<style>
    .mainContainer {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        height: 100vh;
        width: 100vw;
        text-align: center;
    }

    .titleContainer {
        font-size: 64px;
        font-weight: bold;
        margin-bottom: 20px;
        color: royalblue;
    }

    .inputContainer {
        margin-bottom: 20px;
    }

    .emailContainer {
        position: relative;
        width: 300px;
    }

    .passwordContainer {
        position: relative;
        width: 300px;
    }

    .inputBox {
        height: 48px;
        width: 100%;
        font-size: large;
        border-radius: 8px;
        border: 1px solid grey;
        padding-left: 8px;
    }

    .togglePassword {
        position: absolute;
        right: 10px;
        top: 50%;
        transform: translateY(-50%);
        background: none;
        border: none;
        cursor: pointer;
        padding: 0;
    }

    .eyeIcon {
        width: 24px;
        height: 24px;
        position: relative;
    }

    .errorLabel {
        color: red;
        font-size: 18px;
        margin-top: 3px;
        font-family: "Cambay Devanagari",serif;
    }

    .mainContainer input[type="button"] {
        border: none;
        background: cornflowerblue;
        color: white;
        padding: 12px 24px;
        font-size: 24px;
        border-radius: 8px;
        cursor: pointer;
        transition: all 0.3s ease;
    }

    .mainContainer input[type="button"]:hover {
        background: dodgerblue; /* Цвет кнопки при наведении */
        transform: scale(1.05);  /* Увеличиваем размер кнопки при наведении */
    }

    .mainContainer input[type="button"]:active {
        transform: scale(0.98);  /* Слегка уменьшаем кнопку при нажатии */
        background: royalblue;   /* Цвет кнопки при нажатии */
    }

    .signupLinkContainer {
        margin-top: 20px;
        font-size: 16px;
    }

    .signupLinkContainer a {
        color: cornflowerblue;
        cursor: pointer;
        text-decoration: underline;
    }
</style>

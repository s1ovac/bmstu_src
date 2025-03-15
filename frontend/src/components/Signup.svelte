<script>
    import { writable } from 'svelte/store';
    import { push } from 'svelte-spa-router';
    import { signup } from '../lib/api.js';
    import { loggedIn, email as emailStore } from '../lib/stores.js';
    import { validateEmail } from '../lib/utils.js';

    const password = writable("");
    const email = writable("");
    const formError = writable(""); // Одна переменная для ошибок
    const passwordVisible = writable(false); // Состояние видимости пароля

    const togglePasswordVisibility = () => {
        passwordVisible.update(visible => !visible); // Переключаем видимость
    };

    const updateEmail = (event) => {
        email.set(event.target.value);
    };

    const updatePassword = (event) => {
        password.set(event.target.value);
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

        if ($password.length <= 6) {
            formError.set("The password must be more than 6 characters");
            return;
        }

        try {
            const response = await signup($email, $password);
            localStorage.setItem("user", JSON.stringify({ token: response.token }));
            loggedIn.set(true);
            emailStore.set($email);
            window.location.href = '/';
        } catch (error) {
            if (error.message === "User already exists") {
                formError.set("User with provided email already exists");
            } else {
                window.alert("Signup failed: " + error.message);
            }
        }
    };
</script>

<div class="mainContainer">
    <div class="titleContainer">
        <div>Signup</div>
    </div>
    <br/>
    <div class="inputContainer">
        <input
                id="email"
                value={$email}
                on:input={updateEmail}
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
    <div class="errorLabel">
        {$formError}
    </div>
    <br/>
    <div class="inputContainer">
        <input
                class="inputButton"
                type="button"
                on:click={onButtonClick}
                value="Sign up"/>
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
        transition: all 0.3s ease; /* Добавляем плавный переход */
    }

    .mainContainer input[type="button"]:hover {
        background: dodgerblue; /* Цвет кнопки при наведении */
        transform: scale(1.05);  /* Увеличиваем размер кнопки при наведении */
    }

    .mainContainer input[type="button"]:active {
        transform: scale(0.98);  /* Слегка уменьшаем кнопку при нажатии */
        background: royalblue;   /* Цвет кнопки при нажатии */
    }
</style>

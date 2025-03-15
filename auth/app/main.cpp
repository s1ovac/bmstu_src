// app/main.cpp
#include <drogon/drogon.h>
#include "controller.h"
#include "user_db.h"
#include "jwt_utils.h"
#include <fstream>
#include <sstream>

bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

// Function to read file content
std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    auto &app = drogon::app();
    app.loadConfigFile("../config.json");

    // Настройка CORS для всех запросов, начинающихся с /api
    app.registerPreRoutingAdvice(
            [](const drogon::HttpRequestPtr &req, drogon::FilterCallback &&stop, drogon::FilterChainCallback &&pass)
            {
                if (startsWith(req->path(), "/api"))
                {
                    // Обрабатываем preflight-запросы (OPTIONS)
                    if (req->method() == drogon::Options)
                    {
                        auto resp = drogon::HttpResponse::newHttpResponse();
                        resp->addHeader("Access-Control-Allow-Origin", "*");
                        resp->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                        resp->addHeader("Access-Control-Allow-Headers", "*");
                        resp->addHeader("Access-Control-Max-Age", "3600");
                        stop(resp);
                        return;
                    }

                    // Добавляем заголовки CORS для всех остальных запросов
                    req->addHeader("Access-Control-Allow-Origin", "*");
                    req->addHeader("Access-Control-Allow-Headers", "*");
                }

                pass();
            });

    // Настройка CORS для всех ответов на запросы
    app.registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp)
            {
                if (startsWith(req->path(), "/api"))
                {
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Headers", "*");
                }
            });

    // Загружаем параметры базы данных из конфигурационного файла
    auto dbConfig = app.getCustomConfig()["database"];
    std::string dbHost = dbConfig.get("host", "localhost").asString();
    std::string dbPort = dbConfig.get("port", "5432").asString();
    std::string dbName = dbConfig.get("dbname", "postgres").asString();
    std::string dbUser = dbConfig.get("user", "postgres").asString();
    std::string dbPassword = dbConfig.get("password", "password").asString();

    // Load RSA keys for JWT authentication
    try {
        // Get paths from config
        auto jwtConfig = app.getCustomConfig()["jwt"];
        std::string privateKeyPath = jwtConfig.get("private_key_path", "../keys/private.pem").asString();
        std::string publicKeyPath = jwtConfig.get("public_key_path", "../keys/public.pem").asString();

        LOG_INFO << "Loading JWT RSA keys from files";
        std::string privateKey = readFile(privateKeyPath);
        std::string publicKey = readFile(publicKeyPath);
        LOG_INFO << "Successfully loaded JWT RSA keys";

        // Set keys in JwtUtils
        JwtUtils::setPrivateKey(privateKey);
        JwtUtils::setPublicKey(publicKey);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to load JWT keys: " << e.what();
        LOG_ERROR << "Make sure the key files exist at the specified paths in config.json";
        return 1;
    }

    // Создаем объект базы данных
    DB db(dbHost, dbPort, dbName, dbUser, dbPassword);

    // Инициализация базы данных
    if (!db.init())
    {
        std::cerr << "Failed to initialize the database" << std::endl;
        return 1;
    }

    // Создаем объект AuthService
    AuthService authService(db);

    // Создаем экземпляр контроллера
    auto authController = std::make_shared<AuthController>(authService);

    // Регистрация маршрутов
    AuthController::registerRoutes(app, authController);

    // Запуск приложения
    app.run();
    return 0;
}
#include <drogon/drogon.h>
#include "controllers/AuthController.h"
#include "controllers/RoleController.h"
#include "controllers/GroupController.h"
#include "controllers/AdminController.h"
#include "services/RoleService.h"
#include "services/GroupService.h"
#include "services/AccessControlService.h"
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

    // Configure CORS for all requests starting with /api
    app.registerPreRoutingAdvice(
            [](const drogon::HttpRequestPtr &req, drogon::FilterCallback &&stop, drogon::FilterChainCallback &&pass)
            {
                if (startsWith(req->path(), "/api"))
                {
                    // Handle preflight requests (OPTIONS)
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

                    // Add CORS headers for all other requests
                    req->addHeader("Access-Control-Allow-Origin", "*");
                    req->addHeader("Access-Control-Allow-Headers", "*");
                }

                pass();
            });

    // Configure CORS for all responses
    app.registerPostHandlingAdvice(
            [](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp)
            {
                if (startsWith(req->path(), "/api"))
                {
                    resp->addHeader("Access-Control-Allow-Origin", "*");
                    resp->addHeader("Access-Control-Allow-Headers", "*");
                }
            });

    // Load database parameters from config
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

    // Create database instance
    DB db(dbHost, dbPort, dbName, dbUser, dbPassword);

    // Initialize database
    if (!db.init())
    {
        std::cerr << "Failed to initialize the database" << std::endl;
        return 1;
    }

    // Create service instances
    AuthService authService(db);
    AccessControlService accessControlService(db);
    RoleService roleService(db);
    GroupService groupService(db);

    // Register service singletons to make them available via instance() method
    auto accessControlServicePtr = std::make_shared<AccessControlService>(accessControlService);
    auto roleServicePtr = std::make_shared<RoleService>(roleService);
    auto groupServicePtr = std::make_shared<GroupService>(groupService);

    // Create controller instances
    auto authController = std::make_shared<AuthController>(authService);
    auto roleController = std::make_shared<RoleController>(roleService, accessControlService);
    auto groupController = std::make_shared<GroupController>(groupService, accessControlService);
    auto adminController = std::make_shared<AdminController>(db, accessControlService);

    // Register routes for all controllers
    AuthController::registerRoutes(app, authController);
    RoleController::registerRoutes(app, roleController);
    GroupController::registerRoutes(app, groupController);
    AdminController::registerRoutes(app, adminController);

    LOG_INFO << "Auth service started on port " << app.getListeners()[0].toPort();

    // Start the application
    app.run();
    return 0;
}
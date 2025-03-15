#include <drogon/drogon.h>
#include <trantor/utils/Logger.h>
#include "filters/JwtAuthFilter.h"
#include <fstream>
#include <sstream>

// Function to check string prefix
bool startsWith(const std::string& str, const std::string& prefix) {
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
    // Set logging level
    trantor::Logger::setLogLevel(trantor::Logger::kDebug);

    LOG_INFO << "Starting File Service";

    // Create application instance
    auto &app = drogon::app();

    // Load configuration file
    app.loadConfigFile("../config.json");
    LOG_DEBUG << "Configuration file loaded";

    // Configure CORS for all requests starting with /api
    app.registerPreRoutingAdvice(
            [](const drogon::HttpRequestPtr &req, drogon::FilterCallback &&stop, drogon::FilterChainCallback &&pass) {
                if (startsWith(req->path(), "/api")) {
                    // Handle preflight requests (OPTIONS)
                    if (req->method() == drogon::Options) {
                        LOG_DEBUG << "Handling CORS preflight request";
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
            [](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp) {
                if (startsWith(req->path(), "/api")) {
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

    // Initialize database singleton instance
    try {
        DB::initInstance(dbHost, dbPort, dbName, dbUser, dbPassword);
        LOG_INFO << "Database initialized successfully";
    } catch (const std::exception& e) {
        LOG_ERROR << e.what();
        return 1;
    }

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

        // Set keys in filter
        JwtAuthFilter::setPrivateKey(privateKey);
        JwtAuthFilter::setPublicKey(publicKey);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to load JWT keys: " << e.what();
        LOG_ERROR << "Make sure the key files exist at the specified paths in config.json";
        return 1;
    }

    LOG_INFO << "File Service is running";

    // Run the application
    app.run();

    return 0;
}
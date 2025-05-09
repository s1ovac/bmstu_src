#include "jwt_utils.h"
#include "filters/JwtAuthFilter.h"
#include <jwt-cpp/jwt.h>
#include <drogon/drogon.h>

std::string JwtUtils::generateToken(const std::string& userId)
{
    // Get the private key from JwtAuthFilter
    std::string privateKey = JwtAuthFilter::getPrivateKey();

    // Create a JWT using RS256 algorithm with the private key
    auto token = jwt::create()
            .set_type("JWT")
            .set_issuer("auth-service")
            .set_subject(userId)
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
            .sign(jwt::algorithm::rs256("", privateKey, "", ""));

    return token;
}

bool JwtUtils::validateToken(const std::string& token, std::string& userId)
{
    try {
        std::string publicKey = JwtAuthFilter::getPublicKey();

        auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::rs256(publicKey, "", "", ""))
                .with_issuer("auth-service");

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        userId = decoded.get_subject();

        return true;
    }
    catch (const std::exception& e)
    {
        // Log the error and return false
        LOG_ERROR << "JWT validation error: " << e.what();
        return false;
    }
}
#include "JWT.h"
#include <jwt-cpp/jwt.h>
#include <drogon/drogon.h>

std::string JwtUtils::privateKey_;
std::string JwtUtils::publicKey_;

std::string JwtUtils::generateToken(const std::string& userId)
{
    // Create a JWT using RS256 algorithm with the private key
    auto token = jwt::create()
            .set_type("JWT")
            .set_issuer("auth-service")
            .set_subject(userId)
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{24})
            .sign(jwt::algorithm::rs256("", privateKey_, "", ""));

    return token;
}

bool JwtUtils::validateToken(const std::string& token, std::string& userId)
{
    try {
        // Verify the token using RS256 algorithm with the public key
        auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::rs256(publicKey_, "", "", ""))
                .with_issuer("auth-service");

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        // Extract userId from the token
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

void JwtUtils::setPrivateKey(const std::string& key)
{
    privateKey_ = key;
}

void JwtUtils::setPublicKey(const std::string& key)
{
    publicKey_ = key;
}

std::string JwtUtils::getPrivateKey()
{
    return privateKey_;
}

std::string JwtUtils::getPublicKey()
{
    return publicKey_;
}
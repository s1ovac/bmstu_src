// Include/jwt_utils.h
#ifndef AUTH_JWT_UTILS_H
#define AUTH_JWT_UTILS_H

#include <string>
#include <vector>

class JwtUtils {
public:
    static std::string generateToken(const std::string& userId);
    static bool validateToken(const std::string& token, std::string& userId);

    static void setPrivateKey(const std::string& key);
    static void setPublicKey(const std::string& key);
    static std::string getPrivateKey();
    static std::string getPublicKey();

private:
    static std::string privateKey_;
    static std::string publicKey_;
};

#endif //AUTH_JWT_UTILS_H
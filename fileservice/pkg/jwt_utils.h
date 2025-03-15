#ifndef AUTH_JWT_UTILS_H
#define AUTH_JWT_UTILS_H

#include <string>
#include <vector>

class JwtUtils {
public:
    static std::string generateToken(const std::string& userId);

    static bool validateToken(const std::string& token, std::string& userId);
};

#endif //AUTH_JWT_UTILS_H
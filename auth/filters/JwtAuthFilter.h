#pragma once

#include <drogon/HttpFilter.h>
#include <jwt-cpp/jwt.h>
#include <string>
#include <memory>
#include "repository/DB.h"

using namespace drogon;

class JwtAuthFilter : public HttpFilter<JwtAuthFilter>
{
public:
    JwtAuthFilter();
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;

    static void setPrivateKey(const std::string& key)
    {
        privateKey_ = key;
    }

    static void setPublicKey(const std::string& key)
    {
        publicKey_ = key;
    }

    static std::string getPrivateKey()
    {
        return privateKey_;
    }

    static std::string getPublicKey()
    {
        return publicKey_;
    }

private:
    static std::string privateKey_;
    static std::string publicKey_;
};
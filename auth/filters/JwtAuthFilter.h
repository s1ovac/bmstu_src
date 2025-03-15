#pragma once

#include <drogon/HttpFilter.h>
#include <string>
#include <memory>

using namespace drogon;

class JwtAuthFilter : public HttpFilter<JwtAuthFilter>
{
public:
    JwtAuthFilter();
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;
};
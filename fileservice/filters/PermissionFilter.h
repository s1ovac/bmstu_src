#pragma once

#include <drogon/HttpFilter.h>
#include <jwt-cpp/jwt.h>
#include <string>
#include <memory>
#include "../pkg/permission_utils.h"

using namespace drogon;

/**
 * HTTP filter that checks if a user has the required permissions
 * This filter should be applied after JwtAuthFilter
 */
class PermissionFilter : public HttpFilter<PermissionFilter>
{
public:
    PermissionFilter();
    void doFilter(const HttpRequestPtr &req,
                  FilterCallback &&fcb,
                  FilterChainCallback &&fccb) override;

private:
    std::shared_ptr<PermissionUtils> permissionUtils_;

    // Helper methods for permission checks
    bool checkAdminPermission(const HttpRequestPtr &req, FilterCallback &fcb);
};
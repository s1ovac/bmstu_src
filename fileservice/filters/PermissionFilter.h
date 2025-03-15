#pragma once

#include <drogon/HttpFilter.h>

#include <utility>
#include "db.h"

class PermissionFilter : public drogon::HttpFilter<PermissionFilter>
{
public:
    PermissionFilter();
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb,
                  drogon::FilterChainCallback &&fccb) override;

private:
    std::string requiredPermission_;
    std::shared_ptr<DB> db_;
};
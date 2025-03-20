#include "permission_utils.h"
#include <drogon/drogon.h>
#include <ctime>
#include <memory>

const std::string AUTH_SERVICE_URL = "http://localhost:8082";
const std::string AUTH_TOKEN = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3NDI1NTQwODYsImlhdCI6MTc0MjQ2NzY4NiwiaXNzIjoiYXV0aC1zZXJ2aWNlIiwic3ViIjoiMSJ9.NHI27-PpjsOn9IwsPR7Y48DabOppce4_j-d6qzvYw34sgeqT_5CuKudvzEyvVAM3RWvhCI7dnBOMKITch-guAdcViDTePGyjcNOz3ScWKP4A1SuzPAZ5v101-JYJ-P5v8g5uMs2yeB4GwkhhOY-RMLk68x_fGcb4caJw-a4xwuxfOn0ECs9rOjXsBr-33TLQIZIdmnc6lbP1lcPB0krUqHtai8pBYVab-4HgJc6NMvC3PE-7-sYsrMuBumYCY1VwYdvEukUf-lCg3Ld_EGyc28FK0ObuQwghijTWO_3hr1zhG0UDNhpn_ij_YUw-di2lywrmllUbqKK6H-Hkb_6p8w";

std::shared_ptr<PermissionUtils> PermissionUtils::instance()
{
    static std::shared_ptr<PermissionUtils> instance(new PermissionUtils());
    return instance;
}

PermissionUtils::PermissionUtils()
{
    LOG_INFO << "Initializing PermissionUtils";
}

bool PermissionUtils::hasPermission(const std::string& userId, const std::string& permission)
{
    // Fetch permissions if not cached or expired
    fetchUserPermissions(userId);

    // Check if user has the permission
    std::lock_guard<std::mutex> lock(cacheMutex_);

    auto userIt = permissionCache_.find(userId);
    if (userIt == permissionCache_.end()) {
        return false;  // User not found in cache
    }

    // Check if the permission exists in the user's permission map
    auto permIt = userIt->second.find(permission);
    if (permIt == userIt->second.end()) {
        return false;  // Permission not found for this user
    }

    // If permission exists, check if it's still valid (not expired)
    time_t now = std::time(nullptr);
    if (now - permIt->second > cacheExpirationTime_) {
        // Permission expired, remove it from the cache
        userIt->second.erase(permIt);
        if (userIt->second.empty()) {
            permissionCache_.erase(userIt);
        }

        // Try to refresh permissions
        fetchUserPermissions(userId, true);

        // Check again after refresh
        userIt = permissionCache_.find(userId);
        if (userIt == permissionCache_.end()) {
            return false;
        }

        permIt = userIt->second.find(permission);
        if (permIt == userIt->second.end()) {
            return false;
        }
    }

    return true;
}

bool PermissionUtils::fetchUserPermissions(const std::string& userId, bool forceRefresh)
{
    // Check if we need to refresh the cache
    if (!forceRefresh) {
        std::lock_guard<std::mutex> lock(cacheMutex_);
        auto userIt = permissionCache_.find(userId);
        if (userIt != permissionCache_.end() && !userIt->second.empty()) {
            // Check if any permission is still valid
            time_t now = std::time(nullptr);
            for (const auto& permTime : userIt->second) {
                if (now - permTime.second <= cacheExpirationTime_) {
                    // At least one permission is still valid
                    return true;
                }
            }
        }
    }

    // Use the member variable for auth service URL
    const std::string endpoint = AUTH_SERVICE_URL + "/api/v1/users/" + userId + "/permissions";

    // Make an HTTP request to get permissions
    auto client = drogon::HttpClient::newHttpClient(AUTH_SERVICE_URL);
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath("/api/v1/users/" + userId + "/permissions");
    req->addHeader("Authorization", "Bearer " + AUTH_TOKEN);

    LOG_INFO << "Fetching permissions for user " << userId << " from auth service with 5s timeout";

    try {
        // Send request with a 5-second timeout
        auto [result, response] = client->sendRequest(req, 5.0);

        // Check if the request timed out
        if (result == drogon::ReqResult::Timeout) {
            LOG_ERROR << "Request timed out when fetching permissions for user " << userId;
            return false;
        }

        // Process the response if successful
        if (result == drogon::ReqResult::Ok && response->statusCode() == drogon::k200OK) {
            // Get JSON object directly from the response
            auto jsonPtr = response->getJsonObject();

            if (jsonPtr && jsonPtr->isMember("permissions") && (*jsonPtr)["permissions"].isArray()) {
                std::unordered_map<std::string, time_t> permissions;
                time_t now = std::time(nullptr);

                const auto& permArray = (*jsonPtr)["permissions"];
                for (const auto& perm : permArray) {
                    if (perm.isString()) {
                        permissions[perm.asString()] = now;
                    }
                }

                // Update the cache
                std::lock_guard<std::mutex> lock(cacheMutex_);
                permissionCache_[userId] = std::move(permissions);

                LOG_INFO << "Cached " << permArray.size() << " permissions for user " << userId;
                return true;
            }
            LOG_ERROR << "Invalid JSON structure in permissions response for user " << userId;
        } else {
            LOG_ERROR << "Auth service returned status " << (response ? std::to_string(response->statusCode()) : "unknown")
                      << " when fetching permissions for user " << userId;
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Exception when fetching permissions for user " << userId << ": " << e.what();
    }

    return false;
}

void PermissionUtils::clearCache()
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    permissionCache_.clear();
    LOG_INFO << "Permission cache cleared";
}
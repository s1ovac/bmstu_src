#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

/**
 * Utility class for permission checks
 */
class PermissionUtils {
public:
    /**
     * Get singleton instance
     */
    static std::shared_ptr<PermissionUtils> instance();

    /**
     * Check if a user has a specific permission
     *
     * @param userId User ID to check
     * @param permission Permission name to check
     * @return true if the user has the permission, false otherwise
     */
    bool hasPermission(const std::string &userId, const std::string &permission);

    /**
     * Fetch user permissions from auth service and cache them
     * 
     * @param userId User ID to fetch permissions for
     * @param forceRefresh Force a refresh of the cached permissions
     * @return true if permissions were successfully fetched, false otherwise
     */
    bool fetchUserPermissions(const std::string &userId, bool forceRefresh = false);

    /**
     * Clear all cached permissions
     */
    void clearCache();

private:
    PermissionUtils();

    // Cache structure: userId -> (permission -> timestamp)
    std::unordered_map<std::string, std::unordered_map<std::string, time_t>> permissionCache_;

    // Cache mutex for thread safety
    std::mutex cacheMutex_;

    // Cache expiration time in seconds (default: 5 minutes)
    const time_t cacheExpirationTime_ = 300;
};
#pragma once

#include <string>
#include <vector>
#include <regex>
#include <algorithm>

namespace ValidationUtils {

// Error codes for file/folder name validation
    enum class ValidationErrorCode {
        OK,
        EMPTY_NAME,
        NAME_TOO_LONG,
        INVALID_CHARACTERS,
        INVALID_START_END,
        RESERVED_NAME
    };

// Validation result structure
    struct ValidationResult {
        bool valid;
        ValidationErrorCode errorCode;
        std::string errorMessage;
    };

// Validates file/folder names according to filesystem rules
    ValidationResult validateName(const std::string& name) {
        ValidationResult result = {true, ValidationErrorCode::OK, ""};

        // Check if name is empty
        if (name.empty()) {
            return {false, ValidationErrorCode::EMPTY_NAME, "Name cannot be empty"};
        }

        // Check length (too long names can cause issues with some filesystems)
        if (name.length() > 255) {
            return {false, ValidationErrorCode::NAME_TOO_LONG, "Name is too long (maximum is 255 characters)"};
        }

        // Check for invalid characters
        // Disallow: / \ : * ? " < > | and control characters
        std::regex validNamePattern("^[^\\\\/:*?\"<>|\\x00-\\x1F]+$");
        if (!std::regex_match(name, validNamePattern)) {
            return {false, ValidationErrorCode::INVALID_CHARACTERS,
                    "Name contains invalid characters. The following characters are not allowed: / \\ : * ? \" < > | and control characters"};
        }

        // Disallow names that start or end with spaces or periods
        if (name.front() == ' ' || name.front() == '.' || name.back() == ' ' || name.back() == '.') {
            return {false, ValidationErrorCode::INVALID_START_END,
                    "Name cannot start or end with spaces or periods"};
        }

        // Disallow reserved names (Windows-specific, but good practice)
        std::vector<std::string> reservedNames = {
                "CON", "PRN", "AUX", "NUL",
                "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
        };

        // Convert name to uppercase for comparison
        std::string upperName = name;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

        for (const auto& reserved : reservedNames) {
            if (upperName == reserved || upperName.find(reserved + ".") == 0) {
                return {false, ValidationErrorCode::RESERVED_NAME,
                        "Name is a reserved system name or starts with a reserved name"};
            }
        }

        return result;
    }

} // namespace ValidationUtils
//
// Created by igor on 7/20/25.
//

#pragma once

#include <string>

// C++17 compatibility helpers

inline bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

inline bool starts_with(const std::string& str, char prefix) {
    return !str.empty() && str[0] == prefix;
}

inline bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline bool ends_with(const std::string& str, char suffix) {
    return !str.empty() && str[str.size() - 1] == suffix;
}

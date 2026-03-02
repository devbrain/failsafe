#pragma once

#include <system_error>
#include <string>

namespace failsafe {
    using errno_type = int;

    inline std::string error_to_string(errno_type err) {
        return std::error_code(err, std::system_category()).message();
    }
}

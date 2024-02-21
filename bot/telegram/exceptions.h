#include <stdexcept>
#include <string>

struct TelegramAPIError : std::runtime_error {
    TelegramAPIError(int http_code, bool is_ok, std::string details)
        : std::runtime_error("api error: code=" + std::to_string(http_code) +
                             " \"ok\" status=" + std::to_string(is_ok) + " details=" + details),
          http_code(http_code),
          is_ok(is_ok),
          details(std::move(details)) {
    }

    int http_code;
    bool is_ok;
    std::string details;
};

struct SystemError : std::runtime_error {
    SystemError(std::string details)
        : std::runtime_error("system error: details=" + details), details(std::move(details)) {
    }

    std::string details;
};

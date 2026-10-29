#include "backtester/time_utils.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace bt {

time_point parse_datetime(const std::string& text) {
    std::tm tm{};
    std::istringstream ss(text);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        throw std::invalid_argument("Invalid date: '" + text + "'");
    }
    // Optional time part
    if (ss >> std::ws && !ss.eof()) {
        ss >> std::get_time(&tm, "%H:%M:%S");
        if (ss.fail()) {
            throw std::invalid_argument("Invalid datetime: '" + text + "'");
        }
    }
    // timegm interprets the fields as UTC, unlike mktime which uses the local time zone
    return std::chrono::system_clock::from_time_t(timegm(&tm));
}

std::string format_datetime(time_point tp) {
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
    gmtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace bt

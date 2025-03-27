#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <stdexcept>



/// Convertit une date sous forme de chaîne "YYYY-MM-DD" en std::chrono::system_clock::time_point
inline std::chrono::system_clock::time_point parse_date(const std::string& date_str) {
    std::tm tm = {};
    std::istringstream ss(date_str);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        throw std::runtime_error("Date invalide : " + date_str);
    }
    std::time_t tt = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(tt);
}

/// Affiche un std::chrono::system_clock::time_point en format YYYY-MM-DD HH:MM:SS
inline std::string format_datetime(const std::chrono::system_clock::time_point& tp) {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time);  // ou std::gmtime si tu veux UTC
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

#endif // UTILITIES_H

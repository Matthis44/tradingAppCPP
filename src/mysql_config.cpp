#include <cstdlib>
#include <fstream>
#include <stdexcept>

#include "backtester/mysql_data_handler.h"

namespace bt {
namespace {

std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\"'");
    const auto end = s.find_last_not_of(" \t\r\"'");
    return begin == std::string::npos ? "" : s.substr(begin, end - begin + 1);
}

/// Loads KEY=VALUE lines into the environment without overriding existing variables.
void load_env_file(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        setenv(trim(line.substr(0, eq)).c_str(), trim(line.substr(eq + 1)).c_str(), /*overwrite=*/0);
    }
}

std::string env_or(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}

}  // namespace

MySqlConfig MySqlConfig::from_env(const std::string& env_file) {
    load_env_file(env_file);
    MySqlConfig config;
    config.host = env_or("MYSQL_HOST", config.host);
    config.user = env_or("MYSQL_USER", "");
    config.password = env_or("MYSQL_PASSWORD", "");
    config.database = env_or("MYSQL_DATABASE", config.database);
    if (config.user.empty() || config.password.empty()) {
        throw std::runtime_error("MYSQL_USER and MYSQL_PASSWORD must be set (see .env.example)");
    }
    return config;
}

}  // namespace bt

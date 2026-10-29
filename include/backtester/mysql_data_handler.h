#pragma once

#include <string>

#include "backtester/data_handler.h"

namespace bt {

struct MySqlConfig {
    std::string host = "tcp://127.0.0.1:3306";
    std::string user;
    std::string password;
    std::string database = "securities_master";

    /// Reads MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD and MYSQL_DATABASE from the
    /// environment, after loading `env_file` if it exists. Throws if credentials are missing.
    static MySqlConfig from_env(const std::string& env_file = ".env");
};

/// Loads intraday bars from the `price_bar` table (see sql/schema.sql).
/// Only compiled when the project is configured with -DWITH_MYSQL=ON.
class MySqlDataHandler final : public DataHandler {
public:
    MySqlDataHandler(std::shared_ptr<EventQueue> events, std::vector<std::string> symbols, MySqlConfig config,
                     std::string interval, std::string start_date);

protected:
    std::unordered_map<std::string, std::vector<Bar>> load() override;

private:
    MySqlConfig config_;
    std::string interval_;
    std::string start_date_;
};

}  // namespace bt

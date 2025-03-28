#ifndef MYSQL_DATA_HANDLER_H
#define MYSQL_DATA_HANDLER_H

#include "event_queue.h"
#include "bar_row_use.h"
#include <unordered_map>
#include <deque>
#include <string>
#include <vector>
#include <memory>

// MySQL Connector/C++
#include <cppconn/resultset.h>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <mysql_driver.h>
#include <mysql_connection.h>



class mysql_data_handler : public my_data_handler {
public:
    mysql_data_handler(std::shared_ptr<event_queue> events,
                       const std::vector<std::string>& symbol_list,
                       const std::string& interval,
                       const std::string& start_date="",
                       const std::string& user = "sec_user",
                       const std::string& password = "1Lo2Ma3Is@",
                       const std::string& host = "tcp://127.0.0.1:3306",
                       const std::string& db = "securities_master");

    mysql_bar_row get_latest_bar(const std::string& symbol) const override;
    std::vector<mysql_bar_row> get_latest_bars(const std::string& symbol, int N = 1) const override;
    std::chrono::system_clock::time_point get_latest_bar_datetime(const std::string& symbol) const override;
    double get_latest_bar_value(const std::string& symbol, const std::string& val_type) const override;
    std::vector<double> get_latest_bars_values(const std::string& symbol, const std::string& val_type, int N = 1) const override;
    std::vector<std::string> get_symbol_list() const override;
    void update_bars() override;
    bool continue_backtest() const override;


private:
    void load_data_from_db();
    std::vector<mysql_bar_row> fetch_symbol_bars(const std::string& symbol);
    std::shared_ptr<event_queue> events_;
    std::vector<std::string> symbol_list_;
    std::unordered_map<std::string, std::vector<mysql_bar_row>> symbol_data_;
    std::unordered_map<std::string, std::deque<mysql_bar_row>> latest_symbol_data_;
    std::unordered_map<std::string, size_t> symbol_index_;

    std::shared_ptr<sql::Connection> conn_;
    const std::string start_date_;
    const std::string interval_;
    std::string user_;
    std::string password_;
    std::string host_;
    std::string db_;
};

#endif // MYSQL_DATA_HANDLER_H

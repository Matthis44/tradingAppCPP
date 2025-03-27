#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "event_queue.h"
#include "event.h"
#include "bar_row_types.h"
#include <chrono>




template<typename bar_row>

class data_handler {
public:
    virtual ~data_handler() = default;
    virtual bool continue_backtest() const = 0;
    virtual std::vector<std::string> get_symbol_list() const = 0;
    virtual bar_row get_latest_bar(const std::string& symbol) const = 0;
    virtual std::vector<bar_row> get_latest_bars(const std::string& symbol, int N = 1) const = 0;
    virtual std::chrono::system_clock::time_point get_latest_bar_datetime(const std::string& symbol) const = 0;
    virtual double get_latest_bar_value(const std::string& symbol, const std::string& val_type) const = 0;
    virtual std::vector<double> get_latest_bars_values(const std::string& symbol, const std::string& val_type, int N = 1) const = 0;
    virtual void update_bars() = 0;
};

#endif // DATA_HANDLER_H

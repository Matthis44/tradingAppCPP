#ifndef BACKTEST_H
#define BACKTEST_H

#include "event_queue.h"
#include "bar_row_use.h"
#include "execution_handler.h"
#include "portfolio.h"
#include "strategy.h"
#include <memory>
#include <string>
#include <vector>

class Backtest {
public:
    Backtest(const std::vector<std::string>& symbol_list,
             double initial_capital,
             int heartbeat,
             std::chrono::system_clock::time_point& start_date,
             std::shared_ptr<my_data_handler> data_handler,
             std::shared_ptr<execution_handler> execution_handler,
             std::shared_ptr<portfolio> portfolio,
             std::shared_ptr<strategy> strategy,
             std::shared_ptr<event_queue> events);
    void simulate_trading();
    void process_event(const std::shared_ptr<event>& event);

private:
    std::vector<std::string> symbol_list_;
    double initial_capital_;
    int heartbeat_;
    std::chrono::system_clock::time_point  start_date_;
    std::shared_ptr<event_queue> events_;
    std::shared_ptr<my_data_handler> data_handler_;
    std::shared_ptr<execution_handler> execution_handler_;
    std::shared_ptr<portfolio> portfolio_;
    std::shared_ptr<strategy> strategy_;
    int signals_;
    int orders_;
    int fills_;
    void run_backtest();
    void output_performance();
};

#endif // BACKTEST_H
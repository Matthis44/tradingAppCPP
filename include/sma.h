#ifndef SIMPLE_MOVING_AVERAGE_STRATEGY_H
#define SIMPLE_MOVING_AVERAGE_STRATEGY_H

#include "strategy.h"
#include "bar_row_use.h"
#include <unordered_map>
#include <vector>
#include <string>

class simple_moving_average_strategy : public strategy {
public:
    simple_moving_average_strategy(
        std::shared_ptr<my_data_handler> data,
        std::shared_ptr<event_queue> events,
        int short_window,
        int long_window);

    void calculate_signals(const std::shared_ptr<event>& market_event) override;

private:
    std::shared_ptr<my_data_handler> data_;
    std::shared_ptr<event_queue> events_;

    int short_window_;
    int long_window_;

    std::unordered_map<std::string, bool> bought_;

    double calculate_sma(const std::vector<double>& prices);
};

#endif // SIMPLE_MOVING_AVERAGE_STRATEGY_H

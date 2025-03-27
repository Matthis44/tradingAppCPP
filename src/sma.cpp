#include "sma.h"
#include "event.h"
#include "bar_row_use.h"
#include "utilities.h"
#include <numeric>
#include <iostream>

simple_moving_average_strategy::simple_moving_average_strategy(
    std::shared_ptr<my_data_handler> data,
    std::shared_ptr<event_queue> events,
    int short_window,
    int long_window)
    : data_(data), events_(events),
      short_window_(short_window), long_window_(long_window) {
    
    for (const auto& symbol : data_->get_symbol_list()) {
        bought_[symbol] = false;
    }
}

double simple_moving_average_strategy::calculate_sma(const std::vector<double>& prices) {
    if (prices.empty()) return 0.0;
    return std::accumulate(prices.begin(), prices.end(), 0.0) / prices.size();
}

void simple_moving_average_strategy::calculate_signals(const std::shared_ptr<event>& market_event) {
    std::cout << "[STRATEGY] calculate_signals appelée avec : " << market_event->get_type() << std::endl;

    if (market_event->get_type() != "MARKET") return;

    for (const auto& symbol : data_->get_symbol_list()) {
        auto short_prices = data_->get_latest_bars_values(symbol, "adj_close", short_window_);
        auto long_prices = data_->get_latest_bars_values(symbol, "adj_close", long_window_);

        if (short_prices.size() < short_window_ || long_prices.size() < long_window_) continue;

        double short_sma = calculate_sma(short_prices);
        double long_sma = calculate_sma(long_prices);
        auto datetime = data_->get_latest_bar_datetime(symbol);

        std::cout << "[SMA DEBUG] " << symbol
                  << " | short_sma: " << short_sma
                  << " | long_sma: " << long_sma
                  << " | datetime: " << format_datetime(datetime)
                  << " | état actuel: " << (bought_[symbol] ? "ACHETÉ" : "NON ACHETÉ")
                  << std::endl;

        if (short_sma > long_sma && !bought_[symbol]) {
            events_->push(std::make_shared<signal_event>("SMA_Strategy", symbol, datetime, "LONG", 1.0));
            bought_[symbol] = true;
        } else if (short_sma < long_sma && bought_[symbol]) {
            events_->push(std::make_shared<signal_event>("SMA_Strategy", symbol, datetime, "EXIT", 1.0));
            bought_[symbol] = false;
        }
    }
}


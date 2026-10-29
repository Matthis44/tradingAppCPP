#include "backtester/strategy.h"

#include <numeric>
#include <stdexcept>

namespace bt {
namespace {

double mean(std::vector<double>::const_iterator begin, std::vector<double>::const_iterator end) {
    return std::accumulate(begin, end, 0.0) / static_cast<double>(std::distance(begin, end));
}

}  // namespace

MovingAverageCrossStrategy::MovingAverageCrossStrategy(std::shared_ptr<DataHandler> data,
                                                       std::shared_ptr<EventQueue> events, std::size_t short_window,
                                                       std::size_t long_window)
    : data_(std::move(data)), events_(std::move(events)), short_window_(short_window), long_window_(long_window) {
    if (short_window_ == 0 || short_window_ >= long_window_) {
        throw std::invalid_argument("Expected 0 < short_window < long_window");
    }
    for (const auto& s : data_->symbols()) invested_[s] = false;
}

void MovingAverageCrossStrategy::calculate_signals(const MarketEvent&) {
    for (const auto& symbol : data_->symbols()) {
        const auto prices = data_->latest_values(symbol, "adj_close", long_window_);
        if (prices.size() < long_window_) continue;  // warm-up

        const double long_sma = mean(prices.begin(), prices.end());
        const double short_sma = mean(prices.end() - static_cast<long>(short_window_), prices.end());
        const auto now = data_->latest_bar(symbol).datetime;

        bool& invested = invested_[symbol];
        if (short_sma > long_sma && !invested) {
            events_->push(std::make_shared<SignalEvent>("MACross", symbol, now, SignalType::Long));
            invested = true;
        } else if (short_sma < long_sma && invested) {
            events_->push(std::make_shared<SignalEvent>("MACross", symbol, now, SignalType::Exit));
            invested = false;
        }
    }
}

}  // namespace bt

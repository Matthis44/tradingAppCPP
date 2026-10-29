#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "backtester/data_handler.h"
#include "backtester/events.h"

namespace bt {

class Strategy {
public:
    virtual ~Strategy() = default;
    /// Called on every MarketEvent; may push SignalEvents on the queue.
    virtual void calculate_signals(const MarketEvent& event) = 0;
};

/// Long-only moving average crossover on adjusted close prices.
class MovingAverageCrossStrategy final : public Strategy {
public:
    MovingAverageCrossStrategy(std::shared_ptr<DataHandler> data, std::shared_ptr<EventQueue> events,
                               std::size_t short_window, std::size_t long_window);

    void calculate_signals(const MarketEvent& event) override;

private:
    std::shared_ptr<DataHandler> data_;
    std::shared_ptr<EventQueue> events_;
    std::size_t short_window_;
    std::size_t long_window_;
    std::unordered_map<std::string, bool> invested_;
};

}  // namespace bt

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "backtester/data_handler.h"
#include "backtester/events.h"

namespace bt {

struct EquityPoint {
    time_point datetime;
    double cash;
    double market_value;
    double commission;
    double total;
};

/// Tracks positions and mark-to-market holdings, and turns signals into orders.
class Portfolio {
public:
    Portfolio(std::shared_ptr<DataHandler> data, std::shared_ptr<EventQueue> events, double initial_capital,
              int lot_size = 100);

    void update_timeindex();               ///< mark to market on a new bar
    void on_signal(const SignalEvent& s);  ///< may push an OrderEvent
    void on_fill(const FillEvent& f);

    int position(const std::string& symbol) const { return positions_.at(symbol); }
    double cash() const { return cash_; }
    double initial_capital() const { return initial_capital_; }
    const std::vector<EquityPoint>& equity_curve() const { return equity_; }

private:
    std::shared_ptr<DataHandler> data_;
    std::shared_ptr<EventQueue> events_;
    double initial_capital_;
    int lot_size_;
    double cash_;
    double commission_ = 0.0;
    std::unordered_map<std::string, int> positions_;
    std::vector<EquityPoint> equity_;
};

}  // namespace bt

#include "backtester/portfolio.h"

#include <cstdlib>
#include <stdexcept>

namespace bt {

Portfolio::Portfolio(std::shared_ptr<DataHandler> data, std::shared_ptr<EventQueue> events, double initial_capital,
                     int lot_size)
    : data_(std::move(data)),
      events_(std::move(events)),
      initial_capital_(initial_capital),
      lot_size_(lot_size),
      cash_(initial_capital) {
    if (initial_capital <= 0 || lot_size <= 0) throw std::invalid_argument("Capital and lot size must be positive");
    for (const auto& s : data_->symbols()) positions_[s] = 0;
}

void Portfolio::update_timeindex() {
    double market_value = 0.0;
    for (const auto& [symbol, qty] : positions_) market_value += qty * data_->latest_bar(symbol).adj_close;
    const auto now = data_->latest_bar(data_->symbols().front()).datetime;
    equity_.push_back({now, cash_, market_value, commission_, cash_ + market_value});
}

void Portfolio::on_signal(const SignalEvent& signal) {
    const int qty = positions_.at(signal.symbol);
    std::shared_ptr<OrderEvent> order;

    switch (signal.signal) {
        case SignalType::Long:
            if (qty == 0) {
                const double cost = lot_size_ * data_->latest_bar(signal.symbol).adj_close;
                if (cost <= cash_) order = std::make_shared<OrderEvent>(signal.symbol, lot_size_, Direction::Buy);
            }
            break;
        case SignalType::Short:
            if (qty == 0) order = std::make_shared<OrderEvent>(signal.symbol, lot_size_, Direction::Sell);
            break;
        case SignalType::Exit:
            if (qty != 0)
                order = std::make_shared<OrderEvent>(signal.symbol, std::abs(qty),
                                                     qty > 0 ? Direction::Sell : Direction::Buy);
            break;
    }
    if (order) events_->push(order);
}

void Portfolio::on_fill(const FillEvent& fill) {
    const int sign = fill.direction == Direction::Buy ? 1 : -1;
    positions_.at(fill.symbol) += sign * fill.quantity;
    cash_ -= sign * fill.price * fill.quantity + fill.commission;
    commission_ += fill.commission;
    // Refresh the snapshot of the current bar so the equity curve reflects the trade.
    if (!equity_.empty()) {
        equity_.pop_back();
        update_timeindex();
    }
}

}  // namespace bt

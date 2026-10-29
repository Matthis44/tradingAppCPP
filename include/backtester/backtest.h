#pragma once

#include <memory>
#include <string>

#include "backtester/data_handler.h"
#include "backtester/events.h"
#include "backtester/portfolio.h"
#include "backtester/strategy.h"

namespace bt {

/// Fills every order immediately at the latest adjusted close (no slippage, no partial fills).
class SimulatedExecutionHandler {
public:
    SimulatedExecutionHandler(std::shared_ptr<DataHandler> data, std::shared_ptr<EventQueue> events)
        : data_(std::move(data)), events_(std::move(events)) {}
    void execute(const OrderEvent& order);

private:
    std::shared_ptr<DataHandler> data_;
    std::shared_ptr<EventQueue> events_;
};

struct BacktestStats {
    double total_return;
    double sharpe;
    double max_drawdown;
    std::size_t max_drawdown_duration;
    int signals;
    int orders;
    int fills;
};

/// Event loop: for each bar, publish it then process every resulting event
/// (MARKET -> SIGNAL -> ORDER -> FILL) before releasing the next bar.
class Backtest {
public:
    Backtest(std::shared_ptr<DataHandler> data, std::shared_ptr<EventQueue> events,
             std::shared_ptr<Strategy> strategy, std::shared_ptr<Portfolio> portfolio, bool verbose = false);

    BacktestStats run(double periods_per_year = 252.0);

private:
    void dispatch(const Event& event);

    std::shared_ptr<DataHandler> data_;
    std::shared_ptr<EventQueue> events_;
    std::shared_ptr<Strategy> strategy_;
    std::shared_ptr<Portfolio> portfolio_;
    SimulatedExecutionHandler execution_;
    bool verbose_;
    int signals_ = 0;
    int orders_ = 0;
    int fills_ = 0;
};

/// Writes the equity curve as CSV.
void write_equity_curve(const Portfolio& portfolio, const std::string& path);

}  // namespace bt

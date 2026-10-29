#include "backtester/backtest.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include "backtester/performance.h"
#include "backtester/time_utils.h"

namespace bt {

void SimulatedExecutionHandler::execute(const OrderEvent& order) {
    const Bar& bar = data_->latest_bar(order.symbol);
    events_->push(std::make_shared<FillEvent>(bar.datetime, order.symbol, order.quantity, order.direction,
                                              bar.adj_close, ib_commission(order.quantity)));
}

Backtest::Backtest(std::shared_ptr<DataHandler> data, std::shared_ptr<EventQueue> events,
                   std::shared_ptr<Strategy> strategy, std::shared_ptr<Portfolio> portfolio, bool verbose)
    : data_(std::move(data)),
      events_(std::move(events)),
      strategy_(std::move(strategy)),
      portfolio_(std::move(portfolio)),
      execution_(data_, events_),
      verbose_(verbose) {}

void Backtest::dispatch(const Event& event) {
    switch (event.type()) {
        case EventType::Market:
            strategy_->calculate_signals(static_cast<const MarketEvent&>(event));
            portfolio_->update_timeindex();
            break;
        case EventType::Signal: {
            const auto& s = static_cast<const SignalEvent&>(event);
            ++signals_;
            if (verbose_)
                std::cout << format_datetime(s.datetime) << "  " << to_string(s.signal) << ' ' << s.symbol << '\n';
            portfolio_->on_signal(s);
            break;
        }
        case EventType::Order:
            ++orders_;
            execution_.execute(static_cast<const OrderEvent&>(event));
            break;
        case EventType::Fill:
            ++fills_;
            portfolio_->on_fill(static_cast<const FillEvent&>(event));
            break;
    }
}

BacktestStats Backtest::run(double periods_per_year) {
    while (data_->continue_backtest()) {
        data_->update_bars();
        while (auto event = events_->pop()) dispatch(*event);
    }

    std::vector<double> totals;
    for (const auto& p : portfolio_->equity_curve()) totals.push_back(p.total);
    if (totals.empty()) throw std::runtime_error("The backtest produced no bars");

    const auto dd = drawdowns(totals);
    return {totals.back() / portfolio_->initial_capital() - 1.0,
            sharpe_ratio(returns_from_values(totals), periods_per_year),
            dd.max_drawdown,
            dd.max_duration,
            signals_,
            orders_,
            fills_};
}

void write_equity_curve(const Portfolio& portfolio, const std::string& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write " + path);
    out << "datetime,cash,market_value,commission,total\n";
    for (const auto& p : portfolio.equity_curve()) {
        out << format_datetime(p.datetime) << ',' << p.cash << ',' << p.market_value << ',' << p.commission << ','
            << p.total << '\n';
    }
}

}  // namespace bt

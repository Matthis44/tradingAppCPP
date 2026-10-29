#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "backtester/bar.h"
#include "backtester/events.h"

namespace bt {

/// Replays historical bars one timestamp at a time.
///
/// Strategies and the portfolio only see bars already published, which rules out
/// look-ahead bias. Subclasses implement `load()`; the base class aligns all symbols on
/// the union of their timestamps and forward-fills gaps, so every symbol has a bar at
/// every step.
class DataHandler {
public:
    DataHandler(std::shared_ptr<EventQueue> events, std::vector<std::string> symbols);
    virtual ~DataHandler() = default;

    /// Loads and aligns the data. Must be called once before the backtest.
    void initialise();

    /// Publishes the next bar of every symbol and emits a MarketEvent.
    void update_bars();
    bool continue_backtest() const { return cursor_ < timeline_.size(); }

    const std::vector<std::string>& symbols() const { return symbols_; }
    const Bar& latest_bar(const std::string& symbol) const;
    /// Values of `field` for the last `n` published bars (fewer during warm-up).
    std::vector<double> latest_values(const std::string& symbol, const std::string& field, std::size_t n) const;
    std::size_t bars_published() const { return cursor_; }

protected:
    /// Returns the raw, time-sorted bars of each symbol.
    virtual std::unordered_map<std::string, std::vector<Bar>> load() = 0;

private:
    std::shared_ptr<EventQueue> events_;
    std::vector<std::string> symbols_;
    std::vector<time_point> timeline_;
    std::unordered_map<std::string, std::vector<Bar>> aligned_;
    std::size_t cursor_ = 0;
};

/// Reads `<directory>/<SYMBOL>.csv` with columns datetime,open,high,low,close,volume,adj_close.
class CsvDataHandler final : public DataHandler {
public:
    CsvDataHandler(std::shared_ptr<EventQueue> events, std::vector<std::string> symbols, std::string directory);

protected:
    std::unordered_map<std::string, std::vector<Bar>> load() override;

private:
    std::string directory_;
};

/// Parses one CSV file; throws on malformed rows. Exposed for tests.
std::vector<Bar> read_bars_csv(const std::string& path);

}  // namespace bt

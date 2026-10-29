// Minimal self-contained test runner (no external dependency).

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "backtester/backtest.h"
#include "backtester/data_handler.h"
#include "backtester/performance.h"
#include "backtester/time_utils.h"

namespace {

std::vector<std::pair<std::string, std::function<void()>>>& registry() {
    static std::vector<std::pair<std::string, std::function<void()>>> tests;
    return tests;
}

struct Register {
    Register(const char* name, std::function<void()> fn) { registry().emplace_back(name, std::move(fn)); }
};

struct Failure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define TEST(name)                                \
    void name();                                  \
    const Register register_##name(#name, name); \
    void name()

#define CHECK(cond)                                                                                         \
    do {                                                                                                    \
        if (!(cond)) throw Failure(std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #cond); \
    } while (0)

#define CHECK_NEAR(a, b) CHECK(std::fabs((a) - (b)) < 1e-9)

#define CHECK_THROWS(expr)            \
    do {                              \
        bool thrown = false;          \
        try {                         \
            expr;                     \
        } catch (const std::exception&) { \
            thrown = true;            \
        }                             \
        CHECK(thrown);                \
    } while (0)

/// Data handler fed directly from memory.
class InMemoryDataHandler final : public bt::DataHandler {
public:
    InMemoryDataHandler(std::shared_ptr<bt::EventQueue> events,
                        std::unordered_map<std::string, std::vector<bt::Bar>> bars)
        : DataHandler(std::move(events), keys(bars)), bars_(std::move(bars)) {}

protected:
    std::unordered_map<std::string, std::vector<bt::Bar>> load() override { return bars_; }

private:
    static std::vector<std::string> keys(const std::unordered_map<std::string, std::vector<bt::Bar>>& m) {
        std::vector<std::string> k;
        for (const auto& [s, _] : m) k.push_back(s);
        std::sort(k.begin(), k.end());
        return k;
    }
    std::unordered_map<std::string, std::vector<bt::Bar>> bars_;
};

std::vector<bt::Bar> make_bars(const std::vector<double>& closes, int first_day = 1) {
    std::vector<bt::Bar> bars;
    for (std::size_t i = 0; i < closes.size(); ++i) {
        bt::Bar b;
        b.datetime = bt::parse_datetime("2024-01-" + std::string(i + first_day < 10 ? "0" : "") +
                                        std::to_string(i + first_day));
        b.open = b.high = b.low = b.close = b.adj_close = closes[i];
        b.volume = 1000;
        bars.push_back(b);
    }
    return bars;
}

// --- time ---------------------------------------------------------------------

TEST(dates_are_parsed_as_utc) {
    CHECK(bt::format_datetime(bt::parse_datetime("2024-03-05")) == "2024-03-05 00:00:00");
    CHECK(bt::format_datetime(bt::parse_datetime("2024-03-05 14:30:00")) == "2024-03-05 14:30:00");
    CHECK_THROWS(bt::parse_datetime("not a date"));
}

// --- performance --------------------------------------------------------------

TEST(sharpe_matches_definition) {
    const std::vector<double> r{0.01, -0.005, 0.002, 0.007, -0.001};
    const double mean = 0.0026;
    double sq = 0;
    for (double x : r) sq += (x - mean) * (x - mean);
    CHECK_NEAR(bt::sharpe_ratio(r, 252), std::sqrt(252.0) * mean / std::sqrt(sq / 4));
    CHECK(std::isnan(bt::sharpe_ratio({0.01, 0.01}, 252)));
}

TEST(drawdown_is_relative_to_peak) {
    const auto dd = bt::drawdowns({1.0, 1.2, 0.9, 1.0, 1.3, 1.1});
    CHECK_NEAR(dd.max_drawdown, 0.25);
    CHECK(dd.max_duration == 2);
    CHECK(bt::drawdowns({1, 2, 3}).max_drawdown == 0.0);
}

// --- data ---------------------------------------------------------------------

TEST(bars_are_published_one_at_a_time) {
    auto events = std::make_shared<bt::EventQueue>();
    InMemoryDataHandler data(events, {{"A", make_bars({10, 11, 12})}});
    data.initialise();

    data.update_bars();
    CHECK(data.latest_bar("A").close == 10);
    CHECK(events->pop()->type() == bt::EventType::Market);
    data.update_bars();
    CHECK(data.latest_values("A", "close", 5) == std::vector<double>({10, 11}));
    data.update_bars();
    CHECK(!data.continue_backtest());
}

TEST(symbols_are_aligned_and_forward_filled) {
    auto events = std::make_shared<bt::EventQueue>();
    auto b = make_bars({20, 21, 22});
    b.erase(b.begin() + 1);  // B has no bar on day 2
    InMemoryDataHandler data(events, {{"A", make_bars({10, 11, 12})}, {"B", b}});
    data.initialise();
    data.update_bars();
    data.update_bars();
    CHECK(data.latest_bar("A").datetime == data.latest_bar("B").datetime);
    CHECK(data.latest_bar("B").close == 20);
}

TEST(missing_symbol_data_is_an_error) {
    auto events = std::make_shared<bt::EventQueue>();
    InMemoryDataHandler data(events, {{"A", {}}});
    CHECK_THROWS(data.initialise());
}

TEST(csv_files_are_read) {
    const auto path = std::filesystem::temp_directory_path() / "bt_test.csv";
    {
        std::ofstream f(path);
        f << "datetime,open,high,low,close,volume,adj_close\n"
             "2024-01-03,2,2.1,1.9,2,100,2\n"
             "2024-01-02,1,1.1,0.9,1,100,0.9\n";
    }
    const auto bars = bt::read_bars_csv(path.string());
    CHECK(bars.size() == 2);
    CHECK(bars[0].adj_close == 0.9);  // sorted by date
    std::filesystem::remove(path);
}

// --- engine -------------------------------------------------------------------

TEST(commission_model) {
    CHECK_NEAR(bt::ib_commission(10), 1.3);
    CHECK_NEAR(bt::ib_commission(500), 6.5);
    CHECK_NEAR(bt::ib_commission(1000), 8.0);
}

TEST(end_to_end_profit_and_loss) {
    // Rises from 10 to 20 then falls back to 10.
    std::vector<double> prices{10, 10, 10, 10};
    for (int p = 11; p <= 20; ++p) prices.push_back(p);
    for (int p = 19; p >= 10; --p) prices.push_back(p);
    std::vector<bt::Bar> bars;
    for (std::size_t i = 0; i < prices.size(); ++i) {
        bt::Bar b;
        b.datetime = bt::parse_datetime("2024-01-01") + std::chrono::hours(24 * i);
        b.open = b.high = b.low = b.close = b.adj_close = prices[i];
        bars.push_back(b);
    }

    auto events = std::make_shared<bt::EventQueue>();
    auto data = std::make_shared<InMemoryDataHandler>(events, std::unordered_map<std::string, std::vector<bt::Bar>>{
                                                                  {"A", bars}});
    data->initialise();
    auto strategy = std::make_shared<bt::MovingAverageCrossStrategy>(data, events, 2, 4);
    auto portfolio = std::make_shared<bt::Portfolio>(data, events, 10'000, 10);
    const auto stats = bt::Backtest(data, events, strategy, portfolio).run();

    CHECK(stats.signals == 2 && stats.orders == 2 && stats.fills == 2);
    CHECK(portfolio->position("A") == 0);
    // Buy at 11 (bar 4: SMA2=10.5 > SMA4=10.25), sell at 18 (bar 15: SMA2=18.5 < SMA4=19).
    const double expected = 10'000 + 10 * (18 - 11) - 2 * 1.3;
    CHECK_NEAR(portfolio->equity_curve().back().total, expected);
    CHECK_NEAR(stats.total_return, expected / 10'000 - 1);
}

TEST(invalid_windows_are_rejected) {
    auto events = std::make_shared<bt::EventQueue>();
    auto data = std::make_shared<InMemoryDataHandler>(events, std::unordered_map<std::string, std::vector<bt::Bar>>{
                                                                  {"A", make_bars({1, 2})}});
    CHECK_THROWS(bt::MovingAverageCrossStrategy(data, events, 5, 3));
}

}  // namespace

int main() {
    int failed = 0;
    for (const auto& [name, fn] : registry()) {
        try {
            fn();
            std::cout << "[ OK ] " << name << '\n';
        } catch (const std::exception& e) {
            ++failed;
            std::cout << "[FAIL] " << name << ": " << e.what() << '\n';
        }
    }
    std::cout << registry().size() - failed << "/" << registry().size() << " tests passed\n";
    return failed == 0 ? 0 : 1;
}

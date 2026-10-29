#include "backtester/data_handler.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "backtester/time_utils.h"

namespace bt {

DataHandler::DataHandler(std::shared_ptr<EventQueue> events, std::vector<std::string> symbols)
    : events_(std::move(events)), symbols_(std::move(symbols)) {
    if (symbols_.empty()) throw std::invalid_argument("At least one symbol is required");
}

void DataHandler::initialise() {
    auto raw = load();

    for (const auto& symbol : symbols_) {
        auto it = raw.find(symbol);
        if (it == raw.end() || it->second.empty()) {
            throw std::runtime_error("No data found for " + symbol);
        }
        for (const auto& bar : it->second) timeline_.push_back(bar.datetime);
    }
    std::sort(timeline_.begin(), timeline_.end());
    timeline_.erase(std::unique(timeline_.begin(), timeline_.end()), timeline_.end());

    // Forward-fill each symbol on the common timeline. Before a symbol's first bar we
    // back-fill with that first bar so that every step has a value for every symbol.
    for (const auto& symbol : symbols_) {
        const auto& bars = raw[symbol];
        std::vector<Bar> out;
        out.reserve(timeline_.size());
        std::size_t i = 0;
        for (const auto& t : timeline_) {
            while (i + 1 < bars.size() && bars[i + 1].datetime <= t) ++i;
            Bar b = bars[i];
            b.datetime = t;
            out.push_back(b);
        }
        aligned_[symbol] = std::move(out);
    }
}

void DataHandler::update_bars() {
    if (!continue_backtest()) return;
    ++cursor_;
    events_->push(std::make_shared<MarketEvent>());
}

const Bar& DataHandler::latest_bar(const std::string& symbol) const {
    if (cursor_ == 0) throw std::logic_error("No bar published yet");
    auto it = aligned_.find(symbol);
    if (it == aligned_.end()) throw std::out_of_range("Unknown symbol: " + symbol);
    return it->second[cursor_ - 1];
}

std::vector<double> DataHandler::latest_values(const std::string& symbol, const std::string& field,
                                               std::size_t n) const {
    auto it = aligned_.find(symbol);
    if (it == aligned_.end()) throw std::out_of_range("Unknown symbol: " + symbol);
    const std::size_t count = std::min(n, cursor_);
    std::vector<double> values;
    values.reserve(count);
    for (std::size_t i = cursor_ - count; i < cursor_; ++i) values.push_back(it->second[i].value(field));
    return values;
}

// --- CSV --------------------------------------------------------------------

std::vector<Bar> read_bars_csv(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open " + path);

    std::vector<Bar> bars;
    std::string line;
    std::getline(file, line);  // header
    std::size_t line_number = 1;
    while (std::getline(file, line)) {
        ++line_number;
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, ',')) fields.push_back(field);
        if (fields.size() != 7) {
            throw std::runtime_error(path + ":" + std::to_string(line_number) + ": expected 7 columns");
        }
        Bar b;
        b.datetime = parse_datetime(fields[0]);
        b.open = std::stod(fields[1]);
        b.high = std::stod(fields[2]);
        b.low = std::stod(fields[3]);
        b.close = std::stod(fields[4]);
        b.volume = std::stod(fields[5]);
        b.adj_close = std::stod(fields[6]);
        bars.push_back(b);
    }
    std::sort(bars.begin(), bars.end(), [](const Bar& a, const Bar& b) { return a.datetime < b.datetime; });
    return bars;
}

CsvDataHandler::CsvDataHandler(std::shared_ptr<EventQueue> events, std::vector<std::string> symbols,
                               std::string directory)
    : DataHandler(std::move(events), std::move(symbols)), directory_(std::move(directory)) {}

std::unordered_map<std::string, std::vector<Bar>> CsvDataHandler::load() {
    std::unordered_map<std::string, std::vector<Bar>> data;
    for (const auto& symbol : symbols()) data[symbol] = read_bars_csv(directory_ + "/" + symbol + ".csv");
    return data;
}

}  // namespace bt

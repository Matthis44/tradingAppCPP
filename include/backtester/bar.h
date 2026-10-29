#pragma once

#include <chrono>
#include <stdexcept>
#include <string>

namespace bt {

using time_point = std::chrono::system_clock::time_point;

/// One OHLCV bar. `adj_close` equals `close` for sources without corporate action adjustments.
struct Bar {
    time_point datetime{};
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    double adj_close = 0.0;
    double volume = 0.0;

    double value(const std::string& field) const {
        if (field == "open") return open;
        if (field == "high") return high;
        if (field == "low") return low;
        if (field == "close") return close;
        if (field == "adj_close") return adj_close;
        if (field == "volume") return volume;
        throw std::invalid_argument("Unknown bar field: " + field);
    }
};

}  // namespace bt

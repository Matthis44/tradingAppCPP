#include "backtester/events.h"

#include <algorithm>

namespace bt {

std::string to_string(SignalType s) {
    switch (s) {
        case SignalType::Long: return "LONG";
        case SignalType::Short: return "SHORT";
        case SignalType::Exit: return "EXIT";
    }
    return "?";
}

std::string to_string(Direction d) { return d == Direction::Buy ? "BUY" : "SELL"; }

double ib_commission(int quantity) {
    const double rate = quantity <= 500 ? 0.013 : 0.008;
    return std::max(1.3, rate * quantity);
}

}  // namespace bt

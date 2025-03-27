#ifndef PORTFOLIO_TYPES_H
#define PORTFOLIO_TYPES_H

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

struct all_positions {
    std::unordered_map<std::string, int> positions;
    std::chrono::system_clock::time_point datetime;
};

struct all_holdings {
    std::unordered_map<std::string, int> holdings;
    std::chrono::system_clock::time_point datetime;
    double cash = 0.0;
    double commission = 0.0;
    double total = 0.0;
};

struct current_positions {
    std::unordered_map<std::string, int> positions;
    std::chrono::system_clock::time_point datetime;
};

struct current_holdings {
    std::unordered_map<std::string, int> holdings;
    std::chrono::system_clock::time_point datetime;
    double cash = 0.0;
    double commission = 0.0;
    double total = 0.0;
};

#endif // PORTFOLIO_TYPES_H


#include "backtester/performance.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace bt {

std::vector<double> returns_from_values(const std::vector<double>& values) {
    std::vector<double> r;
    for (std::size_t i = 1; i < values.size(); ++i) r.push_back(values[i] / values[i - 1] - 1.0);
    return r;
}

double sharpe_ratio(const std::vector<double>& returns, double periods_per_year) {
    const auto n = returns.size();
    if (n < 2) return std::numeric_limits<double>::quiet_NaN();
    const double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / static_cast<double>(n);
    double sq = 0.0;
    for (double r : returns) sq += (r - mean) * (r - mean);
    const double stdev = std::sqrt(sq / static_cast<double>(n - 1));  // sample standard deviation
    if (stdev == 0.0) return std::numeric_limits<double>::quiet_NaN();
    return std::sqrt(periods_per_year) * mean / stdev;
}

DrawdownStats drawdowns(const std::vector<double>& values) {
    DrawdownStats stats;
    double peak = -std::numeric_limits<double>::infinity();
    std::size_t current = 0;
    for (double v : values) {
        if (v >= peak) {
            peak = v;
            current = 0;
        } else {
            ++current;
            stats.max_drawdown = std::max(stats.max_drawdown, 1.0 - v / peak);
            stats.max_duration = std::max(stats.max_duration, current);
        }
    }
    return stats;
}

}  // namespace bt

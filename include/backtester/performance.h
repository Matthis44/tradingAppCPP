#pragma once

#include <cstddef>
#include <vector>

namespace bt {

/// Simple returns r_t = v_t / v_{t-1} - 1.
std::vector<double> returns_from_values(const std::vector<double>& values);

/// Annualised Sharpe ratio (risk-free rate 0). Returns NaN when volatility is zero.
double sharpe_ratio(const std::vector<double>& returns, double periods_per_year);

struct DrawdownStats {
    double max_drawdown = 0.0;     ///< fraction below the running peak, e.g. 0.25 = -25%
    std::size_t max_duration = 0;  ///< longest stretch below the peak, in bars
};

DrawdownStats drawdowns(const std::vector<double>& values);

}  // namespace bt

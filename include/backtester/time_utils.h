#pragma once

#include <string>

#include "backtester/bar.h"

namespace bt {

/// Parses "YYYY-MM-DD" or "YYYY-MM-DD HH:MM:SS" as a UTC timestamp.
time_point parse_datetime(const std::string& text);

/// Formats a timestamp as "YYYY-MM-DD HH:MM:SS" (UTC).
std::string format_datetime(time_point tp);

}  // namespace bt

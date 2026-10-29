# Event-driven backtester in C++

[![ci](https://github.com/Matthis44/tradingAppCPP/actions/workflows/ci.yml/badge.svg)](https://github.com/Matthis44/tradingAppCPP/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![CMake](https://img.shields.io/badge/build-CMake-informational)
![License](https://img.shields.io/badge/license-MIT-green)

A C++17 port of my [Python event-driven backtester](https://github.com/Matthis44/TradingAppPY),
with a Python ingestion layer that loads **intraday bars from the Alpaca API** into a MySQL
securities master, and a Streamlit dashboard to explore them.

**Both engines produce identical results** on the same data (same trades, return, Sharpe and
drawdown), which cross-validates the two implementations.

## Highlights

- **Event-driven engine**: market, signal, order and fill events on a single queue; each bar's
  events are fully processed before the next bar is released, so look-ahead bias is impossible.
- **Multi-symbol alignment**: symbols are aligned on the union of their timestamps and gaps are
  forward-filled.
- **Two data sources** behind one interface: CSV files, or MySQL through Connector/C++
  (optional at build time, so the project builds anywhere).
- **Intraday data pipeline** (Python): Alpaca bars for stocks, ETFs and crypto, upserted on
  `(vendor, symbol, interval, timestamp)` so reloads are idempotent; UTC timestamps throughout.
- **Analytics**: total return, annualised Sharpe ratio, drawdown relative to the high-water mark
  and its longest duration, equity curve exported to CSV.
- **Tooling**: CMake, `-Wall -Wextra -Wpedantic` with zero warnings, unit tests (P&L checked by
  hand), GitHub Actions building with and without MySQL.

## Architecture

```
 Alpaca API ──▶ ingestion/alpaca_ingest.py ──▶ MySQL price_bar ◀── dashboard.py (Streamlit)
                                                     │
                                CSV files ───┐       │
                                             ▼       ▼
            ┌──────────────┐  MARKET   ┌──────────┐  SIGNAL   ┌───────────┐
            │ DataHandler  │ ────────▶ │ Strategy │ ────────▶ │ Portfolio │
            │ CSV / MySQL  │           └──────────┘           └───────────┘
            └──────────────┘                  FILL ▲            │ ORDER
                   ▲ latest bars   ┌──────────────────────┐     │
                   └────────────── │ SimulatedExecution   │ ◀───┘
                                   └──────────────────────┘
```

| Path | Content |
|---|---|
| `include/backtester/`, `src/` | engine library: events, data handlers, strategy, portfolio, performance |
| `apps/main.cpp` | command-line backtest |
| `tests/` | unit and end-to-end tests (self-contained runner, no dependency) |
| `ingestion/` | Alpaca → MySQL loader and Streamlit dashboard |
| `sql/schema.sql` | securities master schema |
| `data/sample/` | synthetic sample (geometric Brownian motion, **not** market data) |

## Build and run

Requirements: a C++17 compiler and CMake ≥ 3.16.

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure

./build/backtest                                  # synthetic sample
./build/backtest --short 20 --long 50 -v          # print every signal
./build/backtest --csv-dir path/to/csv AAPL MSFT  # your own CSV files
```

CSV format: `datetime,open,high,low,close,volume,adj_close` with dates as `YYYY-MM-DD` or
`YYYY-MM-DD HH:MM:SS` (UTC).

### With MySQL and Alpaca data

```bash
sudo apt install libmysqlcppconn-dev
cmake -S . -B build -DWITH_MYSQL=ON && cmake --build build -j

cp .env.example .env                    # MySQL credentials and Alpaca API keys
mysql -u root -p < sql/schema.sql
pip install -r ingestion/requirements.txt
python ingestion/alpaca_ingest.py AAPL --interval 5Min --start 2024-01-01 --end 2024-03-01

# 5-minute bars: ~78 per US trading day, so ~19,656 per year for the Sharpe ratio
./build/backtest --source mysql --interval 5Min --start 2024-01-01 --periods 19656 AAPL

streamlit run ingestion/dashboard.py    # optional: browse the stored bars
```

## Simplifying assumptions

Orders are filled immediately at the latest price with no slippage or partial fills, positions use
a fixed lot size, and commissions follow a fixed Interactive Brokers style schedule.

## License

MIT

// Command-line entry point.
//
//   backtest                                   # bundled synthetic sample
//   backtest --csv-dir data AAPL MSFT --short 20 --long 50
//   backtest --source mysql --interval 5Min --start 2024-01-01 AAPL   (needs -DWITH_MYSQL=ON)

#include <cmath>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "backtester/backtest.h"
#include "backtester/data_handler.h"
#ifdef BACKTESTER_WITH_MYSQL
#include "backtester/mysql_data_handler.h"
#endif

namespace {

struct Options {
    std::string source = "csv";
    std::string csv_dir = "data/sample";
    std::string interval = "1Day";
    std::string start = "1970-01-01";
    std::string output = "equity_curve.csv";
    std::vector<std::string> symbols;
    std::size_t short_window = 40;
    std::size_t long_window = 100;
    double capital = 100'000.0;
    double periods_per_year = 252.0;
    bool verbose = false;
};

void usage() {
    std::cout << "Usage: backtest [options] [SYMBOL...]\n"
                 "  --source csv|mysql   data source (default csv)\n"
                 "  --csv-dir DIR        directory with <SYMBOL>.csv files (default data/sample)\n"
                 "  --interval I         bar interval for MySQL, e.g. 5Min, 1Day (default 1Day)\n"
                 "  --start YYYY-MM-DD   first date for MySQL\n"
                 "  --short N --long N   SMA windows (default 40 / 100)\n"
                 "  --capital X          initial capital (default 100000)\n"
                 "  --periods N          bars per year for the Sharpe ratio (default 252)\n"
                 "  --output FILE        equity curve CSV (default equity_curve.csv)\n"
                 "  -v, --verbose        print every signal\n";
}

Options parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) throw std::invalid_argument(arg + " expects a value");
            return argv[++i];
        };
        if (arg == "-h" || arg == "--help") {
            usage();
            std::exit(0);
        } else if (arg == "--source") o.source = next();
        else if (arg == "--csv-dir") o.csv_dir = next();
        else if (arg == "--interval") o.interval = next();
        else if (arg == "--start") o.start = next();
        else if (arg == "--short") o.short_window = std::stoul(next());
        else if (arg == "--long") o.long_window = std::stoul(next());
        else if (arg == "--capital") o.capital = std::stod(next());
        else if (arg == "--periods") o.periods_per_year = std::stod(next());
        else if (arg == "--output") o.output = next();
        else if (arg == "-v" || arg == "--verbose") o.verbose = true;
        else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("Unknown option " + arg);
        else o.symbols.push_back(arg);
    }
    if (o.symbols.empty()) o.symbols = {"SYNTH"};
    return o;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options opt = parse(argc, argv);
        auto events = std::make_shared<bt::EventQueue>();

        std::shared_ptr<bt::DataHandler> data;
        if (opt.source == "csv") {
            data = std::make_shared<bt::CsvDataHandler>(events, opt.symbols, opt.csv_dir);
        } else if (opt.source == "mysql") {
#ifdef BACKTESTER_WITH_MYSQL
            data = std::make_shared<bt::MySqlDataHandler>(events, opt.symbols, bt::MySqlConfig::from_env(),
                                                          opt.interval, opt.start);
#else
            throw std::runtime_error("Built without MySQL support: reconfigure with -DWITH_MYSQL=ON");
#endif
        } else {
            throw std::invalid_argument("Unknown source " + opt.source);
        }
        data->initialise();

        auto strategy =
            std::make_shared<bt::MovingAverageCrossStrategy>(data, events, opt.short_window, opt.long_window);
        auto portfolio = std::make_shared<bt::Portfolio>(data, events, opt.capital);
        bt::Backtest backtest(data, events, strategy, portfolio, opt.verbose);

        const auto s = backtest.run(opt.periods_per_year);
        bt::write_equity_curve(*portfolio, opt.output);

        std::printf("Bars             %9zu\n", data->bars_published());
        std::printf("Total return     %8.2f%%\n", 100.0 * s.total_return);
        std::printf("Sharpe ratio     %9.2f\n", s.sharpe);
        std::printf("Max drawdown     %8.2f%%\n", 100.0 * s.max_drawdown);
        std::printf("Longest drawdown %6zu bars\n", s.max_drawdown_duration);
        std::printf("Trades           %9d\n", s.fills);
        std::printf("\nEquity curve saved to %s\n", opt.output.c_str());
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}

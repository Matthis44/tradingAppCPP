#include "backtest.h"
#include "mysql_data_handler.h"
#include "sma.h"
#include "sma.h"
#include "portfolio.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <utilities.h>

int main() {
    // === CONFIGURATION ===
    const std::vector<std::string> SYMBOLS = {"AAPL"};
    const std::string start_date = "2000-01-01";
    std::chrono::system_clock::time_point START_DATE = parse_date(start_date);  // via fonction ci-dessous

    const double INITIAL_CAPITAL = 10000.0;
    const int HEARTBEAT = 0; // En millisecondes (0 pour instantané)

    // Paramètres MySQL
    const std::string USER = "sec_user";
    const std::string PASSWORD = "1Lo2Ma3Is@";
    const std::string HOST = "localhost";
    const std::string DB = "securities_master";

    auto events = std::make_shared<event_queue>(); 

    auto data_handler = std::make_shared<mysql_data_handler>(
        events, SYMBOLS, USER, PASSWORD, HOST, DB);

    auto execution_handler = std::make_shared<simulated_execution_handler>(events);

    auto portfolio_ptr = std::make_shared<portfolio>(
        data_handler, events, START_DATE, INITIAL_CAPITAL);

    int short_window = 40;
    int long_window = 100;

    auto strategy_ptr = std::make_shared<simple_moving_average_strategy>(
        data_handler, events, short_window, long_window);

    // === LANCEMENT DU BACKTEST ===
    std::cout << "Démarrage du backtest..." << std::endl;

    Backtest bt(SYMBOLS,
                INITIAL_CAPITAL,
                HEARTBEAT,
                START_DATE,
                data_handler,
                execution_handler,
                portfolio_ptr,
                strategy_ptr,
                events);

    bt.simulate_trading();

    std::cout << "Backtest terminé." << std::endl;

    return 0;
}
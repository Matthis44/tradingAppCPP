#include "backtest.h"
#include "mysql_data_handler.h"
#include "sma.h"
#include "sma.h"
#include "portfolio.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "env_loader.h"
#include <utilities.h>
#include <filesystem>

int main() {
    // === CONFIGURATION ===
    const std::vector<std::string> SYMBOLS = {"AAPL"};
    const std::string start_date = "2024-01-01";
    std::chrono::system_clock::time_point START_DATE = parse_date(start_date);  // via fonction ci-dessous
    const std::string interval= "5Min";
    const double INITIAL_CAPITAL = 10000.0;
    const int HEARTBEAT = 0; // En millisecondes (0 pour instantané)

    // Paramètres MySQL
    std::string path = std::filesystem::current_path();  // le dossier courant à l’exécution
    auto env = load_env_file(path + "/.env");
    const std::string USER = env["MYSQL_USER"];
    const std::string PASSWORD = env["MYSQL_PASSWORD"];
    const std::string HOST = env["MYSQL_HOST"];
    const std::string DB = env["MYSQL_DATABASE"];

    auto events = std::make_shared<event_queue>(); 

    auto data_handler = std::make_shared<mysql_data_handler>(
        events, SYMBOLS,interval,start_date, USER, PASSWORD, HOST, DB);

    auto execution_handler = std::make_shared<simulated_execution_handler>(events);

    auto portfolio_ptr = std::make_shared<portfolio>(
        data_handler, events, START_DATE, INITIAL_CAPITAL);

    int short_window = 50;
    int long_window = 150;

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
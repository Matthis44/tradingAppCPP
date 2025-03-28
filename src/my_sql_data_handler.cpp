#include "mysql_data_handler.h"
#include "event.h"
#include "utilities.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace std;

mysql_data_handler::mysql_data_handler(std::shared_ptr<event_queue> events,
                                       const vector<string>& symbol_list,
                                       const std::string& interval,
                                       const std::string& start_date,
                                       const string& user,
                                       const string& password,
                                       const string& host,
                                       const string& db)
    : events_(events), symbol_list_(symbol_list),interval_(interval),start_date_(start_date),
      user_(user), password_(password), host_(host), db_(db) {
    
    try {
        sql::Driver* driver = get_driver_instance();
        conn_.reset(driver->connect(host_, user_, password_));
        conn_->setSchema(db_);
        cout << "[INFO] Connexion à la base de données '" << db_ << "' réussie avec l'utilisateur '" << user_ << "'." << endl;

        load_data_from_db();
    } catch (sql::SQLException& e) {
        cerr << "MySQL connection error: " << e.what() << endl;
        throw;
    }
}

void mysql_data_handler::load_data_from_db() {
    for (const auto& symbol : symbol_list_) {
        cout << "[INFO] Chargement des données pour : " << symbol << endl;
        symbol_data_[symbol] = fetch_symbol_bars(symbol);
        symbol_index_[symbol] = 0;
        cout << "[INFO] -> " << symbol_data_[symbol].size() << " lignes chargées pour " << symbol << endl;
    }
}


vector<mysql_bar_row> mysql_data_handler::fetch_symbol_bars(const string& symbol) {
    vector<mysql_bar_row> bars;

    try {
        string query = R"(
            SELECT pb.timestamp AS datetime,
                   pb.open_price AS open,
                   pb.high_price AS high,
                   pb.low_price AS low,
                   pb.close_price AS close,
                   pb.volume AS volume,
                   pb.trade_count AS trade_count,
                   pb.vwap AS vwap
            FROM price_bar pb
            JOIN symbol s ON pb.symbol_id = s.id
            WHERE s.ticker = ?
            AND pb.`interval` = ?
            AND pb.timestamp >= ?
            ORDER BY pb.timestamp ASC
        )";

        unique_ptr<sql::PreparedStatement> stmt(conn_->prepareStatement(query));
        stmt->setString(1, symbol);
        stmt->setString(2, interval_); 
        stmt->setString(3, start_date_);
        unique_ptr<sql::ResultSet> res(stmt->executeQuery());

        // Ici tu peux parser les résultats et remplir ton vector<mysql_bar_row>

        while (res->next()) {
            mysql_bar_row bar;
            std::string datetime_str = res->getString("datetime");
            bar.datetime = parse_date(datetime_str);  // maintenant en time_point
            bar.open       = res->getDouble("open");
            bar.high       = res->getDouble("high");
            bar.low        = res->getDouble("low");
            bar.close      = res->getDouble("close");
            bar.adj_close  = res->getDouble("close");
            bar.volume     = res->getDouble("volume");
            bars.push_back(bar);
        }

    
    } catch (sql::SQLException &e) {
        cerr << "SQL error: " << e.what() << endl;
    } catch (std::exception &e) {
        cerr << "Erreur générique: " << e.what() << endl;
    }
    if (!bars.empty()) {
        const auto& bar = bars.front();
        std::time_t tt=std::chrono::system_clock::to_time_t(bar.datetime);
    }
    
    return bars;
}

void mysql_data_handler::update_bars() {
    for (const auto& symbol : symbol_list_) {
        auto& bars = symbol_data_[symbol];
        auto& index = symbol_index_[symbol];

        if (index < bars.size()) {
            latest_symbol_data_[symbol].push_back(bars[index]);
            index++;
        } else {
            cout << "End of data for symbol: " << symbol << endl;
        }
    }
    events_->push(make_shared<market_event>());

}

mysql_bar_row mysql_data_handler::get_latest_bar(const string& symbol) const {
    const auto& bars = latest_symbol_data_.at(symbol);
    if (!bars.empty()) {
        return bars.back();
    }
    return mysql_bar_row{};
}

std::vector<mysql_bar_row> mysql_data_handler::get_latest_bars(const string& symbol, int N) const {
    vector<mysql_bar_row> datetimes;
    const auto& bars = latest_symbol_data_.at(symbol);
    int count = min(N, static_cast<int>(bars.size()));

    for (int i = bars.size() - count; i < bars.size(); ++i) {
        datetimes.push_back(bars[i]);
    }
    return datetimes;
}

std::chrono::system_clock::time_point mysql_data_handler::get_latest_bar_datetime(const std::string& symbol) const {
    const auto& bars = latest_symbol_data_.at(symbol);
    return bars.back().datetime;
}

double mysql_data_handler::get_latest_bar_value(const string& symbol, const string& val_type) const {
    const auto& bars = latest_symbol_data_.at(symbol);
    if (bars.empty()) return 0.0;

    const auto& bar = bars.back();
    if (val_type == "open") return bar.open;
    if (val_type == "high") return bar.high;
    if (val_type == "low") return bar.low;
    if (val_type == "close") return bar.close;
    if (val_type == "adj_close") return bar.adj_close;
    if (val_type == "volume") return bar.volume;

    return 0.0;
}

vector<double> mysql_data_handler::get_latest_bars_values(const string& symbol, const string& val_type, int N) const {
    vector<double> values;
    const auto& bars = latest_symbol_data_.at(symbol);
    int count = min(N, static_cast<int>(bars.size()));

    for (int i = bars.size() - count; i < bars.size(); ++i) {
        const auto& bar = bars[i];
        if (val_type == "open") values.push_back(bar.open);
        else if (val_type == "high") values.push_back(bar.high);
        else if (val_type == "low") values.push_back(bar.low);
        else if (val_type == "close") values.push_back(bar.close);
        else if (val_type == "adj_close") values.push_back(bar.adj_close);
        else if (val_type == "volume") values.push_back(bar.volume);
    }

    return values;
}

vector<string> mysql_data_handler::get_symbol_list() const {
    return symbol_list_; // Retourne les symboles demandés à l'initialisation
}

bool mysql_data_handler::continue_backtest() const {
    // Exemple simple : backtest continue tant que toutes les données ne sont pas épuisées
    for (const auto& symbol : symbol_list_) {
        if (symbol_index_.at(symbol) < symbol_data_.at(symbol).size()) {
            return true;  // Il reste des données
        }
    }
    return false;  // Plus aucune donnée disponible
}


#include "backtester/mysql_data_handler.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <memory>

#include "backtester/time_utils.h"

namespace bt {

MySqlDataHandler::MySqlDataHandler(std::shared_ptr<EventQueue> events, std::vector<std::string> symbols,
                                   MySqlConfig config, std::string interval, std::string start_date)
    : DataHandler(std::move(events), std::move(symbols)),
      config_(std::move(config)),
      interval_(std::move(interval)),
      start_date_(std::move(start_date)) {}

std::unordered_map<std::string, std::vector<Bar>> MySqlDataHandler::load() {
    static constexpr const char* query = R"(
        SELECT pb.timestamp, pb.open_price, pb.high_price, pb.low_price, pb.close_price, pb.volume
        FROM price_bar pb
        JOIN symbol s ON pb.symbol_id = s.id
        WHERE s.ticker = ? AND pb.`interval` = ? AND pb.timestamp >= ?
        ORDER BY pb.timestamp)";

    // SQL errors propagate: a backtest on silently missing data would be misleading.
    sql::Driver* driver = get_driver_instance();
    std::unique_ptr<sql::Connection> conn(driver->connect(config_.host, config_.user, config_.password));
    conn->setSchema(config_.database);

    std::unordered_map<std::string, std::vector<Bar>> data;
    for (const auto& symbol : symbols()) {
        std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(query));
        stmt->setString(1, symbol);
        stmt->setString(2, interval_);
        stmt->setString(3, start_date_);
        std::unique_ptr<sql::ResultSet> rows(stmt->executeQuery());

        auto& bars = data[symbol];
        while (rows->next()) {
            Bar b;
            b.datetime = parse_datetime(rows->getString("timestamp"));
            b.open = rows->getDouble("open_price");
            b.high = rows->getDouble("high_price");
            b.low = rows->getDouble("low_price");
            b.close = rows->getDouble("close_price");
            b.adj_close = b.close;  // intraday bars are not adjusted
            b.volume = rows->getDouble("volume");
            bars.push_back(b);
        }
    }
    return data;
}

}  // namespace bt

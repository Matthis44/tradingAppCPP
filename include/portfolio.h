#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include <memory>
#include <string>
#include <vector>
#include "portfolio_types.h"   // ⬅️ tes struct all_positions, all_holdings, etc.
#include "event.h"
#include "event_queue.h"
#include "bar_row_use.h"

class portfolio {
public:
    portfolio(std::shared_ptr<my_data_handler> data,
              std::shared_ptr<event_queue> events,
              std::chrono::system_clock::time_point& start_date,
              double initial_capital = 100000.0);

    void update_timeindex(const std::shared_ptr<event>& ev);
    void update_fill(const std::shared_ptr<event>& ev);
    void update_signal(const std::shared_ptr<event>& ev);
    double get_total_pnl() const;
    double get_total_pnl_pct() const;
    double get_max_drawdown() const;

private:
    std::shared_ptr<my_data_handler> data_;
    std::shared_ptr<event_queue> events_;
    std::vector<std::string> symbol_list_;
    std::chrono::system_clock::time_point start_date_;
    double initial_capital_;

    std::vector<current_positions> all_positions_;   
    current_positions current_positions_;        

    std::vector<current_holdings> all_holdings_;     
    current_holdings current_holdings_;          

    void update_positions_from_fill(const std::shared_ptr<fill_event>& fill);
    void update_holdings_from_fill(const std::shared_ptr<fill_event>& fill);
    std::shared_ptr<order_event> generate_naive_order(const std::shared_ptr<signal_event>& signal);
};

#endif // PORTFOLIO_H



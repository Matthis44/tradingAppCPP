#include "portfolio.h"
#include <iostream>
#include <cmath>

portfolio::portfolio(std::shared_ptr<my_data_handler> data,
                    std::shared_ptr<event_queue> events,
                    std::chrono::system_clock::time_point& start_date,
                    double initial_capital)
    : data_(data), events_(events), start_date_(start_date), initial_capital_(initial_capital) {
    
    // Récupère la liste des symboles à partir de data handler
    symbol_list_ = data_->get_symbol_list();

    // Initialisation des positions
    current_positions_.datetime= start_date_;
    for (const auto& symbol : symbol_list_) {
        current_positions_.positions[symbol] = 0;
    }
    //Init Holding
    current_holdings_.datetime = start_date_;
    for (const auto& symbol : symbol_list_) {
        current_holdings_.holdings[symbol] = 0;
    }
    current_holdings_.cash = initial_capital_;
    current_holdings_.commission = 0.0;
    current_holdings_.total = initial_capital_;

    // Création du premier snapshot all_positions_
    current_positions init_pos;
    init_pos.datetime = start_date_;
    for (const auto& symbol : symbol_list_) {
        init_pos.positions[symbol] = 0;
    }
    all_positions_.push_back(init_pos);

    // Création du premier snapshot all_holdings_
    current_holdings init_hold;
    init_hold.datetime = std::chrono::system_clock::now();
    for (const auto& symbol : symbol_list_) {
           init_hold.holdings[symbol] = 0;
    }
    init_hold.cash = initial_capital_;
    init_hold.commission = 0.0;
    init_hold.total = initial_capital_;
    all_holdings_.push_back(init_hold);

}

void portfolio::update_timeindex(const std::shared_ptr<event>& ev) {
    std::chrono::system_clock::time_point dt = data_->get_latest_bar_datetime(symbol_list_[0]);

    current_positions pos;
    pos.datetime=dt;
    pos.positions = current_positions_.positions;
    all_positions_.push_back(pos);

    // Mise à jour des avoirs
    current_holdings hold;
    hold.datetime = dt;
    hold.cash = current_holdings_.cash;
    hold.commission = current_holdings_.commission;
    hold.total = current_holdings_.cash;

    for (const auto& s : symbol_list_) {
        double market_value = current_positions_.positions[s] * data_->get_latest_bar_value(s, "adj_close");
        hold.holdings[s] = market_value;
        hold.total += market_value;
    }

    all_holdings_.push_back(hold);
}

void portfolio::update_positions_from_fill(const std::shared_ptr<fill_event>& fill) {
    int direction = fill->get_direction() == "BUY" ? 1 : -1;
    current_positions_.positions[fill->get_symbol()] += direction * fill->get_quantity();
}

void portfolio::update_holdings_from_fill(const std::shared_ptr<fill_event>& fill) {
    int direction = fill->get_direction() == "BUY" ? 1 : -1;
    double price = data_->get_latest_bar_value(fill->get_symbol(), "adj_close");
    double cost = direction * price * fill->get_quantity();

    current_holdings_.holdings[fill->get_symbol()] += cost;
    current_holdings_.commission += fill->get_commission();
    current_holdings_.cash -= (cost + fill->get_commission());
    current_holdings_.total -= (cost + fill->get_commission());
}

void portfolio::update_fill(const std::shared_ptr<event>& ev) {
    if (ev->get_type() == "FILL") {
        auto fill = std::dynamic_pointer_cast<fill_event>(ev);
        update_positions_from_fill(fill);
        update_holdings_from_fill(fill);
    }
}

std::shared_ptr<order_event> portfolio::generate_naive_order(const std::shared_ptr<signal_event>& signal) {
    std::string symbol = signal->get_symbol();
    std::string direction = signal->get_signal_type();
    int current_qty = current_positions_.positions[symbol];
    std::string order_type = "MKT";
    int quantity = 100;

    if (direction == "LONG" && current_qty == 0) {
        return std::make_shared<order_event>(symbol, order_type, quantity, "BUY");
    } else if (direction == "SHORT" && current_qty == 0) {
        return std::make_shared<order_event>(symbol, order_type, quantity, "SELL");
    } else if (direction == "EXIT") {
        if (current_qty > 0)
            return std::make_shared<order_event>(symbol, order_type, std::abs(current_qty), "SELL");
        else if (current_qty < 0)
            return std::make_shared<order_event>(symbol, order_type, std::abs(current_qty), "BUY");
    }

    return nullptr;
}

void portfolio::update_signal(const std::shared_ptr<event>& ev) {
    if (ev->get_type() == "SIGNAL") {
        auto signal = std::dynamic_pointer_cast<signal_event>(ev);
        auto order = generate_naive_order(signal);
        if (order) {
            events_->push(order);
        }
    }
}

double portfolio::get_total_pnl() const {
    if (all_holdings_.empty()) return 0.0;

    double final_total = all_holdings_.back().total;
    return final_total - initial_capital_;
}

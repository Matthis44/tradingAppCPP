

#include "strategy.h"
#include "bar_row_use.h"
#include "event_queue.h"
#include "event.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <numeric>

#include "backtest.h"
#include <iostream>
#include <thread>
#include <chrono>

Backtest::Backtest(const std::vector<std::string>& symbol_list,
                   double initial_capital,
                   int heartbeat,
                   std::chrono::system_clock::time_point&  start_date,
                   std::shared_ptr<my_data_handler> data_handler,
                   std::shared_ptr<execution_handler> execution_handler,
                   std::shared_ptr<portfolio> portfolio,
                   std::shared_ptr<strategy> strategy,
                   std::shared_ptr<event_queue> events)
    : symbol_list_(symbol_list),
      initial_capital_(initial_capital),
      heartbeat_(heartbeat),
      start_date_(start_date),
      data_handler_(data_handler),
      execution_handler_(execution_handler),
      portfolio_(portfolio),
      strategy_(strategy),
      events_(events),
      signals_(0),
      orders_(0),
      fills_(0) {}


void Backtest::run_backtest() {
    std::cout << "==================== Starting backtest ====================" << std::endl;

    int iteration = 0;

    while (data_handler_->continue_backtest()) {
        data_handler_->update_bars();

        while (!events_->is_empty()) {
            iteration++;
            auto event = events_->front();
            events_->pop();

            if (!event) continue;

            std::cout << "[EVENT HANDLED] " << event->repr() << std::endl;
            process_event(event);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_));
    }

    std::cout << "✅ Fin du backtest atteinte." << std::endl;
    std::cout << "\n==================== Backtest terminé ====================" << std::endl;
    std::cout << "Total SIGNALS : " << signals_ << std::endl;
    std::cout << "Total ORDERS  : " << orders_ << std::endl;
    std::cout << "Total FILLS   : " << fills_ << std::endl;
}

    
    
void Backtest::process_event(const std::shared_ptr<event>& event) {
        const std::string type = event->get_type();
        
        if (!strategy_) {
            std::cerr << "[ERROR] La stratégie n'est pas initialisée !" << std::endl;
        }
        if (type == "MARKET") {
            std::cout << "→ Traitement [MARKET]" << std::endl;
            strategy_->calculate_signals(event);
            portfolio_->update_timeindex(event);
        } 
        else if (type == "SIGNAL") {
            std::cout << "→ Traitement [SIGNAL]" << std::endl;
            signals_++;
            portfolio_->update_signal(event);
        } 
        else if (type == "ORDER") {
            std::cout << "→ Traitement [ORDER]" << std::endl;
            orders_++;
            execution_handler_->execute_order(event);
        } 
        else if (type == "FILL") {
            std::cout << "→ Traitement [FILL]" << std::endl;
            fills_++;
            portfolio_->update_fill(event);
        } 
        else {
            std::cout << "[WARNING] Type d’événement inconnu : " << type << std::endl;
        }
    }
    

void Backtest::output_performance() {
    //portfolio_->create_equity_curve();
    //std::cout << "Final portfolio statistics:" << std::endl;

    //auto stats = portfolio_->output_summary_stats();
    //for (const auto& [key, value] : stats) {
    //    std::cout << key << ": " << value << std::endl;
    //}

    std::cout << "\nTotal signals: " << signals_ << std::endl;
    std::cout << "Total orders:  " << orders_ << std::endl;
    std::cout << "Total fills:   " << fills_ << std::endl;
    double pnl = portfolio_->get_total_pnl();
    std::cout << "💰 PnL final : " << pnl << " $" << std::endl;
}

void Backtest::simulate_trading() {
    run_backtest();
    output_performance();
}
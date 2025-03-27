#include "execution_handler.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

simulated_execution_handler::simulated_execution_handler(std::shared_ptr<event_queue> event_queue)
    : events_(event_queue) {}

void simulated_execution_handler::execute_order(const std::shared_ptr<event>& event) {
    if (event->get_type() == "ORDER") {
        auto order = std::dynamic_pointer_cast<order_event>(event);
        if (order) {
            // Générer un timestamp ISO8601 simplifié
            auto now = std::chrono::system_clock::now();
            std::time_t time_now = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&time_now), "%Y-%m-%d %H:%M:%S");

            auto fill = std::make_shared<fill_event>(
                ss.str(),                      // timeindex
                order->get_symbol(),           // symbol
                "SIMULATED",                   // exchange
                order->get_quantity(),         // quantity
                order->get_direction(),        // direction
                0.0                            // fill_cost (set to 0, portfolio will compute)
            );


            events_->push(fill);
        }
    }
}

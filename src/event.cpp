#include "event.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>  // pour put_time
#include <sstream>
#include <chrono>   // pour system_clock::time_point si utilisé ici aussi
#include "utilities.h"

////////////////////////////Market Event//////////////////////////////////////
std::string market_event::repr() const {
    return "MarketEvent()";
}

std::string market_event::get_type() const{
    return "MARKET";
}



////////////////////////////////Signal Event///////////////////////////////////
signal_event::signal_event(const std::string& strategy_id, const std::string& symbol,
                            const std::chrono::system_clock::time_point datetime, const std::string& signal_type,
                            double strength)
    : strategy_id(strategy_id), symbol(symbol), datetime(datetime),
    signal_type(signal_type), strength(strength) {}

std::string signal_event::repr() const{
    std::ostringstream oss;
    std::string datetime_str=format_datetime(datetime);
    oss << "SignalEvent(strategy_id=" << strategy_id
        << ", symbol=" << symbol
        << ", datetime=" << datetime_str
        << ", type=" << signal_type
        << ", strength=" << strength << ")";
    return oss.str();
}
std::string signal_event::get_type() const {
    return "SIGNAL";
}

std::string signal_event::get_strategy_id() const { return strategy_id; }
std::string signal_event::get_symbol() const { return symbol; }
std::string signal_event::get_signal_type() const { return signal_type; }
double signal_event::get_strength() const { return strength; }
std::chrono::system_clock::time_point signal_event::get_datetime() const { return datetime; }


/////////////////////Order Event///////////////////////////////////////
order_event::order_event(const std::string& symbol, const std::string& order_type,
                        int quantity, const std::string& direction)
    : symbol(symbol), order_type(order_type), quantity(quantity), direction(direction) {}

std::string order_event::repr() const {
        std::ostringstream oss;
        oss << "OrderEvent(symbol=" << symbol
            << ", type=" << order_type
            << ", quantity=" << quantity
            << ", direction=" << direction << ")";
        return oss.str();
}
    
std::string order_event::get_type() const {
        return "ORDER";
}

void order_event::print_order() const {
    std::cout << repr() << std::endl;
}
std::string order_event::get_symbol() const { return symbol; }
std::string order_event::get_order_type() const { return order_type; }
int order_event::get_quantity() const { return quantity; }
std::string order_event::get_direction() const { return direction; }

/////////////////////Ib Comission Model/////////////
double ib_commission_model::calculate(int quantity) const{
    if (quantity<=500)
        return std::max(1.3,0.013*quantity);
    else
        return std::max(1.3,0.008*quantity);

}

////////////////Fill Event////////////////////////
fill_event::fill_event(const std::string& timeindex, const std::string& symbol,
    const std::string& exchange, int quantity, const std::string& direction,
    double fill_cost, std::optional<double> commission,
    std::shared_ptr<commission_model> commission_model)
: timeindex(timeindex), symbol(symbol), exchange(exchange),
quantity(quantity), direction(direction), fill_cost(fill_cost)
{
    if (commission.has_value()){
        this->commission=commission.value();
    }
    else if (commission_model != nullptr){
        this->commission=commission_model->calculate(quantity);
    }
    else {
        this->commission=ib_commission_model().calculate(quantity);
    }
}

std::string fill_event::repr() const {
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed;
    oss << "FillEvent(timeindex=" << timeindex
        << ", symbol=" << symbol
        << ", exchange=" << exchange
        << ", quantity=" << quantity
        << ", direction=" << direction
        << ", fill_cost=" << fill_cost
        << ", commission=" << commission << ")";
    return oss.str();
}

std::string fill_event::get_type() const {
    return "FILL";
}

std::string fill_event::get_timeindex() const { return timeindex; }
std::string fill_event::get_symbol() const { return symbol; }
std::string fill_event::get_exchange() const { return exchange; }
int fill_event::get_quantity() const { return quantity; }
std::string fill_event::get_direction() const { return direction; }
double fill_event::get_fill_cost() const { return fill_cost; }
double fill_event::get_commission() const { return commission; }
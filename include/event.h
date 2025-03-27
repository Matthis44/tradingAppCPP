#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <memory>
#include <optional>
#include <chrono>


class event {

    public :
        virtual ~event() = default;
        virtual std::string repr() const =0;
        virtual std::string get_type() const=0;
};

class market_event : public event {
    public:
        std::string repr() const override;
        std::string get_type() const override;

};

class signal_event : public event {
    public:
        signal_event(const std::string& strategy_id, const std::string& symbol,
                    const std::chrono::system_clock::time_point datetime, const std::string& signal_type,
                    double strenght);

        std::string repr() const override;
        std::string get_type() const override;

        ///Getters///
        std::string get_strategy_id() const;
        std::string get_symbol() const;
        std::chrono::system_clock::time_point get_datetime() const;
        std::string get_signal_type() const;
        double get_strength() const;

    
    private:
        std::string strategy_id;
        std::string symbol;
        std::chrono::system_clock::time_point datetime;
        std::string signal_type;
        double strength;
};

class order_event : public event {
    public:
        order_event(const std::string& symbol, const std::string& order_type,
                    int quantity, const std::string& direction);
    
        std::string repr() const override;
        std::string get_type() const override;
        void print_order() const;

        ///Getters////
        std::string get_symbol() const;
        std::string get_order_type() const;
        int get_quantity() const;
        std::string get_direction() const;
        
    private:
        std::string symbol;
        std::string order_type;
        int quantity;
        std::string direction;
};

class commission_model {

    public:
        virtual ~commission_model() = default;
        virtual double calculate(int quantity) const = 0;
};

    
class ib_commission_model : public commission_model {

    public:
        double calculate(int quantity) const override;
};

class fill_event : public event {
    public:
        fill_event(const std::string& timeindex, const std::string& symbol,
                   const std::string& exchange, int quantity, const std::string& direction,
                   double fill_cost,
                   std::optional<double> commission = std::nullopt,
                   std::shared_ptr<commission_model> commission_model = nullptr);
    
        std::string repr() const override;
        std::string get_type() const override;

        ///Getters///
        std::string get_timeindex() const;
        std::string get_symbol() const;
        std::string get_exchange() const;
        int get_quantity() const;
        std::string get_direction() const;
        double get_fill_cost() const;
        double get_commission() const;
    
    private:
        std::string timeindex;
        std::string symbol;
        std::string exchange;
        int quantity;
        std::string direction;
        double fill_cost;
        double commission;
    };
    

    
#endif // EVENT_H
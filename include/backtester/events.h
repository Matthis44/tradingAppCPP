#pragma once

#include <memory>
#include <queue>
#include <string>

#include "backtester/bar.h"

namespace bt {

enum class EventType { Market, Signal, Order, Fill };
enum class SignalType { Long, Short, Exit };
enum class Direction { Buy, Sell };

std::string to_string(SignalType s);
std::string to_string(Direction d);

struct Event {
    virtual ~Event() = default;
    virtual EventType type() const = 0;
};

/// A new bar has been published for every symbol.
struct MarketEvent final : Event {
    EventType type() const override { return EventType::Market; }
};

struct SignalEvent final : Event {
    SignalEvent(std::string strategy_id, std::string symbol, time_point datetime, SignalType signal,
                double strength = 1.0)
        : strategy_id(std::move(strategy_id)),
          symbol(std::move(symbol)),
          datetime(datetime),
          signal(signal),
          strength(strength) {}
    EventType type() const override { return EventType::Signal; }

    std::string strategy_id;
    std::string symbol;
    time_point datetime;
    SignalType signal;
    double strength;
};

struct OrderEvent final : Event {
    OrderEvent(std::string symbol, int quantity, Direction direction)
        : symbol(std::move(symbol)), quantity(quantity), direction(direction) {}
    EventType type() const override { return EventType::Order; }

    std::string symbol;
    int quantity;
    Direction direction;
};

struct FillEvent final : Event {
    FillEvent(time_point datetime, std::string symbol, int quantity, Direction direction, double price,
              double commission)
        : datetime(datetime),
          symbol(std::move(symbol)),
          quantity(quantity),
          direction(direction),
          price(price),
          commission(commission) {}
    EventType type() const override { return EventType::Fill; }

    time_point datetime;
    std::string symbol;
    int quantity;
    Direction direction;
    double price;
    double commission;
};

using EventPtr = std::shared_ptr<Event>;

/// FIFO queue shared by every component of the engine.
class EventQueue {
public:
    void push(EventPtr event) { queue_.push(std::move(event)); }
    bool empty() const { return queue_.empty(); }
    EventPtr pop() {
        if (queue_.empty()) return nullptr;
        EventPtr e = std::move(queue_.front());
        queue_.pop();
        return e;
    }

private:
    std::queue<EventPtr> queue_;
};

/// Interactive Brokers style fixed-rate commission for US equities.
double ib_commission(int quantity);

}  // namespace bt

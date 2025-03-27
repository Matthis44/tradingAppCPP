#ifndef STRATEGY_H
#define STRATEGY_H

#include "event_queue.h"
#include "event.h"
#include <memory>

class strategy {
public:
    virtual ~strategy() = default;

    // Calcule les signaux en fonction des données de marché
    virtual void calculate_signals(const std::shared_ptr<event>& market_event) = 0;
};

#endif // STRATEGY_H

#ifndef EXECUTION_HANDLER_H
#define EXECUTION_HANDLER_H

#include "event.h"
#include "event_queue.h"
#include <memory>

// Classe abstraite
class execution_handler {
public:
    virtual ~execution_handler() = default;

    // Interface à implémenter
    virtual void execute_order(const std::shared_ptr<event>& event) = 0;
};

// Implémentation simulée
class simulated_execution_handler : public execution_handler {
public:
    explicit simulated_execution_handler(std::shared_ptr<event_queue> event_queue);

    void execute_order(const std::shared_ptr<event>& event) override;

private:
    std::shared_ptr<event_queue> events_;
};

#endif // EXECUTION_HANDLER_H

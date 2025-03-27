#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <queue>
#include <memory>
#include "event.h"

class event_queue {
public:
    event_queue() = default;

    void push(std::shared_ptr<event> event);
    void pop();
    std::shared_ptr<event> front() const;
    bool is_empty() const;

private:
    std::queue<std::shared_ptr<event>> queue_;
};

#endif // EVENT_QUEUE_H

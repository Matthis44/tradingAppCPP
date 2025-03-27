#include "event_queue.h"

void event_queue::push(std::shared_ptr<event> event) {
    queue_.push(event);
}

void event_queue::pop() {
    if (!queue_.empty()) {
        queue_.pop();
    }
}

std::shared_ptr<event> event_queue::front() const {
    if (!queue_.empty()) {
        return queue_.front();
    }
    return nullptr;
}

bool event_queue::is_empty() const {
    return queue_.empty();
}

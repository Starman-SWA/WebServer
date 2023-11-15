//
// Created by James on 2023/6/17.
//

#include "TimerHeap.h"
#include "../Server/Logger.h"

void TimerHeap::add_timer(TimerTask* timer_task) {
    std::lock_guard lock(mutex);
    s.insert(timer_task);
    logger.debug("[TimerHeap::add_timer] inserting " + to_string(timer_task->conn_data.conn_fd) + ", addr: " + to_string(long(timer_task)) + ", s.size(): " + to_string(s.size()));
}

std::time_t TimerHeap::tick() {
    logger.debug("[TimerHeap::tick] tick, s.size(): " + to_string(s.size()));
    std::lock_guard lock(mutex);

    if (s.empty()) {
        return default_time_slot;
    }

    while (!s.empty() && (*s.begin())->get_expired_time() <= std::time(nullptr)) {
        if ((*s.begin())->is_valid()) {
            if ((*s.begin())->is_reset) {
                (*s.begin())->is_reset = false;
                (*s.begin())->expired_time = (*s.begin())->new_expired_time;
                auto ptr = *s.begin();
                s.erase(s.begin());
                s.insert(ptr);
            } else {
                (*s.begin())->callback();
                s.erase(s.begin());
            }
        } else {
            s.erase(s.begin());
        }
    }

    std::time_t ret = default_time_slot;
    if (!s.empty()) {
        ret = (*s.begin())->get_expired_time() - std::time(nullptr);
    }
    return ret <= 0 ? default_time_slot : ret;
}

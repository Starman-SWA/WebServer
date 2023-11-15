//
// Created by James on 2023/6/17.
//

#include "TimerTask.h"

[[nodiscard]] std::time_t TimerTask::get_expired_time() const {
    return expired_time;
}

void TimerTask::stop() {
    valid = false;
}

void TimerTask::callback() {
    callback_func(conn_data);
}

[[nodiscard]] bool TimerTask::is_valid() const {
    return valid;
}

void TimerTask::reset() {
    is_reset = true;
    new_expired_time = std::time(nullptr) + timeout;
}

void TimerTask::start(std::time_t _timeout, ConnData _conn_data, std::function<void(ConnData)> _callback_func) {
    valid = true;
    timeout = _timeout;
    conn_data = _conn_data;
    callback_func = std::move(_callback_func);
    expired_time = std::time(nullptr) + _timeout;
}

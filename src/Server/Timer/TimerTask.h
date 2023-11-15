//
// Created by James on 2023/6/17.
//

#ifndef WEBSERVER_TIMERTASK_H
#define WEBSERVER_TIMERTASK_H

#include <functional>
#include <ctime>
#include <memory>
#include "ConnData.h"

class TimerTask {
    friend class TimerHeap;
public:
    TimerTask() = default;
    TimerTask(std::time_t _timeout, ConnData _conn_data, std::function<void(ConnData)> _callback_func) : \
        timeout(_timeout), conn_data(_conn_data), callback_func(std::move(_callback_func)) {
        expired_time = std::time(nullptr) + _timeout;
    }
    TimerTask(TimerTask&) = default;
    TimerTask(TimerTask&&) = default;
    TimerTask& operator=(TimerTask const&) = default;
    TimerTask& operator=(TimerTask&&) = default;
    ~TimerTask() = default;

    [[nodiscard]] std::time_t get_expired_time() const;
    void stop();
    void callback();
    [[nodiscard]] bool is_valid() const;
    void start(std::time_t _timeout, ConnData _conn_data, std::function<void(ConnData)> _callback_func);

    void reset();

private:
    ConnData conn_data;
    std::time_t timeout{}, expired_time{}, new_expired_time{};
    std::function<void(ConnData)> callback_func;
    bool valid = false, is_reset = false;
};

struct TimerTaskPtrCompare {
    bool operator()(const TimerTask* const lhs, const TimerTask* const rhs) const {
        return lhs->get_expired_time() < rhs->get_expired_time();
    }
};

#endif //WEBSERVER_TIMERTASK_H

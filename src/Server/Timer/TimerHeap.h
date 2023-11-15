//
// Created by James on 2023/6/17.
//

#ifndef WEBSERVER_TIMERHEAP_H
#define WEBSERVER_TIMERHEAP_H

#include <queue>
#include <set>
#include <mutex>
#include "TimerTask.h"

class TimerHeap {
public:
    explicit TimerHeap(const std::time_t _default_time_slot) : default_time_slot(_default_time_slot) {}
    TimerHeap(TimerHeap&) = delete;
    TimerHeap(TimerHeap&&) = delete;
    TimerHeap& operator=(TimerHeap const&) = delete;
    TimerHeap& operator=(TimerHeap&&) = delete;
    ~TimerHeap() = default;

    void add_timer(TimerTask*);
    std::time_t tick();

public:
    std::mutex mutex;
    std::multiset<TimerTask*, TimerTaskPtrCompare> s;
    std::time_t default_time_slot;
};


#endif //WEBSERVER_TIMERHEAP_H

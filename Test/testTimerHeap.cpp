//
// Created by James on 2023/11/14.
//

#include <iostream>

#include "../src/Server/Timer/TimerHeap.h"
#include "../src/Server/Server/Logger.h"

Logger logger(Logger::DEBUG, Logger::TERMINAL, "");

int main() {
    TimerHeap timer_heap;
    std::vector<TimerTask> timer_tasks(100);

    for (int i = 0; i < 100; ++i) {
        timer_tasks[i].start(5, ConnData(1, i), [](ConnData){});
        std::cout << &timer_tasks[i] << std::endl;
        timer_heap.add_timer(&timer_tasks[i]);
        //s.insert(&timer_tasks[i]);
        std::cout << timer_heap.s.size() << std::endl;
    }

    for (int i = 20; i < 30; ++i) {
        std::cout << &timer_tasks[i] << std::endl;
        auto it = timer_heap.s.erase(timer_heap.s.find(&timer_tasks[i]));
        std::cout << *it << std::endl;
//        timer_heap.reset(&timer_tasks[i]);
//        timer_heap.s.insert(&timer_tasks[i]);
        std::cout << timer_heap.s.size() << std::endl;
    }

//    for (int i = 50; i < 100; ++i) {
//        std::cout << &timer_tasks[i] << std::endl;
//        timer_tasks[i].start(5, ConnData(1, i), [](ConnData){});
//        timer_heap.add_timer(&timer_tasks[i]);
//        //s.insert(&timer_tasks[i]);
//        std::cout << timer_heap.s.size() << std::endl;
//    }

    return 0;
}
//
// Created by James on 2023/6/15.
//
#include <iostream>
#include <thread>
#include "../ThreadSafeQueue/ThreadSafeQueue.h"

int main() {
    ThreadSafeQueue<int> q;

    std::thread t1(&ThreadSafeQueue<int>::wait_and_pop, &q);
    std::thread t2(&ThreadSafeQueue<int>::wait_and_pop, &q);
    t1.detach();
    t2.detach();

    return 0;
}
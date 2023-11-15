//
// Created by James on 2023/6/15.
//

#ifndef WEBSERVER_THREADSAFEQUEUE_H
#define WEBSERVER_THREADSAFEQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include "../Server/Logger.h"

template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(std::size_t _max_size) :  max_size(_max_size) {}
    ThreadSafeQueue(ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;
    ~ThreadSafeQueue() {
        logger.debug("[ThreadSafeQueue] deconstruct start");
        stop = true;
        cond.notify_all();
        logger.debug("[ThreadSafeQueue] deconstruct end");
    }

    bool try_push_and_notify(T t);
    T wait_and_pop(bool &success);

    void push(T t);
    T pop(bool &success);

private:
    std::queue<T> q;
    std::size_t size = 0, max_size;

    std::mutex mutex;
    std::condition_variable cond;

    bool stop = false;
};

template<typename T>
bool ThreadSafeQueue<T>::try_push_and_notify(T t) {
    std::unique_lock<std::mutex> lock(mutex);
    if (q.size() >= max_size) {
        return false;
    }
    q.push(std::move(t));
    cond.notify_one();
    return true;
}

template<typename T>
T ThreadSafeQueue<T>::wait_and_pop(bool &success) {
    std::unique_lock<std::mutex> lock(mutex);
    cond.wait(lock, [this] {return !this->q.empty() || stop;});

    if (this->q.empty() || stop) {
        success = false;
        return T();
    }

    success = true;
    T t = std::move(q.front());
    q.pop();
    return t;
}

template<typename T>
void ThreadSafeQueue<T>::push(T t) {
    std::unique_lock<std::mutex> lock(mutex);
    q.push(std::move(t));
}

template<typename T>
T ThreadSafeQueue<T>::pop(bool &success) {
    std::unique_lock<std::mutex> lock(mutex);
    if (q.empty()) {
        success = false;
        return T();
    }
    success = true;
    T t = std::move(q.front());
    q.pop();
    return t;
}

#endif //WEBSERVER_THREADSAFEQUEUE_H

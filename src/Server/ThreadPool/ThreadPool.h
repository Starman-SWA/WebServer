//
// Created by James on 2023/6/15.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H


#include <vector>
#include <thread>
#include "ThreadSafeQueue.h"
#include "../Server/Logger.h"
#include "../RequestHandler/RequestHandler.h"

class ThreadPool {
public:
    explicit ThreadPool(std::size_t _thread_num, std::size_t _max_request_num) : \
        threads(_thread_num), request_queue(_max_request_num) {
        for (auto& thread : threads) {
            thread = std::thread(&ThreadPool::worker, this);
        }
    }
    ThreadPool(ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    ~ThreadPool();

    bool append(RequestHandler* p_req);

    void worker();
private:
    std::vector<std::thread> threads;
    ThreadSafeQueue<RequestHandler*> request_queue;
    bool stop = false;
};


#endif //WEBSERVER_THREADPOOL_H

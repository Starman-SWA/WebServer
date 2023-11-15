//
// Created by James on 2023/6/15.
//

#include <iostream>
#include <sstream>
#include "ThreadPool.h"

ThreadPool::~ThreadPool() {
    logger.debug("[ThreadPool] deconstruct start");
    stop = true;
    for (auto& thread : threads) {
        thread.detach();
    }
    logger.debug("[ThreadPool] deconstruct end");
}

bool ThreadPool::append(RequestHandler* p_req) {
    int fd = p_req->get_conn_fd();
    if (!request_queue.try_push_and_notify(p_req)) {
        logger.debug("[ThreadPool] fd " + to_string(fd) + " push and notify fail");
        return false;
    }
    logger.debug("[ThreadPool] fd " + to_string(fd) + " push and notify success");
    return true;
}

void ThreadPool::worker() {
    while (!stop) {
        bool success = false;
        RequestHandler* p_req = request_queue.wait_and_pop(success);
        if (!success) {
            break;
        }
        logger.info("[ThreadPool] fd " + to_string(p_req->get_conn_fd()) + " pop from queue");
        logger.debug("===Thread processing conn_fd " + to_string(p_req->get_conn_fd()) + " start===");

        if (p_req->task_type == RequestHandler::READ) {
            p_req->process_read();
        } else if (p_req->task_type == RequestHandler::WRITE) {
            p_req->process_write();
        }

        logger.debug("===Thread processing conn_fd " + to_string(p_req->get_conn_fd()) + " end===");
    }
    logger.debug("[ThreadPool::worker] end");
}
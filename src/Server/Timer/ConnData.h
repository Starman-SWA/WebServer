//
// Created by James on 2023/6/17.
//

#ifndef WEBSERVER_CONNDATA_H
#define WEBSERVER_CONNDATA_H

#include <memory>
#include <utility>

struct ConnData {
    ConnData() = default;
    ConnData(int _epoll_fd, int _conn_fd) : \
        epoll_fd(_epoll_fd), conn_fd(_conn_fd) {}
    int epoll_fd{}, conn_fd{};
};


#endif //WEBSERVER_CONNDATA_H

//
// Created by James on 2023/6/16.
//

#ifndef WEBSERVER_REQUESTHANDLER_H
#define WEBSERVER_REQUESTHANDLER_H

#include <map>
#include <csignal>
#include <utility>
#include "../Server/Buffer.h"
#include "../ThreadPool/ThreadSafeQueue.h"
#include "../Timer/TimerHeap.h"
#include "../Server/Logger.h"

class RequestHandler {
public:
    explicit RequestHandler(std::size_t read_buf_size = 2048, std::size_t write_buf_size = 2048) : read_state(read_buf_size),
                                                                                     write_state(write_buf_size) {
        signal(SIGPIPE, SIG_IGN);
    }
    RequestHandler(RequestHandler&) = delete;
    RequestHandler(RequestHandler&&) = default;
    RequestHandler& operator=(RequestHandler&) = delete;
    RequestHandler& operator=(RequestHandler&&) = delete;
    ~RequestHandler() = default;

    void reset(int conn_fd, TimerTask* timer_task);
    void process_read();
    void process_write();

    static void close_conn_static(int conn_fd, TimerTask* timer_task);
    void close_conn(int conn_fd);
    [[nodiscard]] int get_conn_fd() const;

    static void mod_fd_to_epoll(int epoll_fd, int fd, uint32_t events);

    static int epoll_fd;

    using TaskType = enum {READ, WRITE};
    TaskType task_type = READ;

    static TimerHeap* timer_heap;
    TimerTask* timer_task = nullptr;
protected:
    int conn_fd = -1;

    ReadState read_state;
    WriteState write_state;

    bool keep_alive = false;

    void write_status_code_to_buf(int status_code, const std::string &err_msg, size_t body_length = 0);
    void write_file_info_to_buf(const std::string &url);
    bool read_socket_to_buf();
    bool write_buf_to_socket();
    bool parse_one_request();
};


#endif //WEBSERVER_REQUESTHANDLER_H

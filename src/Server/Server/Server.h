//
// Created by James on 2023/6/16.
//

#ifndef WEBSERVER_SERVER_H
#define WEBSERVER_SERVER_H

#include <arpa/inet.h>
#include <map>
#include <utility>
#include "../ThreadPool/ThreadPool.h"
#include "../Timer/TimerHeap.h"
#include "Logger.h"
#include "Buffer.h"

class ThreadPool;

struct ServerConfig {
    std::string port = "9090";
    int op_so_reuseaddr = 1;
    int listen_queue_len = std::numeric_limits<int>::max();
    int max_event_size = 65536;
    int epoll_timeout = 60000;
    int max_fd = 65536;
    int conn_timeout = 10;
    std::size_t thread_num = 4;
    std::size_t threadpool_max_request_num = 65536;
    int timerheap_default_time_slot = 5;
};

class Server {
public:
    explicit Server(ServerConfig  _config) : config(std::move(_config)), worker_thread_pool(config.thread_num, config.threadpool_max_request_num), \
    timer_heap(config.timerheap_default_time_slot)
    {
        listen_sock = socket(PF_INET, SOCK_STREAM, 0);
        if (listen_sock == -1) {
            throw std::runtime_error("socket() err");
        }

        if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &config.op_so_reuseaddr, sizeof(config.op_so_reuseaddr))) {
            throw std::runtime_error("setsockopt() err");
        }

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(std::stoi(config.port));

        if (bind(listen_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
            throw std::runtime_error("bind() err");
        }

        if (listen(listen_sock, config.listen_queue_len) == -1) {
            throw std::runtime_error("listen() err");
        }

        if (pipe(sig_pipe) < 0) {
            throw std::runtime_error("pipe() err");
        }

        request_handlers = std::vector<RequestHandler>(config.max_fd);
        timer_tasks = std::vector<TimerTask>(config.max_fd);
    }
    Server(Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&) = delete;
    Server& operator=(Server&&) = delete;
    ~Server();

    void run();
private:
    using EpollSigHandlerReturnType = struct {bool stop; bool timeout;};

    ServerConfig config;

    int listen_sock;
    static int sig_pipe[2];
    char sig_buf[4];
    TimerHeap timer_heap;
    std::vector<RequestHandler> request_handlers;
    std::vector<TimerTask> timer_tasks;

    static int fd_set_non_blocking(int fd);
    static void add_fd_to_epoll(int epoll_fd, int fd, uint32_t events);

    static void sig_handler(int sig);
    static void add_sig(int sig);
    [[nodiscard]] EpollSigHandlerReturnType epoll_sig_handler();

    void timer_heap_ticker();

    ThreadPool worker_thread_pool;
};

#endif //WEBSERVER_SERVER_H

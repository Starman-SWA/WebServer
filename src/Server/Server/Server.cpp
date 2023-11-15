//
// Created by James on 2023/6/16.
//
#include <cassert>
#include <csignal>
#include <fcntl.h>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include "Server.h"

using std::to_string;

int RequestHandler::epoll_fd;
TimerHeap* RequestHandler::timer_heap;

int Server::sig_pipe[2];

Server::~Server() {
    logger.debug("[Server] deconstructing start");
    if (close(listen_sock) < 0) {
        logger.error("[Server] event fd " + to_string(listen_sock) + " close fail, errno: " + to_string(errno));
    }
    logger.debug("[Server] deconstructing end");
}

void Server::run() {
    add_sig(SIGTERM);
    add_sig(SIGINT);
    add_sig(SIGQUIT);
    add_sig(SIGALRM);
    signal(SIGPIPE, SIG_IGN);

    int epoll_fd = epoll_create1(0);
    RequestHandler::epoll_fd = epoll_fd;

    add_fd_to_epoll(epoll_fd, listen_sock, EPOLLIN|EPOLLET|EPOLLONESHOT);
    fd_set_non_blocking(listen_sock);

    add_fd_to_epoll(epoll_fd, sig_pipe[0], EPOLLIN|EPOLLET|EPOLLONESHOT);
    fd_set_non_blocking(sig_pipe[0]);

    fd_set_non_blocking(sig_pipe[1]);

    timer_heap_ticker();

    epoll_event ep_events[config.max_event_size];
    bool stop = false, timeout = false;
    try {
        while (!stop) {
            int event_cnt = epoll_wait(epoll_fd, ep_events, config.max_event_size, config.epoll_timeout);

            if (event_cnt == -1) {
                if (errno != EINTR) {
                    throw std::runtime_error("epoll_wait() err, errno==" + std::to_string(errno));
                }
            } else if (event_cnt == 0) {
                break;
            } else {
                for (std::size_t i = 0; i < event_cnt; ++i) {
                    int event_fd = ep_events[i].data.fd;
                    logger.debug("[Server] processing fd " + std::to_string(event_fd));

                    if (event_fd == listen_sock) {
                        logger.debug("[Server] processing fd " + std::to_string(event_fd) + " branch 1");
                        int conn_sock = accept(event_fd, nullptr, nullptr);
                        if (conn_sock == -1) {
                            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                                logger.error("[Server] fd " + std::to_string(conn_sock) + " accept errno: " + std::to_string(errno));
                            }
                            RequestHandler::mod_fd_to_epoll(epoll_fd, listen_sock, EPOLLET | EPOLLIN | EPOLLONESHOT);
                            continue;
                        }
                        logger.info("[Server] fd " + std::to_string(conn_sock) + " accept");

                        add_fd_to_epoll(epoll_fd, conn_sock, EPOLLET | EPOLLONESHOT | EPOLLIN);
                        fd_set_non_blocking(conn_sock);

                        request_handlers[conn_sock].reset(conn_sock, &timer_tasks[conn_sock]);

                        logger.debug("[Server] ready to add timer " + to_string(conn_sock));
                        timer_tasks[conn_sock].start(config.conn_timeout, ConnData(epoll_fd, conn_sock), [this, &conn_sock](const ConnData& conn_data){
                            logger.info("[TimerHeap.callback] conn fd " + to_string(conn_data.conn_fd) + " close due to timeout");
                            RequestHandler::close_conn_static(conn_data.conn_fd, &timer_tasks[conn_sock]);
                        });
                        timer_heap.add_timer(&timer_tasks[conn_sock]);


                        RequestHandler::mod_fd_to_epoll(epoll_fd, listen_sock, EPOLLET | EPOLLIN | EPOLLONESHOT);

                    } else if (event_fd == sig_pipe[0]) {
                        logger.debug("[Server] processing fd " + std::to_string(event_fd) + " branch 2");
                        auto [_stop, _timeout] = epoll_sig_handler();
                        stop = _stop, timeout = _timeout;
                        RequestHandler::mod_fd_to_epoll(epoll_fd, sig_pipe[0], EPOLLET | EPOLLIN | EPOLLONESHOT);
                    } else {
                        logger.debug("[Server] processing fd " + std::to_string(event_fd) + " branch 3");

                        if (ep_events[i].events & (EPOLLIN | EPOLLOUT)) {
                            timer_tasks[event_fd].reset();

                            if (ep_events[i].events & EPOLLIN) {
                                request_handlers[event_fd].task_type = RequestHandler::READ;
                            } else if (ep_events[i].events & EPOLLOUT) {
                                request_handlers[event_fd].task_type = RequestHandler::WRITE;
                            }

                        } else {
                            throw std::runtime_error("other event type");
                        }

                        if (!worker_thread_pool.append(&request_handlers[event_fd])) {
                            logger.info("[Server] fd " + to_string(event_fd) + " discard due to full queue");
                            logger.debug("[Server] fd " + to_string(event_fd) + " {EPOLL_CTL_DEL}");
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, event_fd, nullptr);

                        } else {
                            logger.info("[Server] fd " + to_string(event_fd) + " append to queue");
                        }
                    }
                }

                if (timeout) {
                    timer_heap_ticker();
                    timeout = false;
                }
            }
        }
    } catch (std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
    }

    if (close(epoll_fd) < 0) {
        logger.error("[Server] event fd " + to_string(epoll_fd) + " close fail, errno: " + to_string(errno));
    }
}

void Server::sig_handler(int sig) {
    int save_errno = errno;
    ssize_t ret = write(sig_pipe[1], reinterpret_cast<char*>(&sig), 1);
    if (ret < 0) {
       logger.error("[Server] sig_handler write() err");
    }
    errno = save_errno;
}

void Server::add_sig(int sig) {
    struct sigaction sa{};
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    if (sigaction(sig, &sa, nullptr) < 0) {
        logger.error("[Server] sigaction() err, sig==" + std::to_string(sig));
    }
}

int Server::fd_set_non_blocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    return fcntl(fd, F_SETFL, new_option);
}

void Server::add_fd_to_epoll(int epoll_fd, int fd, uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    logger.debug("[Server] fd " + to_string(fd) + " {EPOLL_CTL_ADD} ET:" + to_string(events&EPOLLET) + " IN:" +
                 to_string(events&EPOLLIN) + " OUT:" + to_string(events&EPOLLOUT) + " ONESHOT:" + to_string(events&EPOLLONESHOT));
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
}

Server::EpollSigHandlerReturnType Server::epoll_sig_handler() {
    bool stop = false, timeout = false;
    ssize_t read_len;
    do {
        read_len = read(sig_pipe[0], sig_buf, sizeof(sig_buf));
        if (read_len == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
            logger.error("[Server] read() sig_pipe[0] err, errno=" + std::to_string(errno));
        } else if (read_len == 0) {
            throw std::runtime_error("sig_pipe[0] closed");
        }

        for (int j = 0; j < read_len; ++j) {
            switch(sig_buf[j]) {
                case SIGTERM:
                case SIGINT:
                case SIGQUIT:
                    stop = true;
                    break;
                case SIGALRM:
                    timeout = true;
                    break;
            }
            logger.info("[Server] signal " + to_string(sig_buf[j]) + " handled");
        }
    } while (!(read_len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)));

    return {stop, timeout};
}

void Server::timer_heap_ticker() {
    std::time_t timeout = timer_heap.tick();
    alarm(timeout);
}

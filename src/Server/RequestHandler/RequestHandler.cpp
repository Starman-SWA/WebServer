//
// Created by James on 2023/6/16.
//

#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include "RequestHandler.h"
#include "../Util/StringProcess.h"
#include "HttpParser.h"

using std::to_string;

void RequestHandler::close_conn_static(int conn_fd, TimerTask* timer_task) {
    logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " {EPOLL_CTL_DEL}");
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_fd, nullptr);
    timer_task->stop();

    if (close(conn_fd) < 0) {
        logger.error("[RequestHandler] conn fd " + to_string(conn_fd) + " close fail, errno: " + to_string(errno));
    }
    logger.info("[RequestHandler] " + to_string(conn_fd) + " close conn");
}

void RequestHandler::close_conn(int conn_fd) {
    logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " {EPOLL_CTL_DEL}");
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_fd, nullptr);
    timer_task->stop();

    if (close(conn_fd) < 0) {
        logger.error("[RequestHandler] conn fd " + to_string(conn_fd) + " close fail, errno: " + to_string(errno));
    }
    logger.info("[RequestHandler] " + to_string(conn_fd) + " close conn");
}

void RequestHandler::mod_fd_to_epoll(int epoll_fd, int fd, uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
    logger.debug("[RequestHandler] fd " + to_string(fd) + " {EPOLL_CTL_MOD} ET:" + to_string(events&EPOLLET) + " IN:" +
                         to_string(events&EPOLLIN) + " OUT:" + to_string(events&EPOLLOUT) + " ONESHOT:" + to_string(events&EPOLLONESHOT));
}

std::map<int, const char*> HttpStatusCodes = {
        {200, "OK"},
        {400, "Bad Request"},
        {404, "Not Found"},
};

void RequestHandler::write_status_code_to_buf(int status_code, const std::string& err_msg, size_t body_length) {
    auto it = HttpStatusCodes.begin();
    assert ((it = HttpStatusCodes.find(status_code)) != HttpStatusCodes.end());

    std::stringstream strm;
    if (status_code == 200) {
        strm << "HTTP/1.1 " << std::to_string(status_code) << " " << it->second << "\r\n"
             << "Content-Length: " << body_length << "\r\n";

        if (keep_alive) {
            strm << "Connection: keep-alive" << "\r\n";
        } else {
            strm << "Connection: close" << "\r\n";
        }

        strm << "\r\n";

    } else {
        strm << "HTTP/1.1 " << std::to_string(status_code) << " " << it->second << "\r\n"
             << "Content-Length: " << err_msg.size() << "\r\n";

        if (keep_alive) {
            strm << "Connection: keep-alive" << "\r\n";
        } else {
            strm << "Connection: close" << "\r\n";
        }

        strm << "\r\n" << err_msg;
    }
    
    assert(write_state.buf_idx == 0);
    if (strm.str().size() > write_state.buf_size) {
        throw std::runtime_error("write buffer overflow");
    }
    memcpy(write_state.buffer.get(), strm.str().c_str(), strm.str().size());
    write_state.buf_idx = strm.str().size();
}

void RequestHandler::write_file_info_to_buf(const std::string& url) {
    std::string url_decode = StringProcess::URLDecode(url);

    struct stat path_stat;
    if (stat(url_decode.c_str(), &path_stat) < 0) {
        logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " stat fail, errno: " + to_string(errno));
        write_status_code_to_buf(404, "resource not found, stat fail", 0);
        return;
    }

    int fd;
    if (S_ISDIR(path_stat.st_mode)) {
        fd = open((url_decode + "index.html").c_str(), O_RDONLY);
        if (fd == -1) {
            write_status_code_to_buf(404, "resource not found");
            return;
        }
    } else {
        fd = open(url_decode.c_str(), O_RDONLY);
        if (fd == -1) {
            write_status_code_to_buf(404, "resource not found");
            return;
        }
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " bad file stat, errno: " + to_string(errno));
        write_status_code_to_buf(404, "bad file stat");
        if (close(fd) < 0) {
            logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " close fail, errno: " + to_string(errno));
        }
        return;
    }

    __off_t file_len = file_stat.st_size;
    if (!file_len) {
        write_status_code_to_buf(404, "empty resource");
        if (close(fd) < 0) {
            logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " close fail, errno: " + to_string(errno));
        }
        return;
    }

//    char* file_buf = reinterpret_cast<char*>(mmap(nullptr, file_len, PROT_READ, MAP_SHARED, fd, SEEK_SET));
//    if (file_buf == MAP_FAILED) {
//        logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " mmap fail, errno: " + to_string(errno));
//        write_status_code_to_buf(404, "resource mmap fail");
//        if (close(fd) < 0) {
//            logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " close fail, errno: " + to_string(errno));
//        }
//        return;
//    }

//    if (close(fd) < 0) {
//        logger.error("[RequestHandler] file fd " + to_string(conn_fd) + " close fail, errno: " + to_string(errno));
//    }

    write_status_code_to_buf(200, "", file_len);
    
    write_state.fd = fd;
    write_state.file_size = file_len;
}

bool RequestHandler::write_buf_to_socket() {
    // loop to write buffer to socket
    if (write_state.state == WriteState::WRITE_BUFFER) {

        while (write_state.write_idx < write_state.buf_idx) {
//            if (!timer_task->is_valid()) {
//                return false;
//            }
            logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " try write length: " + to_string(write_state.buf_idx - write_state.write_idx));
            ssize_t ret = write(conn_fd, &write_state.buffer[write_state.write_idx], write_state.buf_idx - write_state.write_idx);

            if (ret < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    mod_fd_to_epoll(RequestHandler::epoll_fd, conn_fd, EPOLLET | EPOLLONESHOT | EPOLLOUT);
                    logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " exit write_buf_to_socket due to EAGAIN");
                    return false;
                } else {
                    logger.error("[RequestHandler] fd " + to_string(conn_fd) + " write buffer errno: " + to_string(errno));
                    close_conn(conn_fd);
                    return false;
                }
            } else if (ret == 0) {
                logger.error("[RequestHandler] fd " + to_string(conn_fd) + " write buffer: EOF");
                close_conn(conn_fd);
                return false;
            } else {
//                timer_heap->reset(timer_task);
                logger.info("[RequestHandler] fd " + to_string(conn_fd) + " write buffer length: " + to_string(ret));
                write_state.write_idx += ret;
                if (write_state.write_idx == write_state.buf_idx && write_state.fd != -1) {
                    write_state.state = WriteState::WRITE_FILE;
                }
            }
        }

        write_state.write_idx = write_state.buf_idx = 0;
    }

    if (write_state.state == WriteState::WRITE_FILE) {

        while (write_state.file_write_idx < write_state.file_size) {
//            ssize_t ret = read(write_state.fd, write_state.buffer.get(), std::min(write_state.buf_size, write_state.file_size-write_state.file_write_idx));
//            if (ret == -1) {
//                logger.error("[RequestHandler] fd " + to_string(conn_fd) + " read file " + to_string(write_state.fd) + " errno: " + to_string(errno));
//                throw std::runtime_error("read file err, see log");
//            }
//
//            logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " try write length: " + to_string(ret));
//            ret = write(conn_fd, write_state.buffer.get(), ret);
            logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " try sendfile length: " + to_string(write_state.file_size - write_state.file_write_idx));
            ssize_t ret = sendfile(conn_fd, write_state.fd, &write_state.file_write_idx, write_state.file_size - write_state.file_write_idx);

            if (ret < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    mod_fd_to_epoll(RequestHandler::epoll_fd, conn_fd, EPOLLET | EPOLLONESHOT | EPOLLOUT);
                    logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " exit write_buf_to_socket due to EAGAIN");
                    return false;
                } else {
                    logger.error("[RequestHandler] fd " + to_string(conn_fd) + " write file errno: " + to_string(errno));
                    close_conn(conn_fd);
                    return false;
                }
            } else if (ret == 0) {
                logger.error("[RequestHandler] fd " + to_string(conn_fd) + " write file: EOF");
                close_conn(conn_fd);
                return false;
            } else {
                logger.info("[RequestHandler] fd " + to_string(conn_fd) + " write file length: " + to_string(ret));
                write_state.file_write_idx += ret;
            }
        }

        write_state.file_write_idx = write_state.file_size = 0;
        if (close(write_state.fd) < 0) {
            logger.error("[RequestHandler] fd " + to_string(conn_fd) + " close file " + to_string(write_state.fd) + "errno: " + to_string(errno));
        }
        write_state.fd = -1;
        write_state.state = WriteState::WRITE_BUFFER;
    }



    return true;
}

bool RequestHandler::read_socket_to_buf() {
    // read until EAGAIN
    while (true) {
        if (read_state.read_idx >= read_state.buf_size) {
            throw std::runtime_error("read buffer overflow");
        }

//        if (!p_timer->is_valid()) {
//            return false;
//        }
        logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " try read length: " + to_string(read_state.buf_size - read_state.read_idx));
        ssize_t ret = read(conn_fd, &read_state.buffer[read_state.read_idx], read_state.buf_size - read_state.read_idx);
        // timer_heap.reset(p_timer);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                logger.error("[RequestHandler] fd " + to_string(conn_fd) + " read errno: " + to_string(errno));
                close_conn(conn_fd);
                return false;
            }
        } else if (ret == 0) {
            logger.error("[RequestHandler] fd " + to_string(conn_fd) + " read: EOF");
            close_conn(conn_fd);
            return false;
        } else {
            logger.info("[RequestHandler] fd " + to_string(conn_fd) + " read length: " + to_string(ret));
            read_state.read_idx += ret;
        }
    }

    return true;
}

bool RequestHandler::parse_one_request() {
    if (read_state.read_idx == 0) {
        if (keep_alive) {
            mod_fd_to_epoll(epoll_fd, conn_fd, EPOLLET | EPOLLIN | EPOLLONESHOT); // connection:keep-alive
        } else {
            close_conn(conn_fd); // connection:closed
        }

        logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " exit parse_one_request due to read_idx==0");
        return false;
    }

    HttpParser httpParser(read_state);
    auto parser_ret = httpParser.parse_request_line_and_header();
    if (parser_ret == HttpParser::REQUEST_OPEN) {
        if (read_state.read_idx < read_state.buf_size) {
            mod_fd_to_epoll(epoll_fd, conn_fd, EPOLLET | EPOLLONESHOT | EPOLLIN);
        } else {
            close_conn(conn_fd);
        }
        logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " exit parse_one_request due to REQUEST_OPEN");
        return false;
    } else if (parser_ret == HttpParser::REQUEST_BAD) {
        close_conn(conn_fd);
        logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " exit parse_one_request due to REQUEST_BAD");
        return false;
    }

    keep_alive = httpParser.keep_alive;

    memmove(read_state.buffer.get(), &read_state.buffer[httpParser.checked_idx], read_state.read_idx - httpParser.checked_idx);
    read_state.read_idx -= httpParser.checked_idx;

    if (httpParser.request_type != "GET") {
        close_conn(conn_fd);
        logger.debug("[RequestHandler] fd " + to_string(conn_fd) + " exit parse_one_request due to method not \"GET\"");
        return false;
    }

    write_file_info_to_buf("../resources" + httpParser.url);

    return true;
}

int RequestHandler::get_conn_fd() const {
    return conn_fd;
}

void RequestHandler::process_read() {
    logger.debug("[ReadHandler] fd " + to_string(conn_fd) + " process start");

    if (!read_socket_to_buf()) {
        logger.debug("[ReadHandler] fd " + to_string(conn_fd) + " process end after read_socket_to_buf()");
        return;
    }

    // loop to deal with possible multiple requests in read buffer
    while (true) {
        if (!parse_one_request()) {
            logger.debug("[ReadHandler] fd " + to_string(conn_fd) + " process end after parse_one_request()");
            return;
        }

        if (!write_buf_to_socket()) {
            logger.debug("[ReadHandler] fd " + to_string(conn_fd) + " process end after write_buf_to_socket()");
            return;
        }
    }

    logger.debug("[ReadHandler] fd " + to_string(conn_fd) + " process end at the end");
}

void RequestHandler::process_write() {
    logger.debug("[WriteHandler] fd " + to_string(conn_fd) + " process start");

    // write buffer and file
    if (!write_buf_to_socket()) {
        logger.debug("[WriteHandler] fd " + to_string(conn_fd) + " process end after first write_buf_to_socket()");
        return;
    }

    // loop to parse read buffer and write
    while (true) {
        if (!parse_one_request()) {
            logger.debug("[WriteHandler] fd " + to_string(conn_fd) + " process end after parse_one_request()");
            return;
        }

        if (!write_buf_to_socket()) {
            logger.debug("[WriteHandler] fd " + to_string(conn_fd) + " process end after second write_buf_to_socket()");
            return;
        }
    }

    logger.debug("[WriteHandler] fd " + to_string(conn_fd) + " process end at the end");
}

void RequestHandler::reset(int conn_fd, TimerTask* timer_task) {
    this->conn_fd = conn_fd;
    read_state.read_idx = 0;
    write_state.state = WriteState::WRITE_BUFFER;
    write_state.buf_idx = write_state.write_idx = write_state.file_size = write_state.file_write_idx = 0;
    write_state.fd = -1;
    this->timer_task = timer_task;
}
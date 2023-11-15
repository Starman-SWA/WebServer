//
// Created by James on 2023/10/12.
//

#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H

#include <sys/mman.h>
#include <memory>

struct ReadState {
    std::unique_ptr<char[]> buffer;
    std::size_t buf_size, read_idx = 0;

    explicit ReadState(std::size_t _buf_size) : buf_size(_buf_size) {
        buffer = std::make_unique<char[]>(_buf_size);
    }
    ReadState(ReadState&) = delete;
    ReadState(ReadState&&) = default;
    ReadState& operator=(ReadState&) = delete;
    ReadState& operator=(ReadState&&) = default;
    ~ReadState() = default;
};

struct WriteState {
    using State = enum {WRITE_BUFFER, WRITE_FILE};

    State state = WRITE_BUFFER;
    std::unique_ptr<char[]> buffer;
    __off_t buf_size, buf_idx = 0, write_idx = 0;

    int fd = -1;
    __off_t file_size = 0, file_write_idx = 0;

    explicit WriteState(std::size_t _buf_size) : buf_size(_buf_size) {
        buffer = std::make_unique<char[]>(_buf_size);
    }

    WriteState(WriteState&) = delete;
    WriteState(WriteState&&) = default;
    WriteState& operator=(WriteState&) = delete;
    WriteState& operator=(WriteState&& rhs) = default;

    ~WriteState();
};

#endif //WEBSERVER_BUFFER_H

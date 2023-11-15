//
// Created by James on 2023/9/26.
//

#include <iostream>
#include <cassert>
#include <c++/7/thread>
#include <fcntl.h>
#include <sstream>

#include "Logger.h"

Logger::Logger() {
    level_threashold = INFO;
    target = TERMINAL;

    std::cout << "===== start logging at " << getTime() << " =====" << std::endl;
}

Logger::Logger(log_level level_threashold) : Logger(level_threashold, TERMINAL, "") {}

Logger::Logger(log_level level_threashold, log_target target, const std::string& path) {
    this->level_threashold = level_threashold;
    this->target = target;

    file_out.open(path, std::ios::app);

    if (target != FILE) {
        std::cout << "===== start logging at " << getTime() << " =====" << std::endl;
    }
    if (target != TERMINAL) {
        file_out << "===== start logging at " << getTime() << " =====" << std::endl;
    }
}

Logger::~Logger() {
    if (target != FILE) {
        std::cout << "===== stop logging at " << getTime() << " =====" << std::endl;
    }
    if (target != TERMINAL) {
        file_out << "===== stop logging at " << getTime() << " =====" << std::endl;
    }
}

void Logger::output(log_level level, const std::string& msg) {
    std::string prefix;
    switch (level) {
        case DEBUG:
            prefix = " [DEBUG]: ";
            break;
        case INFO:
            prefix = " [INFO]: ";
            break;
        case WARNING:
            prefix = " [WARNING]: ";
            break;
        case ERROR:
            prefix = " [ERROR]: ";
            break;
        default:
            assert(false);
    }

    std::stringstream strm;
    strm << getTime() << " [" << std::this_thread::get_id() << "]" << prefix << msg << std::endl;

    // std::lock_guard lock(mutex);
    if (target != FILE && level >= level_threashold) {
        std::cout << strm.str();
    }
    if (target != TERMINAL) {
        file_out << strm.str();
    }
}

void Logger::debug(const std::string& msg) {
    output(DEBUG, msg);
}

void Logger::info(const std::string& msg) {
    output(INFO, msg);
}

void Logger::warning(const std::string& msg) {
    output(WARNING, msg);
}

void Logger::error(const std::string& msg) {
    output(ERROR, msg);
}

std::string Logger::getTime()
{
    time_t timep;
    time(&timep);
    char tmp[64];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
    return tmp;
}
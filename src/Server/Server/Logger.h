//
// Created by James on 2023/9/26.
//

#ifndef WEBSERVER_LOGGER_H
#define WEBSERVER_LOGGER_H

#include <string>
#include <fstream>

using std::to_string;

class Logger {
public:
    enum log_level {DEBUG, INFO, WARNING, ERROR};
    enum log_target {FILE_AND_TERMINAL, FILE, TERMINAL};

    Logger();
    explicit Logger(log_level level_threashold);
    Logger(log_level level_threashold, log_target target, const std::string& path);
    ~Logger();
    Logger(const Logger&)=delete;
    Logger(const Logger&&)=delete;
    Logger& operator=(const Logger&)=delete;
    Logger& operator=(const Logger&&)=delete;

    void output(log_level level, const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);

private:
    log_level level_threashold;
    log_target target;

    std::fstream file_out;

    static std::string getTime();

};

extern Logger logger;


#endif //WEBSERVER_LOGGER_H

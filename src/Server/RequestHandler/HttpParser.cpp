//
// Created by James on 2023/6/19.
//

#include <cstring>
#include <iostream>
#include "HttpParser.h"
#include "../Server/Logger.h"

HttpParser::ParseRequestLineAndHeaderRet HttpParser::parse_request_line_and_header() {
    // logger.debug("parse_request_line_and_header " + std::string(&read_state.buffer[line_start_idx]));
    HttpParser::ParseLineRet parse_line_ret;
    while ((parse_line_ret = parse_line()) != LINE_OPEN) {
        if (parse_line_ret == LINE_BAD) {
            return REQUEST_BAD;
        }

        if (parseRequestLineAndHeaderState == PARSE_REQUEST_LINE) {
            ParseRequestLineRet parse_request_line_ret = parse_request_line();
            if (parse_request_line_ret == REQUEST_LINE_BAD) {
                return REQUEST_BAD;
            }
            parseRequestLineAndHeaderState = PARSE_HEADER;
        } else if (parseRequestLineAndHeaderState == PARSE_HEADER) {
            ParseHeaderRet parse_header_ret = parse_header();
            if (parse_header_ret == HEADER_END) {
                parseRequestLineAndHeaderState = PARSE_REQUEST_LINE;
                return REQUEST_OK;
            } else if (parse_header_ret == HEADER_BAD) {
                return REQUEST_BAD;
            }
        }
    }

    return REQUEST_OPEN;
}

HttpParser::ParseLineRet HttpParser::parse_line() {
    line_start_idx = checked_idx;

    for (; checked_idx < read_idx; ++checked_idx) {
        if (read_state.buffer[checked_idx] == '\r') {
            if (checked_idx + 1 >= read_idx) {
                checked_idx += 1;
                return LINE_OPEN;
            } else if (read_state.buffer[checked_idx+1] == '\n') {
                checked_idx += 2;
                return LINE_OK;
            } else {
                checked_idx += 1;
                err_msg = "invalid line format";
                return LINE_BAD;
            }
        }
    }

    return LINE_OPEN;
}

HttpParser::ParseRequestLineRet HttpParser::parse_request_line() {
    std::string_view sv(&read_state.buffer[line_start_idx], checked_idx - line_start_idx);
    std::size_t start_pos = 0, pos;

    if ((pos = sv.find_first_of(" \r\n", start_pos)) == std::string_view::npos) {
        err_msg = "invalid request line";
        return REQUEST_LINE_BAD;
    }
    request_type = std::string(&sv[start_pos], pos - start_pos);
    if (request_type != "GET" && request_type != "POST") {
        err_msg = "invalid request method";
        return REQUEST_LINE_BAD;
    }

    start_pos = pos + 1;
    if ((pos = sv.find_first_of(" \r\n", start_pos)) == std::string_view::npos) {
        err_msg = "invalid request line";
        return REQUEST_LINE_BAD;
    }
    url = std::string(&sv[start_pos], pos - start_pos);

    start_pos = pos + 1;
    if ((pos = sv.find_first_of(" \r\n", start_pos)) == std::string_view::npos) {
        err_msg = "invalid request line";
        return REQUEST_LINE_BAD;
    }
    std::string_view http_version_sv = std::string_view(&sv[start_pos], pos - start_pos);
    if (http_version_sv != "HTTP/1.0" && http_version_sv != "HTTP/1.1") {
        err_msg = "invalid http version";
        return REQUEST_LINE_BAD;
    }

    return REQUEST_LINE_OK;
}

HttpParser::ParseHeaderRet HttpParser::parse_header() {
    if (read_state.buffer[line_start_idx] == '\r') {
        return HEADER_END;
    }

    std::string_view sv(&read_state.buffer[line_start_idx], checked_idx - line_start_idx);
    std::size_t start_pos = 0, pos;

    if ((pos = sv.find_first_of(':', start_pos)) == std::string_view::npos) {
        err_msg = "invalid header";
        return HEADER_BAD;
    }
    std::string_view field(&read_state.buffer[line_start_idx + start_pos], pos);
    if (field == "Content-Length") {
        start_pos = pos + 2;
        if ((pos = sv.find_first_of("\r\n", start_pos)) == std::string_view::npos) {
            err_msg = "invalid header";
            return HEADER_BAD;
        }
        content_length = std::stoul(std::string_view(&read_state.buffer[line_start_idx + start_pos], pos - start_pos).data());
    } else if (field == "Content-Type") {
        start_pos = pos + 2;
        if ((pos = sv.find_first_of(';', start_pos)) == std::string_view::npos) {
            err_msg = "invalid header";
            return HEADER_BAD;
        }
        if ((std::string_view(&read_state.buffer[line_start_idx + start_pos], pos - start_pos)) != "multipart/form-data") {
            err_msg = "invalid Content-Type";
            return HEADER_BAD;
        }

        start_pos = pos + 2;
        if ((pos = sv.find("boundary=", start_pos)) == std::string_view::npos) {
            err_msg = "invalid header, missing boundary";
            return HEADER_BAD;
        }

        start_pos = pos + 9;
        if ((pos = sv.find_first_of(" \r\n", start_pos)) == std::string_view::npos) {
            err_msg = "invalid header";
            return HEADER_BAD;
        }
        boundary = "--" + std::string(&read_state.buffer[line_start_idx + start_pos], pos - start_pos);

    } else if (field == "Transfer-Encoding") {
        err_msg = "header field Transfer-Encoding not supported";
        return HEADER_BAD;
    } else if (field == "Connection") {
        start_pos = pos + 2;
        if ((pos = sv.find_first_of("\r\n", start_pos)) == std::string_view::npos) {
            err_msg = "invalid header";
            return HEADER_BAD;
        }

        if ((std::string_view(&read_state.buffer[line_start_idx + start_pos], pos - start_pos)) == "close") {
            keep_alive = false;
        } else if ((std::string_view(&read_state.buffer[line_start_idx + start_pos], pos - start_pos)) == "keep-alive") {
            keep_alive = true;
        } else {
            err_msg = "header field connection format wrong";
            return HEADER_BAD;
        }
    }

    return HEADER_OK;
}
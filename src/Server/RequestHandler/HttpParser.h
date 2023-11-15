//
// Created by James on 2023/6/19.
//

#ifndef WEBSERVER_HTTPPARSER_H
#define WEBSERVER_HTTPPARSER_H

#include <vector>
#include <string>
#include "../Server/Buffer.h"

class HttpParser {
public:
    friend class RequestHandler;

    explicit HttpParser(ReadState& _read_state) : read_state(_read_state), read_idx(_read_state.read_idx) {}
    HttpParser(HttpParser&) = delete;
    HttpParser(HttpParser&&) = delete;
    HttpParser& operator=(HttpParser const&) = delete;
    HttpParser& operator=(HttpParser&&) = delete;
    ~HttpParser() = default;

    using ParseRequestLineAndHeaderRet = enum {REQUEST_OPEN, REQUEST_BAD, REQUEST_OK};
    ParseRequestLineAndHeaderRet parse_request_line_and_header();

    using ParseLineRet = enum {LINE_OPEN, LINE_OK, LINE_BAD};
    ParseLineRet parse_line();

    using ParseRequestLineRet = enum {REQUEST_LINE_BAD, REQUEST_LINE_OK};
    ParseRequestLineRet parse_request_line();

    using ParseHeaderRet = enum {HEADER_OK, HEADER_END, HEADER_BAD};
    ParseHeaderRet parse_header();

private:
    using ParseRequestLineAndHeaderState = enum {PARSE_REQUEST_LINE, PARSE_HEADER};
    ParseRequestLineAndHeaderState parseRequestLineAndHeaderState = PARSE_REQUEST_LINE;

    ReadState& read_state;
    std::size_t checked_idx = 0, read_idx, line_start_idx = 0;

    std::string request_type, url, boundary, filename;
    std::size_t content_length = 0;

    bool keep_alive = false;

    std::string err_msg;
};


#endif //WEBSERVER_HTTPPARSER_H

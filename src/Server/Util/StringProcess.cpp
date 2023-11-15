//
// Created by James on 2023/9/27.
//

#include "StringProcess.h"
#include "../Server/Logger.h"

std::string StringProcess::URLDecode(const std::string& url) {
    std::string res;
    std::string::size_type i = 0;
    while (i < url.size()) {
        if (url[i] != '%') {
            res.push_back(url[i++]);
        } else {
            char ch = static_cast<char>(std::stoi(url.substr(i + 1, 2), nullptr, 16));
            res.push_back(ch);
            i += 3;
        }
    }

    return res;
}
//
// Created by James on 2023/10/18.
//

#include "Logger.h"
#include "Buffer.h"

WriteState::~WriteState() {
    if (fd != -1) {
        if (close(fd) < 0) {
            logger.error("[WriteState] fd " + to_string(fd) + " close errno: " + to_string(errno));
        }
        fd = -1;
    }
}
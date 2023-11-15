#include "Server/Server.h"
#include "Server/Logger.h"

Logger logger(Logger::ERROR, Logger::FILE_AND_TERMINAL, "../log.txt");

int main(int argc, char* argv[]) {
    ServerConfig config;
    Server server(config);
    server.run();
    logger.debug("[main] end");
    return 0;
}

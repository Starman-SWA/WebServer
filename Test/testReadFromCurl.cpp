#include <iostream>
#include <sys/socket.h>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

const int ACCEPT_COUNT = 5;
const int BUF_SIZE = 512;

class Server {
public:
    explicit Server(const std::string& port, const int max_connection_num = 5, const int op_so_reuseaddr = 1) {
        serv_sock = socket(PF_INET, SOCK_STREAM, 0);
        if (serv_sock == -1) {
            throw std::runtime_error("socket() err");
        }

        if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &op_so_reuseaddr, sizeof(op_so_reuseaddr))) {
            throw std::runtime_error("setsockopt() err");
        }

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(std::stoi(port));

        if (bind(serv_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
            throw std::runtime_error("bind() err");
        }

        if (listen(serv_sock, max_connection_num) == -1) {
            throw std::runtime_error("listen() err");
        }
    }

    void accept_and_send_back() const {
        sockaddr_in clnt_addr{};
        socklen_t clnt_addr_size = sizeof(clnt_addr);
        int clnt_sock = accept(serv_sock, (sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1) {
            throw std::runtime_error("accept() err");
        }

        std::vector<char> buf(BUF_SIZE);
        ssize_t read_len;
        while ((read_len = read(clnt_sock, &buf[0], BUF_SIZE))) {
            if (read_len == -1) {
                std::cout << "read() error" << std::endl;
                break;
            }
            ssize_t write_len = 0;
            while (write_len < read_len) {
                ssize_t this_write_len = write(clnt_sock, &buf[0], read_len * sizeof(char));
                if (this_write_len == -1) {
                    std::cout << "write() err" << std::endl;
                    break;
                }
                write_len += this_write_len;
            }

        }
        std::cout << "read() ends" << std::endl;
        close(clnt_sock);
    }

    ~Server() {
        close(serv_sock);
    }
private:
    int serv_sock;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::string err_msg = "Usage: " + std::string(argv[0]) + " <port>";
        throw std::invalid_argument(err_msg);
    }

    Server s(argv[1]);
    for (int count = 0; count < ACCEPT_COUNT; ++count) {
        s.accept_and_send_back();
    }

    return 0;
}

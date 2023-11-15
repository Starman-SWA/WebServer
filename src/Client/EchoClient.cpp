#include <iostream>
#include <sys/socket.h>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

const int BUF_SIZE = 512;

class Client {
public:
    Client(const std::string& ip, const std::string& port) {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            throw std::runtime_error("socket() err");
        }

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        if (inet_aton(ip.c_str(), &serv_addr.sin_addr) == 0) {
            throw std::invalid_argument("ip address format error");
        }
        serv_addr.sin_port = htons(stoi(port));

        if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
            throw std::runtime_error("connect() err");
        }
    }

    [[nodiscard]] ssize_t send_msg() const {
        std::string msg;
        std::cin >> msg;
        if (msg == "Q") {
            return -1;
        }

        return write(sock, msg.c_str(), (msg.size()+1)*sizeof(char));
    }

    [[nodiscard]] ssize_t read_msg(ssize_t write_len) const {
        std::vector<char> recv_buf(BUF_SIZE);

        ssize_t read_len = 0;
        while (read_len < write_len) {
            ssize_t this_read_len = read(sock, &recv_buf[0], recv_buf.size() - 1);
            if (this_read_len <= 0) {
                return this_read_len;
            }
            read_len += this_read_len;

            recv_buf[this_read_len] = '\0';
            std::cout << "receive length " << this_read_len << ", message: " << std::string(&recv_buf[0]) << std::endl;
        }

        return read_len;
    }

    ~Client() {
        close(sock);
    }
private:
    int sock;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::string err_msg = "Usage: " + std::string(argv[0]) + " <IP> <port>";
        throw std::invalid_argument(err_msg);
    }

    Client c(argv[1], argv[2]);
    ssize_t write_len, read_len;
    while (true) {
        if ((write_len = c.send_msg()) <= 0) {
            std::cout << "write() return " << write_len << std::endl;
            break;
        }
        if ((read_len = c.read_msg(write_len)) <= 0) {
            std::cout << "read() return " << read_len << std::endl;
            break;
        }
    }

    return 0;
}
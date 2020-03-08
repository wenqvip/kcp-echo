#include "ikcp.h"
#include "argparse.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>

struct kcp_user
{
    int sockfd;
};

int create_socket(const char* host, int port)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = port;
    connect(sockfd, (const sockaddr*)&addr, sizeof(addr));

    std::cout << "created socket fd for " << host << ":" << port;
    return sockfd;
}

int udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    kcp_user* kuser = (kcp_user*)user;
    return send(kuser->sockfd, buf, len, 0);
}

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("kcp test");
    parser.add_argument("-s", "--server")
        .help("server mode");
    parser.add_argument("-c", "--client")
        .help("client mode");
    parser.add_argument("host")
        .help("remote ip, default: 127.0.0.1")
        .default_value("127.0.0.1");
    parser.add_argument("port")
        .help("remote port, default: 1060")
        .default_value(1060);

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << parser;
        exit(0);
    }

    kcp_user user;
    user.sockfd = create_socket(
        parser.get<std::string>("host").c_str(),
        parser.get<int>("port"));
    ikcpcb *kcp = ikcp_create(0, &user);
    kcp->output = udp_output;
    ikcp_setmtu(kcp, 1472);
    return 0;
}
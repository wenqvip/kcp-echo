#include "util.h"
#include "ikcp.h"
#include "argparse.hpp"
#include "storm.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("kcp test");
    parser.add_argument("-s", "--server").help("server mode").default_value(false).implicit_value(true);
    parser.add_argument("-c", "--client").help("client mode").default_value(false).implicit_value(true);
    parser.add_argument("host")
        .help("remote ip, default: 127.0.0.1")
        .default_value(std::string("127.0.0.1"))
        .required();
    parser.add_argument("port")
        .help("remote port, default: 1060")
        .action([](const std::string& value) { return std::stoi(value); })
        .default_value(1060)
        .required();

    try {
        parser.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << err.what() << std::endl;
        std::cout << parser;
        return -1;
    }
    bool server_mode = (parser["-s"] == true);

    parser["-c"];

    bool client_mode = (parser["-c"] == true);
    if (!server_mode && !client_mode)
        server_mode = true;
    if (server_mode && client_mode)
    {
        std::cout << "you should not set server and client mode same time" << std::endl;
        std::cout << parser;
        return -1;
    }

    std::string host = parser.get<std::string>("host");
    int port = parser.get<int>("port");
    storm stm;
    stm.init();
    if (server_mode)
    {
        std::cout << "running as server at " << host << ":" << port << std::endl;
        stm.accept_session(host.c_str(), port);
    }
    else if (client_mode)
    {
        std::cout << "running as client, remote " << host << ":" << port << std::endl;
        stm.create_session(host.c_str(), port);
    }


    std::stringstream ss;
    do {
        stm.update();

        if (server_mode) {
            do {
                char buf[128] = {0};
                ssize_t count = stm.recv(buf, 128);
                if(count > 0)
                {
                    int len = count;
                    while(len > 0)
                    {
                        char ch = buf[count - len];
                        ss << ch;
                        if (ch == '\n')
                        {
                            std::string str;
                            ss >> str;
                            stm.send(str.c_str(), str.size());
                        }
                        len--;
                    }
                    if (count < 128)
                        break;
                }
                else
                {
                    break;
                }
            } while(true);
        }
        else if (client_mode) {
            do {
                char buf[128] = {0};
                ssize_t count = stm.recv(buf, 128);
                if(count > 0)
                {
                    std::cout << buf << std::endl;
                    if (count < 128)
                        break;
                }
                else
                {
                    break;
                }
            } while (true);

            // int c = getchar();
            // char buf[1];
            // buf[0] = c;
            // stm.send(buf, 1);
        }
    } while (true);

    return 0;
}
#include "util.h"
#include "ikcp.h"
#include "argparse.hpp"
#include "udp_connection.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("./kcp-echo");
    parser.add_argument("-s", "--server").help("server mode").default_value(false).implicit_value(true);
    parser.add_argument("-c", "--client").help("client mode").default_value(false).implicit_value(true);
    parser.add_argument("-l", "--logging").help("logging on").default_value(false).implicit_value(true);
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

    bool logging = (parser["-l"] == true);
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
    udp_connection _connection;
    _connection.init();
    _connection.log(logging);
    std::mutex _mutex;
    std::string input;
    if (server_mode)
    {
        std::cout << "running as server at " << host << ":" << port << std::endl;
        _connection.accept(host.c_str(), port);
    }
    else if (client_mode)
    {
        std::cout << "running as client, remote " << host << ":" << port << std::endl;
        _connection.connect(host.c_str(), port);

        std::thread([&] {
            while (true) {
                std::cin >> input;
                {
                    std::lock_guard<std::mutex> guard(_mutex);
                    _connection.send(input);
                }
            }
            }).detach();
    }

    std::stringstream ss;
    do {
        if (_connection.is_shutdown())
        {
            std::cout << "connection shutdown" << std::endl;
            break;
        }

        auto frame_begin_t = timer::since_start();
        {
            std::lock_guard<std::mutex> guard(_mutex);
            _connection.update();
        }

        if (server_mode) {
            std::string buf;
            ssize_t count = _connection.recv(buf);
            if (count > 0) {
                _connection.send(buf);
                std::cout << "echo: " << buf << std::endl;
            }
            else if (count < -1) {
                std::cout << "error when recv: " << count << std::endl;
            }
        }
        else if (client_mode) {
            std::string buf;
            ssize_t count = 0;
            {
                std::lock_guard<std::mutex> guard(_mutex);
                count = _connection.recv(buf);
            }
            if (count > 0) {
                std::cout << buf << std::endl;
            }
            else if (count < -1) {
                std::cout << "error when recv: " << count << std::endl;
            }
        }
        auto frame_end_t = timer::since_start();
        std::chrono::duration<long, std::milli> du(frame_end_t - frame_begin_t);
        using namespace std::chrono_literals;
        auto left = 2ms - du;
        //std::cout << "frame time: " << du.count() << ", left time: " << left.count() << std::endl;
        //std::cout << "time: " << frame_end_t << std::endl;
        if (left.count() > 0)
            std::this_thread::sleep_for(left);
    } while (true);

    return 0;
}
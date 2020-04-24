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
#include <vector>

int main(int argc, const char* argv[])
{
    argparse::ArgumentParser parser("./kcp-echo");
    parser.add_argument("-s", "--server").help("server mode").default_value(false).implicit_value(true);
    parser.add_argument("-c", "--client").help("client mode").default_value(false).implicit_value(true);
    parser.add_argument("-l", "--logging").help("logging on").default_value(false).implicit_value(true);
    parser.add_argument("-d", "--delay").help("show delay time").default_value(false).implicit_value(true);
    parser.add_argument("-p", "--ping-pong").help("perform ping-pong test(for 1 minute)").default_value(false).implicit_value(true);
    parser.add_argument("host")
        .help("default: 127.0.0.1")
        .default_value(std::string("127.0.0.1"));
    parser.add_argument("port")
        .help("default: 1060")
        .action([](const std::string& value) { return std::stoi(value); })
        .default_value(1060);

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
    bool show_delay = (parser["-d"] == true);
    bool ping_pong = (parser["-p"] == true);
    uint32_t ping_pong_times = 0;
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
    UdpConnection udp_connection;
    udp_connection.Init();
    udp_connection.enable_log(logging);
    std::mutex _mutex;
    std::string input;
    if (server_mode)
    {
        std::cout << "running as server at " << host << ":" << port << std::endl;
        udp_connection.Accept(host.c_str(), port);
    }
    else if (client_mode)
    {
        std::cout << "running as client, remote " << host << ":" << port << std::endl;
        udp_connection.Connect(host.c_str(), port);

        if (ping_pong)
        {
            std::cout << "perform ping-pong test..." << std::endl;
            int64_t now_t = Timer::since_start();
            std::string str((const char*)&now_t, sizeof(int64_t));
            str += "ping-pong test";
            udp_connection.Send(str);
        }
        
        std::thread([&]
        {
            while (true) {
                if (ping_pong)
                    continue;

                std::cin >> input;
                {
                    std::lock_guard<std::mutex> guard(_mutex);
                    int64_t now_t = Timer::since_start();
                    std::string str((const char*)&now_t, sizeof(int64_t));
                    input = str + input;
                    udp_connection.Send(input);
                }
            }
        }).detach();
    }

    std::vector<std::string> bufs;
    while (true)
    {
        if (udp_connection.is_timeout())
        {
            std::cout << "connection timeout" << std::endl;
            break;
        }

        int64_t frame_begin_t = Timer::since_start();
        {
            std::lock_guard<std::mutex> guard(_mutex);
            udp_connection.Update();
        }

        if (server_mode) {
            std::string buf;
            while(udp_connection.can_read())
            {
                ssize_t count = udp_connection.Recv(buf);
                if (count > 0) {
                    udp_connection.Send(buf);
                    if (count >= sizeof(int64_t))
                    {
                        if (show_delay && !ping_pong)
                            std::cout << "[deal time: " << Timer::since_start() << "] ";
                        buf = buf.substr(sizeof(int64_t));
                    }
                    if (!ping_pong)
                        std::cout << "echo: " << buf << std::endl;
                }
                else if (count < -1) {
                    std::cout << "error when recv: " << count << std::endl;
                }
            }
        }
        else if (client_mode) {
            std::string buf;
            while (udp_connection.can_read())
            {
                ssize_t count = 0;
                {
                    std::lock_guard<std::mutex> guard(_mutex);
                    count = udp_connection.Recv(buf);
                }
                if (count > 0) {
                    if (count >= sizeof(int64_t))
                    {
                        int64_t send_t = *((int64_t*)(buf.substr(0, sizeof(int64_t)).c_str()));
                        int64_t now_t = Timer::since_start();
                        if (show_delay && !ping_pong)
                            std::cout << "[send: " << send_t << ", now: " << now_t << ", delay: " << now_t - send_t << "] ";
                        buf = buf.substr(sizeof(int64_t));
                    }
                    if (!ping_pong)
                        std::cout << buf << std::endl;
                    if (ping_pong)
                    {
                        int64_t now_t = Timer::since_start();
                        if (now_t > 60000)
                        {
                            ping_pong = false;
                            std::cout << "ping-pong times: " << ping_pong_times << std::endl;
                            continue;
                        }
                        std::string str((const char*)&now_t, sizeof(int64_t));
                        str += buf;
                        udp_connection.Send(str);
                        ping_pong_times++;
                    }
                }
                else if (count < -1) {
                    std::cout << "error when recv: " << count << std::endl;
                }
            }
        }
        int64_t frame_end_t = Timer::since_start();
        std::chrono::duration<long, std::milli> du(frame_end_t - frame_begin_t);
        using namespace std::chrono_literals;
        auto left = 2ms - du;
        //std::cout << "frame time: " << du.count() << ", left time: " << left.count() << std::endl;
        //std::cout << "time: " << frame_end_t << std::endl;
        //if (left.count() > 0)
        //    std::this_thread::sleep_for(left);
        //std::this_thread::sleep_for(0ms);
    }

    return 0;
}
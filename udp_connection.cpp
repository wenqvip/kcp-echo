#include "util.h"
#include "udp_connection.h"
#include "timer.h"

#include <iostream>
#include <iomanip>
#include <functional>
#include <cstring>
#include <thread>

#ifdef LINUX
#include <fcntl.h>
#endif

static std::string heartbeat_str;

udp_connection::udp_connection(): m_kcp(nullptr), m_last_update_t(0), m_logging(true), m_heartbeat_time(0)
{
}

int udp_connection::init()
{
#if defined(_WIN64) || defined(_WIN32)
    WSADATA wsaData;
    WORD sockVersion = MAKEWORD(2, 2);
    return WSAStartup(sockVersion, &wsaData);
#endif
#ifdef LINUX
    return 0;
#endif
}

int udp_connection::de_init()
{
#if defined(_WIN64) || defined(_WIN32)
    closesocket(m_sockfd);
    return WSACleanup();
#endif
#ifdef LINUX
    return 0;
#endif
}

int udp_connection::connect(const char* host, int port)
{
    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //set_socket_blocking(m_sockfd, false);
    
    m_remote_addr.sin_family = AF_INET;
    m_remote_addr.sin_addr.s_addr = inet_addr(host);
    m_remote_addr.sin_port = htons(port);

    create_kcp();
    create_recv_thread();
    send(heartbeat_str);
    return 0;
}

int udp_connection::accept(const char* host, int port)
{
    std::memset(&m_remote_addr, 0, sizeof(m_remote_addr));

    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //set_socket_blocking(m_sockfd, false);
    sockaddr_in addr_info;
    addr_info.sin_family = AF_INET;
    addr_info.sin_addr.s_addr = inet_addr(host);
    addr_info.sin_port = htons(port);
    ::bind(m_sockfd, (const sockaddr*)&addr_info, sizeof(addr_info));

    create_kcp();
    create_recv_thread();
    return 0;
}

void udp_connection::create_kcp()
{
    m_kcp = ikcp_create(0xCBCBCBCB, this);
    m_kcp->output = udp_connection::udp_output;
    if (m_logging)
    {
        m_kcp->logmask = IKCP_LOG_IN_ACK | IKCP_LOG_OUT_ACK
                    | IKCP_LOG_IN_DATA | IKCP_LOG_OUT_DATA;
    }
    m_kcp->writelog = log_callback;
    ikcp_setmtu(m_kcp, MTU);
    //ikcp_nodelay(m_kcp, 0, 100, 0, 0);
}

void udp_connection::recv_listener()
{
    while (true)
    {
        char buf[MTU] = {0};
        sockaddr_in addrinfo;
        socklen_t len = sizeof(addrinfo);
        ssize_t count = ::recvfrom(m_sockfd, buf, MTU, 0, (sockaddr*)&addrinfo, &len);
        if (count > 0)
        {
            if (is_waiting())
            {
                std::memcpy(&m_remote_addr, &addrinfo, sizeof(m_remote_addr));
                std::cout << "new connect from " << inet_ntoa(addrinfo.sin_addr) << ":" << ntohs(addrinfo.sin_port) << std::endl;
                send(heartbeat_str);
            }

            if (addrinfo.sin_addr.s_addr == m_remote_addr.sin_addr.s_addr)
            {
                std::string data(buf, count);
                std::lock_guard<std::mutex> guard(m_recv_mutex);
                m_recv_queue.push(data);
            }
        }
        else if (count < 0)
        {
#if defined(_WIN64) || defined(_WIN32)
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
                std::cout << "error " << error << " occurs when call recvfrom" << std::endl;
#endif
        }
    }
}

void udp_connection::create_recv_thread()
{
    std::thread(std::bind(std::mem_fn(&udp_connection::recv_listener), this)).detach();
}

size_t udp_connection::send(std::string& data)
{
    return this->send(data.c_str(), data.size());
}

size_t udp_connection::send(const char* buf, size_t len)
{
    if (is_timeout())
        return 0;

    int ret = ikcp_send(m_kcp, buf, len);
    if (ret < 0)
    {
        std::cout << "sending error" << std::endl;
        return 0;
    }
    else
    {
        ikcp_flush(m_kcp);
    }
    return ret;
}

void udp_connection::flush()
{
    ikcp_flush(m_kcp);
}

bool udp_connection::can_read()
{
    return m_kcp->nrcv_que > 0;
}

ssize_t udp_connection::recv(std::string& data)
{
    if (m_kcp->nrcv_que > 0)
    {
        int data_size = ikcp_peeksize(m_kcp);
        if (data_size > 0)
            data.resize(data_size);
        return ikcp_recv(m_kcp, data.data(), data_size);
    }
    return 0;
}

bool udp_connection::is_waiting()
{
    return m_remote_addr.sin_addr.s_addr == 0;
}

bool udp_connection::is_timeout()
{
    return m_kcp->state != 0;
}

void udp_connection::update()
{
    int time_now = timer::since_start();
    m_heartbeat_time += time_now - m_last_update_t;
    //发送心跳包，不发心跳包处于NAT后的端过段时间就无法连上了
    if (!is_waiting() && !is_timeout() && m_heartbeat_time > m_heartbeat_interval)
    {
        m_heartbeat_time = 0;
        ikcp_send(m_kcp, nullptr, 0);
    }

    ikcp_update(m_kcp, time_now);
    m_last_update_t = time_now;

    std::queue<std::string> recv_queue;
    {
        std::lock_guard<std::mutex> guard(m_recv_mutex);
        recv_queue.swap(m_recv_queue);
    }
    while (recv_queue.size() > 0)
    {
        std::string& data = recv_queue.front();

        ikcp_input(m_kcp, data.c_str(), data.size());
        if (m_kcp->ackcount > 0)
            ikcp_flush(m_kcp);
            
        recv_queue.pop();
    }
}

int udp_connection::udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    udp_connection* pConnection = (udp_connection*)user;
    ssize_t ret = ::sendto(pConnection->m_sockfd, buf, len, 0, (const sockaddr*)&(pConnection->m_remote_addr), sizeof(sockaddr_in));
    if (ret <= 0)
        std::cout << "sending error" << std::endl;
    else
        pConnection->m_heartbeat_time = 0;//optimize heart beat
    return ret;
}

void udp_connection::log_callback(const char* log, struct IKCPCB* kcp, void* user)
{
    std::cout << "[" << std::setfill(' ') << std::setw(10)
        << std::setiosflags(std::ios::right) << timer::since_start() << "] " << log << std::endl;
}

bool udp_connection::set_socket_blocking(int fd, bool blocking)
{
#ifdef LINUX
    if (fd < 0) return false;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif

#if defined(_WIN64) || defined(_WIN32)
    u_long mode = blocking ? 0 : 1;
    return ioctlsocket(fd, FIONBIO, &mode) == NO_ERROR ? true : false;
#endif
}

void udp_connection::log(const char* prefix, const char* buf, size_t len)
{
    std::cout << prefix << len << " bytes: ";
    if (len >= 24 && *((int*)buf) == 0xcbcbcbcb)
    {
        std::cout << str_to_hex(buf, 24);
        buf += 24;
        len -= 24;
    }
    std::cout << std::string(buf, len) << std::endl;
}
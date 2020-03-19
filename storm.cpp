#include "util.h"
#include "storm.h"

#include <iostream>
#include <functional>
#include <cstring>

#ifdef LINUX
#include <fcntl.h>
#endif

storm::storm(): m_kcp(nullptr), m_can_read(false), m_last_update_t(0), m_logging(true)
{
}

int storm::init()
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

int storm::de_init()
{
#if defined(_WIN64) || defined(_WIN32)
    closesocket(m_sockfd);
    return WSACleanup();
#endif
#ifdef LINUX
    return 0;
#endif
}

int storm::create_session(const char* host, int port)
{
    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_socket_blocking(m_sockfd, false);
    
    m_remote_addr.sin_family = AF_INET;
    m_remote_addr.sin_addr.s_addr = inet_addr(host);
    m_remote_addr.sin_port = htons(port);

    create_kcp();
    return send(nullptr, 0);
}

int storm::accept_session(const char* host, int port)
{
    std::memset(&m_remote_addr, 0, sizeof(m_remote_addr));

    m_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_socket_blocking(m_sockfd, false);
    sockaddr_in addr_info;
    addr_info.sin_family = AF_INET;
    addr_info.sin_addr.s_addr = inet_addr(host);
    addr_info.sin_port = htons(port);
    ::bind(m_sockfd, (const sockaddr*)&addr_info, sizeof(addr_info));

    create_kcp();
    return 0;
}

void storm::create_kcp()
{
    m_kcp = ikcp_create(0xCBCBCBCB, this);
    m_kcp->output = storm::udp_output;
    m_kcp->logmask = 0xFFFF;
    m_kcp->writelog = log_callback;
    m_kcp->rcv_wnd = 4096;
    m_kcp->snd_wnd = 4096;
    ikcp_setmtu(m_kcp, 1472);
    //ikcp_nodelay(m_kcp, 1, 10, 2, 1);
}

size_t storm::send(const char* buf, size_t len)
{
    int ret = ikcp_send(m_kcp, buf, len);
    if (ret < 0)
        std::cout << "sending error" << std::endl;
    return ret;
}

bool storm::can_read()
{
    return m_can_read;
}

ssize_t storm::recv(char* buf, size_t len)
{
    return ikcp_recv(m_kcp, buf, len);
}

bool storm::wait_remote()
{
    return m_remote_addr.sin_addr.s_addr == 0;
}

void storm::update()
{
    static int cumulative_time = 0;
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock().now();
    int time_now = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    cumulative_time += time_now - m_last_update_t;
    if (wait_remote() && cumulative_time > 10000)
    {
        cumulative_time = 0;
        ikcp_send(m_kcp, nullptr, 0);
    }
    if (time_now > m_last_update_t + 10)
    {
        ikcp_update(m_kcp, time_now);
        m_last_update_t = time_now;
    }

    char buf[1472] = {0};
    sockaddr_in addrinfo;
    socklen_t len = sizeof(addrinfo);
    ssize_t count = ::recvfrom(m_sockfd, buf, 1472, 0, (sockaddr*)&addrinfo, &len);

    if (count > 0)
    {
        if (wait_remote())
        {
            std::memcpy(&m_remote_addr, &addrinfo, sizeof(m_remote_addr));
            std::cout << "new connect from " << inet_ntoa(addrinfo.sin_addr) << ":" << ntohs(addrinfo.sin_port) << std::endl;
            send(nullptr, 0);
        }

        if (addrinfo.sin_addr.s_addr == m_remote_addr.sin_addr.s_addr)
        {
            m_can_read = true;
            ikcp_input(m_kcp, buf, count);
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

int storm::udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    storm* pstorm = (storm*)user;
    ssize_t ret = ::sendto(pstorm->m_sockfd, buf, len, 0, (const sockaddr*)&(pstorm->m_remote_addr), sizeof(sockaddr_in));
    if (ret <= 0)
        std::cout << "sending error" << std::endl;
    return ret;
}

void storm::log_callback(const char* log, struct IKCPCB* kcp, void* user)
{
    std::cout << log << std::endl;
}

bool storm::set_socket_blocking(int fd, bool blocking)
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

void storm::log(const char* prefix, const char* buf, size_t len)
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
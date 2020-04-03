#include "util.h"
#include "storm.h"
#include "timer.h"

#include <iostream>
#include <iomanip>
#include <functional>
#include <cstring>

#ifdef LINUX
#include <fcntl.h>
#endif

static std::string keepalive_str;

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
    return send(keepalive_str);
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
    m_kcp->logmask = IKCP_LOG_IN_ACK | IKCP_LOG_OUT_ACK
                    | IKCP_LOG_IN_DATA | IKCP_LOG_OUT_DATA;
    m_kcp->writelog = log_callback;
    ikcp_setmtu(m_kcp, MTU);
    ikcp_nodelay(m_kcp, 0, 10, 0, 0);
}

size_t storm::send(std::string& data)
{
    int ret = ikcp_send(m_kcp, data.c_str(), data.size());
    if (ret < 0)
        std::cout << "sending error" << std::endl;
    return ret;
}

void storm::flush()
{
    ikcp_flush(m_kcp);
}

bool storm::can_read()
{
    return m_can_read;
}

ssize_t storm::recv(std::string& data)
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

bool storm::wait_remote()
{
    return m_remote_addr.sin_addr.s_addr == 0;
}

void storm::update()
{
    static int cumulative_time = 0;
    int time_now = timer::since_start();
    cumulative_time += time_now - m_last_update_t;
    //发送心跳包，不发心跳包处于NAT后的端过段时间就无法连上了
    if (!wait_remote() && cumulative_time > 10000)
    {
        cumulative_time = 0;
        ikcp_send(m_kcp, nullptr, 0);
    }

    ikcp_update(m_kcp, time_now);
    m_last_update_t = time_now;

    char buf[MTU] = {0};
    sockaddr_in addrinfo;
    socklen_t len = sizeof(addrinfo);
    ssize_t count = ::recvfrom(m_sockfd, buf, MTU, 0, (sockaddr*)&addrinfo, &len);

    if (count > 0)
    {
        if (wait_remote())
        {
            std::memcpy(&m_remote_addr, &addrinfo, sizeof(m_remote_addr));
            std::cout << "new connect from " << inet_ntoa(addrinfo.sin_addr) << ":" << ntohs(addrinfo.sin_port) << std::endl;
            send(keepalive_str);
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
    std::cout << "[" << std::setfill(' ') << std::setw(10)
        << std::setiosflags(std::ios::right) << timer::since_start() << "] " << log << std::endl;
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
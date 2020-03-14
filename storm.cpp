#include "storm.h"

#include <iostream>
#include <functional>
#include <cstring>

#include <fcntl.h>

storm::storm(): m_kcp(nullptr), m_can_read(false), m_last_update_t(0)
{

}

void storm::create_session(const char* host, int port)
{
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    set_socket_blocking(m_sockfd, false);
    
    m_remote_addr.sin_family = AF_INET;
    m_remote_addr.sin_addr.s_addr = inet_addr(host);
    m_remote_addr.sin_port = port;

    create_kcp();
    send(nullptr, 0);
}

void storm::accept_session(const char* host, int port)
{
    std::memset(&m_remote_addr, 0, sizeof(m_remote_addr));

    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    set_socket_blocking(m_sockfd, false);
    sockaddr_in addr_info;
    addr_info.sin_family = AF_INET;
    addr_info.sin_addr.s_addr = inet_addr(host);
    addr_info.sin_port = port;
    ::bind(m_sockfd, (const sockaddr*)&addr_info, sizeof(addr_info));

    create_kcp();
}

void storm::create_kcp()
{
    m_kcp = ikcp_create(0, this);
    m_kcp->output = storm::udp_output;
    ikcp_setmtu(m_kcp, 1472);
    ikcp_nodelay(m_kcp, 1, 10, 2, 1);
}

size_t storm::send(const char* buf, size_t len)
{
    std::cout << "sending " << len << " byets:" << std::string(buf, len) << std::endl;
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

void storm::update()
{
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock().now();
    int time_now = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    if (time_now > m_last_update_t + 100)
    {
        //std::cout << "storm update" << std::endl;
        ikcp_update(m_kcp, time_now);
        m_last_update_t = time_now;
    }

    char buf[128] = {0};
    sockaddr_in addrinfo;
    socklen_t len;
    ssize_t count = ::recvfrom(m_sockfd, buf, 128, 0, (sockaddr*)&addrinfo, &len);

    if (count > 0)
    {
        if (m_remote_addr.sin_addr.s_addr == 0)
        {
            std::memcpy(&m_remote_addr, &addrinfo, sizeof(m_remote_addr));
        }

        if (addrinfo.sin_addr.s_addr == m_remote_addr.sin_addr.s_addr)
        {
            std::cout << "receive " << std::string(buf, count) << std::endl;
            m_can_read = true;
            ikcp_input(m_kcp, buf, count);
        }
    }
}

int storm::udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    std::cout << "use udp sending " << len << " bytes" << std::endl;
    storm* pstorm = (storm*)user;
    ssize_t ret = ::sendto(pstorm->m_sockfd, buf, len, 0, (const sockaddr*)&(pstorm->m_remote_addr), sizeof(sockaddr_in));
    if (ret <= 0)
        std::cout << "sending error" << std::endl;
    return ret;
}

bool storm::set_socket_blocking(int fd, bool blocking)
{
    if (fd < 0) return false;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}
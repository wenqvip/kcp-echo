#include "storm.h"

#include <iostream>
#include <thread>
#include <functional>

#include <fcntl.h>

storm::storm(): m_kcp(nullptr), m_can_read(false)
{

}

void storm::create_session(const char* host, int port)
{
    m_tp_now = std::chrono::steady_clock().now();
    m_user.sockfd = create_socket(host, port);
    m_kcp = ikcp_create(0, &m_user);
    m_kcp->output = storm::udp_output;
    ikcp_setmtu(m_kcp, 1472);

    std::thread(std::bind(std::mem_fn(&storm::loop), this)).detach();
}

size_t storm::send(const char* buf, size_t len)
{
    return ikcp_send(m_kcp, buf, len);
}

bool storm::can_read()
{
    return m_can_read;
}

size_t storm::recv(char* buf, size_t len)
{
    return ikcp_recv(m_kcp, buf, len);
}

void storm::loop()
{
    while(true)
    {
        m_tp_now = std::chrono::steady_clock().now();
        int time_now = m_tp_now.time_since_epoch().count();
        if (ikcp_check(m_kcp, time_now))
            ikcp_update(m_kcp, time_now);

        do {
            char buf[128] = {0};
            size_t count = ::recv(m_user.sockfd, buf, 128, 0);
            if (count > 0)
            {
                m_can_read = true;
                ikcp_input(m_kcp, buf, count);
                if(count < 128)
                    break;
            }
            else
            {
                break;
            }
        } while(true);
    }
}

int storm::create_socket(const char* host, int port)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = port;
    connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
    set_socket_blocking(sockfd, false);

    return sockfd;
}

int storm::udp_output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    kcp_user* kuser = (kcp_user*)user;
    return ::send(kuser->sockfd, buf, len, 0);
}

bool storm::set_socket_blocking(int fd, bool blocking)
{
    if (fd < 0) return false;

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}
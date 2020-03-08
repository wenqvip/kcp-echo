#include "storm.h"
#include <iostream>

void Storm::CreateSession(const char* host, int port)
{
    KcpUser user;
    user.sockfd = CreateSocket(host, port);
    m_kcp = ikcp_create(0, &user);
    m_kcp->output = Storm::UdpOutput;
    ikcp_setmtu(m_kcp, 1472);
}

void Storm::Loop()
{
    if (ikcp_check(m_kcp, 0))
        ikcp_update(m_kcp, 0);
}

int Storm::CreateSocket(const char* host, int port)
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = port;
    connect(sockfd, (const sockaddr*)&addr, sizeof(addr));

    return sockfd;
}

int Storm::UdpOutput(const char* buf, int len, ikcpcb* kcp, void* user)
{
    KcpUser* kuser = (KcpUser*)user;
    return send(kuser->sockfd, buf, len, 0);
}
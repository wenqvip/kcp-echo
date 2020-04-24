#include "util.h"
#include "udp_connection.h"
#include "timer.h"

#include <iostream>
#include <iomanip>
#include <functional>
#include <cstring>

#ifdef LINUX
#include <fcntl.h>
#endif

static std::string heartbeat_str;

UdpConnection::UdpConnection(): kcp_(nullptr), last_update_time_(0), enable_log_(true), heartbeat_time_(0)
{
}

int UdpConnection::Init()
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

int UdpConnection::DeInit()
{
#if defined(_WIN64) || defined(_WIN32)
    closesocket(sockfd_);
    return WSACleanup();
#endif
#ifdef LINUX
    return 0;
#endif
}

int UdpConnection::Connect(const char* host, int port)
{
    sockfd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_socket_blocking(sockfd_, false);
    
    remote_addr_.sin_family = AF_INET;
    remote_addr_.sin_addr.s_addr = inet_addr(host);
    remote_addr_.sin_port = htons(port);

    CreateKcp();
    return Send(heartbeat_str);
}

int UdpConnection::Accept(const char* host, int port)
{
    std::memset(&remote_addr_, 0, sizeof(remote_addr_));

    sockfd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_socket_blocking(sockfd_, false);
    sockaddr_in addr_info;
    addr_info.sin_family = AF_INET;
    addr_info.sin_addr.s_addr = inet_addr(host);
    addr_info.sin_port = htons(port);
    ::bind(sockfd_, (const sockaddr*)&addr_info, sizeof(addr_info));

    CreateKcp();
    return 0;
}

void UdpConnection::CreateKcp()
{
    kcp_ = ikcp_create(0xCBCBCBCB, this);
    kcp_->output = UdpConnection::Output;
    if (enable_log_)
    {
        kcp_->logmask = IKCP_LOG_IN_ACK | IKCP_LOG_OUT_ACK
                    | IKCP_LOG_IN_DATA | IKCP_LOG_OUT_DATA;
    }
    kcp_->writelog = UdpConnection::WriteLog;
    ikcp_setmtu(kcp_, kMTU);
    //ikcp_nodelay(kcp_, 0, 100, 0, 0);
}

size_t UdpConnection::Send(std::string& data)
{
    return this->Send(data.c_str(), data.size());
}

size_t UdpConnection::Send(const char* buf, size_t len)
{
    if (is_timeout())
        return 0;

    int ret = ikcp_send(kcp_, buf, len);
    if (ret < 0)
    {
        std::cout << "sending error" << std::endl;
        return 0;
    }
    else
    {
        ikcp_flush(kcp_);
    }
    return ret;
}

void UdpConnection::Flush()
{
    ikcp_flush(kcp_);
}

bool UdpConnection::can_read()
{
    return kcp_->nrcv_que > 0;
}

ssize_t UdpConnection::Recv(std::string& data)
{
    if (kcp_->nrcv_que > 0)
    {
        int data_size = ikcp_peeksize(kcp_);
        if (data_size > 0)
            data.resize(data_size);
        return ikcp_recv(kcp_, data.data(), data_size);
    }
    return 0;
}

bool UdpConnection::is_waiting()
{
    return remote_addr_.sin_addr.s_addr == 0;
}

bool UdpConnection::is_timeout()
{
    return kcp_->state != 0;
}

void UdpConnection::Update()
{
    int time_now = Timer::since_start();
    heartbeat_time_ += time_now - last_update_time_;
    //发送心跳包，不发心跳包处于NAT后的端过段时间就无法连上了
    if (!is_waiting() && !is_timeout() && heartbeat_time_ > heartbeat_interval_)
    {
        heartbeat_time_ = 0;
        ikcp_send(kcp_, nullptr, 0);
    }

    ikcp_update(kcp_, time_now);
    last_update_time_ = time_now;

    char buf[kMTU] = {0};
    sockaddr_in addrinfo;
    socklen_t len = sizeof(addrinfo);
    ssize_t count = ::recvfrom(sockfd_, buf, kMTU, 0, (sockaddr*)&addrinfo, &len);

    if (count > 0)
    {
        if (is_waiting())
        {
            std::memcpy(&remote_addr_, &addrinfo, sizeof(remote_addr_));
            std::cout << "new connect from " << inet_ntoa(addrinfo.sin_addr) << ":" << ntohs(addrinfo.sin_port) << std::endl;
            Send(heartbeat_str);
        }

        if (addrinfo.sin_addr.s_addr == remote_addr_.sin_addr.s_addr)
        {
            ikcp_input(kcp_, buf, count);
            if (kcp_->ackcount > 0)
                ikcp_flush(kcp_);
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

int UdpConnection::Output(const char* buf, int len, ikcpcb* kcp, void* user)
{
    UdpConnection* pConnection = (UdpConnection*)user;
    ssize_t ret = ::sendto(pConnection->sockfd_, buf, len, 0, (const sockaddr*)&(pConnection->remote_addr_), sizeof(sockaddr_in));
    if (ret <= 0)
        std::cout << "sending error" << std::endl;
    else
        pConnection->heartbeat_time_ = 0;//optimize heart beat
    return ret;
}

void UdpConnection::WriteLog(const char* log, struct IKCPCB* kcp, void* user)
{
    std::cout << "[" << std::setfill(' ') << std::setw(10)
        << std::setiosflags(std::ios::right) << Timer::since_start() << "] " << log << std::endl;
}

bool UdpConnection::set_socket_blocking(int fd, bool blocking)
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

void UdpConnection::Log(const char* prefix, const char* buf, size_t len)
{
    std::cout << prefix << len << " bytes: ";
    if (len >= 24 && *((int*)buf) == 0xcbcbcbcb)
    {
        std::cout << StringToHex(buf, 24);
        buf += 24;
        len -= 24;
    }
    std::cout << std::string(buf, len) << std::endl;
}
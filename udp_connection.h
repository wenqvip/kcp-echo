#pragma once
#include "util.h"
#include "ikcp.h"
#include <string>

class UdpConnection
{
public:
    UdpConnection();
    int Init();
    int DeInit();
    int Connect(const char* host, int port);
    int Accept(const char* host, int port);
    size_t Send(std::string& data);
    size_t Send(const char* buf, size_t len);
    ssize_t Recv(std::string& data);
    void Update();
    void Flush();

    inline void enable_log(bool enable) { enable_log_ = enable; }

    bool can_read();
    bool is_waiting();
    bool is_timeout();
    void set_heartbeat_interval(uint32_t ms) { heartbeat_interval_ = ms; }
    bool set_socket_blocking(int fd, bool blocking);

protected:
    void CreateKcp();
    static int Output(const char* buf, int len, ikcpcb* kcp, void* user);
    static void WriteLog(const char* log, struct IKCPCB* kcp, void* user);
    void Log(const char* prefix, const char* buf, size_t len);

    static const int kMTU = 1472;

private:
    ikcpcb* kcp_;
    long last_update_time_;
    uint32_t heartbeat_interval_ = 30000;
    uint32_t heartbeat_time_ = 0;
    socket_t sockfd_;
    sockaddr_in remote_addr_;
    bool enable_log_;
};
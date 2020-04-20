#pragma once
#include "util.h"
#include "ikcp.h"
#include <string>
#include <queue>

class udp_connection
{
public:
    udp_connection();
    int init();
    int de_init();
    int connect(const char* host, int port);
    int accept(const char* host, int port);
    size_t send(std::string& data);
    size_t send(const char* buf, size_t len);
    ssize_t recv(std::string& data);
    void update();
    void flush();

    inline void log(bool on) { m_logging = on; }

    bool can_read();
    bool is_waiting();
    bool is_timeout();
    void set_heartbeat_interval(uint32_t ms) { m_heartbeat_interval = ms; }

protected:
    void create_kcp();
    void recv_listener();
    void create_recv_thread();
    static int udp_output(const char* buf, int len, ikcpcb* kcp, void* user);
    static void log_callback(const char* log, struct IKCPCB* kcp, void* user);
    bool set_socket_blocking(int fd, bool blocking);
    void log(const char* prefix, const char* buf, size_t len);

    static const int MTU = 1472;

private:
    ikcpcb* m_kcp;
    long m_last_update_t;
    uint32_t m_heartbeat_interval = 30000;
    uint32_t m_heartbeat_time = 0;
    socket_t m_sockfd;
    sockaddr_in m_remote_addr;
    std::queue<std::string> m_recv_queue;
    std::mutex m_recv_mutex;
    bool m_logging;
};
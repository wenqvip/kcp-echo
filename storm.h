#pragma once
#include "util.h"
#include "ikcp.h"

class storm
{
public:
    storm();
    int init();
    int de_init();
    int create_session(const char* host, int port);
    int accept_session(const char* host, int port);
    size_t send(const char* buf, size_t len);
    bool can_read();
    ssize_t recv(char* buf, size_t len);
    void update();
    void flush();
    inline void log(bool on) { m_logging = on; }
    bool wait_remote();

protected:
    void create_kcp();
    static int udp_output(const char* buf, int len, ikcpcb* kcp, void* user);
    static void log_callback(const char* log, struct IKCPCB* kcp, void* user);
    bool set_socket_blocking(int fd, bool blocking);
    void log(const char* prefix, const char* buf, size_t len);

    static const int MTU = 1472;

private:
    ikcpcb* m_kcp;
    long m_last_update_t;
    bool m_can_read;
    socket_t m_sockfd;
    sockaddr_in m_remote_addr;
    bool m_logging;
};
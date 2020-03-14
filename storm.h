#include "ikcp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <chrono>

class storm
{
public:
    storm();
    void create_session(const char* host, int port);
    void accept_session(const char* host, int port);
    void bind_port(int port);
    size_t send(const char* buf, size_t len);
    bool can_read();
    ssize_t recv(char* buf, size_t len);
    void update();

protected:
    void create_kcp();
    static int udp_output(const char* buf, int len, ikcpcb* kcp, void* user);
    bool set_socket_blocking(int fd, bool blocking);

private:
    ikcpcb* m_kcp;
    long m_last_update_t;
    bool m_can_read;
    int m_sockfd;
    sockaddr_in m_remote_addr;
};
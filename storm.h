#include "ikcp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <chrono>

class storm
{
public:
    struct kcp_user
    {
        int sockfd;
    };
public:
    storm();
    void create_session(const char* host, int port);
    size_t send(const char* buf, size_t len);
    bool can_read();
    size_t recv(char* buf, size_t len);

protected:
    int create_socket(const char* host, int port);
    void loop();
    static int udp_output(const char* buf, int len, ikcpcb* kcp, void* user);
    bool set_socket_blocking(int fd, bool blocking);

private:
    ikcpcb* m_kcp;
    kcp_user m_user;
    std::chrono::time_point<std::chrono::steady_clock> m_tp_now;
    bool m_can_read;
};
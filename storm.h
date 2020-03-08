#include "ikcp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Storm
{
public:
    struct KcpUser
    {
        int sockfd;
    };
public:
    Storm();
    void CreateSession(const char* host, int port);
    void Loop();

protected:
    int CreateSocket(const char* host, int port);
    static int UdpOutput(const char* buf, int len, ikcpcb* kcp, void* user);

private:
    ikcpcb* m_kcp;
};
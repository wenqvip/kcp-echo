#include <string>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#if defined(_WIN64) || defined(_WIN32)
#include <winsock2.h>  
#pragma comment(lib, "ws2_32.lib")

typedef long ssize_t;
typedef int socklen_t;
typedef SOCKET socket_t;
#endif

#ifdef LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef int socket_t;
#endif

std::string str_to_hex(const char* str, int len);
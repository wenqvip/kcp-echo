#pragma once

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
#include <unistd.h>

typedef int socket_t;
#endif

#include <string>
#include <cstdint>

std::string StringToHex(const char* str, int len);
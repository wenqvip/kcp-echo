#include <string>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#if defined(_WIN64) || defined(_WIN32)
typedef long ssize_t;
typedef int socklen_t;
#endif

std::string str_to_hex(const char* str, int len);
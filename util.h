#include <string>

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#if defined(WIN64) || defined(WIN32)
typedef long ssize_t;
#endif

std::string str_to_hex(const char* str, int len);
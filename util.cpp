#include "util.h"
#include <sstream>
#include <iomanip>


std::string str_to_hex(const char* str, int len)
{
    std::stringstream ss;
    for(int i = 0; i < len; i++)
    {
        uchar uc = str[i];
        ss << '\\' << std::setw(2) <<std::setfill('0') << std::hex << uint(uc);
    }
    return ss.str();
}
#include "util.h"
#include <sstream>
#include <iomanip>


std::string StringToHex(const char* str, int len)
{
    std::stringstream ss;
    for(int i = 0; i < len; i++)
    {
        uint8_t uc = str[i];
        ss << '\\' << std::setw(2) <<std::setfill('0') << std::hex << uint32_t(uc);
    }
    return ss.str();
}
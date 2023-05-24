#include "util.h"

std::string Util::extractString(const std::string &input)
{
    std::size_t start = input.find("${");
    if (start == std::string::npos)
        return ""; 

    std::size_t end = input.find("}", start + 2);
    if (end == std::string::npos)
        return "";

    return input.substr(start + 2, end - start - 2);
}
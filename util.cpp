#include "util.h"

std::string Util::extractString(const char *input)
{
    std::string result;
    std::string inputStr(input);

    size_t startPos = inputStr.find("${");
    if (startPos != std::string::npos)
    {
        size_t endPos = inputStr.find("}", startPos);
        if (endPos != std::string::npos)
        {
            result = inputStr.substr(startPos + 2, endPos - startPos - 2);
        }
    }

    return result;
}
#ifndef UTIL_H
#define UTIL_H

#include <Arduino.h>
#include <string>

class Util
{
public:
    static std::string extractString(const char *input);
};

#endif
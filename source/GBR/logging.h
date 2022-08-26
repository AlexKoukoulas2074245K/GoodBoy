#ifndef Logging_h
#define Logging_h

#include "types.h"

#include <stdarg.h> // va_list, va_start, va_end
#include <stdio.h>  // vprintf, printf
#include <string>
#include <sstream>
#include <iomanip>

#define LOG_IN_RELEASE


enum class LogType
{
    INFO, WARNING, ERROR
};


#if !defined(NDEBUG) || defined(LOG_IN_RELEASE)
inline void log(const LogType logType, const char* message, ...)
{

    switch (logType)
    {
    case LogType::INFO: printf("[INFO] "); break;
    case LogType::WARNING: printf("[WARNING] "); break;
    case LogType::ERROR: printf("[ERROR] "); break;
    }

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}

inline std::string getHexByte(const byte b) 
{
    std::stringstream s;
    s << "0x" << std::setfill('0') << std::setw(2) << std::right << std::hex << std::uppercase << static_cast<word>(b);
    return s.str();
}

inline std::string getHexWord(const word w)
{
    std::stringstream s;
    s << "0x" << std::setfill('0') << std::setw(4) << std::right << std::hex << std::uppercase << w;
    return s.str();
}

#else
inline void log(const LogType, const char*, ...) {}
inline std::string getHexByte(const byte b) { return std::string(); }
inline std::string getHexWord(const word w) { return std::string(); }
#endif /* not NDEBUG */

#endif /* Logging_h */

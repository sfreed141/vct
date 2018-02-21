#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#define ASCII_CSI "\x1b["
#define ASCII_RED "\x1b[31m"
#define ASCII_GRN "\x1b[32m"
#define ASCII_YEL "\x1b[33m"
#define ASCII_BLU "\x1b[34m"
#define ASCII_MAG "\x1b[35m"
#define ASCII_CYN "\x1b[36m"
#define ASCII_WHT "\x1b[37m"
#define ASCII_RST "\x1b[0m"

enum LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };
static const char *logStrings[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR" };
static const char *logColors[] = { ASCII_RST, ASCII_BLU, ASCII_GRN, ASCII_YEL, ASCII_RED };

#define LOG_TRACE(args...) LOG(TRACE, __FILE__, __LINE__, args)
#define LOG_DEBUG(args...) LOG(DEBUG, __FILE__, __LINE__, args)
#define LOG_INFO(args...) LOG(INFO, __FILE__, __LINE__, args)
#define LOG_WARN(args...) LOG(WARN, __FILE__, __LINE__, args)
#define LOG_ERROR(args...) LOG(ERROR, __FILE__, __LINE__, args)

// c++ magic https://en.wikipedia.org/wiki/Variadic_template
struct pass {
    template<typename ...Args> pass(Args...) {}
};

template<typename ...Args>
void LOG(LogLevel level, const char *file, int line, Args &&...args) {
    std::cerr
        << logColors[level] << "[" << logStrings[level] << "]" << ASCII_RST
        << "\t(" << file << ":" << line << ")\t";
    pass{(std::cerr << args, 1)...};
    std::cerr << std::endl;
}

#endif

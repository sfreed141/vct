#ifndef LOG_H
#define LOG_H

#include <iostream>

#ifdef _WIN32
// Windows hates colors :(
// maybe worth adding... https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
#define ASCII_RST ""
static const char *logColors[] = { "", "", "", "", "" };
#else
#define ASCII_CSI "\x1b["
#define ASCII_RED "\x1b[31m"
#define ASCII_GRN "\x1b[32m"
#define ASCII_YEL "\x1b[33m"
#define ASCII_BLU "\x1b[34m"
#define ASCII_MAG "\x1b[35m"
#define ASCII_CYN "\x1b[36m"
#define ASCII_WHT "\x1b[37m"
#define ASCII_RST "\x1b[0m"

static const char *logColors[] = { ASCII_RST, ASCII_BLU, ASCII_GRN, ASCII_YEL, ASCII_RED };
#endif


enum LogLevel { TRACE, DEBUG, INFO, WARN, ERROR };
static const char *logStrings[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR" };

#define LOG_TRACE(...) LOG(TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) LOG(INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) LOG(WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) LOG(ERROR, __FILE__, __LINE__, __VA_ARGS__)

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

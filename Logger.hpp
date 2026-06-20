#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <ctime>
#include <sstream>

enum LogLevel {
    LOG_OFF,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

#ifndef LOG_LEVEL
# define LOG_LEVEL LOG_WARN
#endif

#define BLUE "\033[34m"
#define RESET "\033[0m"
#define RED "\033[31m"

inline const char* logLevelName(int lvl) {
	switch (lvl) {
		case LOG_ERROR: return "ERROR";
		case LOG_WARN: return "WARN";
		case LOG_INFO: return "INFO";
		case LOG_DEBUG: return "DEBUG";
		default: 	return "HUH?";
	}
}

#define LOG(level, msg) do { \
	if (level <= LOG_LEVEL) { \
	std::time_t now = std::time(NULL); \
	char buf[26]; \
	ctime_r(&now, buf); \
	buf[24] = '\0'; /* Remove newline */ \
	if (level == LOG_ERROR) { \
	std::cerr << RED << "[" << buf << "] [" << logLevelName(level) << "] " << "grep: " << msg << RESET << std::endl; \
	} else { \
	std::cerr << BLUE << "[" << buf << "] [" << logLevelName(level) << "] " << "grep: " << msg << RESET << std::endl; } \
	} \
	} while(0)

#endif // LOGGER_HPP

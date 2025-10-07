#ifndef LOGGER_H
#define LOGGER_H

#include <string>

namespace logger {

void log(const char *message);
void log(int number, const char **messages);
void log(const std::string message);

void info(const char *message);
void info(int number, const char **messages);
void info(const std::string message);

void debug(const char *message);
void debug(int number, const char **messages);
void debug(const std::string message);

void warn(const char *message);
void warn(int number, const char **messages);
void warn(const std::string message);

void error(const char *message);
void error(int number, const char **messages);
void error(const std::string message);

void assert(bool condition, const char *message);
void assert(bool condition, int number, const char **messages);
void assert(bool condition, const std::string message);

void levelmeter(float level);

} // namespace logger

#endif // LOGGER_H
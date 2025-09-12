#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>

namespace logger {

inline void log(const char *message);
inline void log(int number, const char **messages);
inline void log(const std::string message);

inline void info(const char *message);
inline void info(int number, const char **messages);
inline void info(const std::string message);

inline void debug(const char *message);
inline void debug(int number, const char **messages);
inline void debug(const std::string message);

inline void warn(const char *message);
inline void warn(int number, const char **messages);
inline void warn(const std::string message);

inline void error(const char *message);
inline void error(int number, const char **messages);
inline void error(const std::string message);

inline void assert(bool condition, const char *message);
inline void assert(bool condition, int number, const char **messages);
inline void assert(bool condition, const std::string message);

} // namespace logger

inline void standard_output(const char *message) {
    std::cout << message << std::endl;
}
inline void standard_output(int number, const char **messages) {
    for (int i = 0; i < number; i++) {
        std::cout << messages[i];
    }
    std::cout << std::endl;
}
inline void standard_output(const std::string message) {
    std::cout << message << std::endl;
}
inline void standard_error(const char *message) {
    std::cerr << message << std::endl;
}
inline void standard_error(int number, const char **messages) {
    for (int i = 0; i < number; i++) {
        std::cerr << messages[i];
    }
    std::cerr << std::endl;
}
inline void standard_error(const std::string message) {
    std::cerr << message << std::endl;
}

inline void logger::log(const char *message) {
    standard_output(message);
}
inline void logger::log(int number, const char **messages) {
    standard_output(number, messages);
}
inline void logger::log(const std::string message) {
    standard_output(message);
}

inline void logger::info(const char *message) {
    std::cout << "ℹ️ INFO: ";
    standard_output(message);
}
inline void logger::info(int number, const char **messages) {
    std::cout << "ℹ️ INFO: ";
    standard_output(number, messages);
}
inline void logger::info(const std::string message) {
    std::cout << "ℹ️ INFO: ";
    standard_output(message);
}

inline void logger::debug(const char *message) {
    std::cout << "🔍 DEBUG: ";
    standard_output(message);
}
inline void logger::debug(int number, const char **messages) {
    std::cout << "🔍 DEBUG: ";
    standard_output(number, messages);
}
inline void logger::debug(const std::string message) {
    std::cout << "🔍 DEBUG: ";
    standard_output(message);
}

inline void logger::warn(const char *message) {
    std::cerr << "⚠️ WARNING: ";
    standard_error(message);
}
inline void logger::warn(int number, const char **messages) {
    std::cerr << "⚠️ WARNING: ";
    standard_error(number, messages);
}
inline void logger::warn(const std::string message) {
    std::cerr << "⚠️ WARNING: ";
    standard_error(message);
}

inline void logger::error(const char *message) {
    std::cerr << "❌ ERROR: ";
    standard_error(message);
}
inline void logger::error(int number, const char **messages) {
    std::cerr << "❌ ERROR: ";
    standard_error(number, messages);
}
inline void logger::error(const std::string message) {
    std::cerr << "❌ ERROR: ";
    standard_error(message);
}

inline void logger::assert(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(message);
    }
}
inline void logger::assert(bool condition, int number, const char **messages) {
    if (!condition) {
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(number, messages);
    }
}
inline void logger::assert(bool condition, const std::string message) {
    if (!condition) {
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(message);
    }
}

#endif // LOGGER_H
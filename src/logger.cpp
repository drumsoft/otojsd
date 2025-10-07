#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include "logger.h"

void clear_levelmeter_if_needed();

void standard_output(const char *message) {
    std::cout << message << std::endl;
}
void standard_output(int number, const char **messages) {
    for (int i = 0; i < number; i++) {
        std::cout << messages[i];
    }
    std::cout << std::endl;
}
void standard_output(const std::string message) {
    std::cout << message << std::endl;
}
void standard_error(const char *message) {
    std::cerr << message << std::endl;
}
void standard_error(int number, const char **messages) {
    for (int i = 0; i < number; i++) {
        std::cerr << messages[i];
    }
    std::cerr << std::endl;
}
void standard_error(const std::string message) {
    std::cerr << message << std::endl;
}

void logger::log(const char *message) {
    clear_levelmeter_if_needed();
    standard_output(message);
}
void logger::log(int number, const char **messages) {
    clear_levelmeter_if_needed();
    standard_output(number, messages);
}
void logger::log(const std::string message) {
    clear_levelmeter_if_needed();
    standard_output(message);
}

void logger::info(const char *message) {
    clear_levelmeter_if_needed();
    std::cout << "ℹ️ INFO: ";
    standard_output(message);
}
void logger::info(int number, const char **messages) {
    clear_levelmeter_if_needed();
    std::cout << "ℹ️ INFO: ";
    standard_output(number, messages);
}
void logger::info(const std::string message) {
    clear_levelmeter_if_needed();
    std::cout << "ℹ️ INFO: ";
    standard_output(message);
}

void logger::debug(const char *message) {
    clear_levelmeter_if_needed();
    std::cout << "🔍 DEBUG: ";
    standard_output(message);
}
void logger::debug(int number, const char **messages) {
    clear_levelmeter_if_needed();
    std::cout << "🔍 DEBUG: ";
    standard_output(number, messages);
}
void logger::debug(const std::string message) {
    clear_levelmeter_if_needed();
    std::cout << "🔍 DEBUG: ";
    standard_output(message);
}

void logger::warn(const char *message) {
    clear_levelmeter_if_needed();
    std::cerr << "⚠️ WARNING: ";
    standard_error(message);
}
void logger::warn(int number, const char **messages) {
    clear_levelmeter_if_needed();
    std::cerr << "⚠️ WARNING: ";
    standard_error(number, messages);
}
void logger::warn(const std::string message) {
    clear_levelmeter_if_needed();
    std::cerr << "⚠️ WARNING: ";
    standard_error(message);
}

void logger::error(const char *message) {
    clear_levelmeter_if_needed();
    std::cerr << "❌ ERROR: ";
    standard_error(message);
}
void logger::error(int number, const char **messages) {
    clear_levelmeter_if_needed();
    std::cerr << "❌ ERROR: ";
    standard_error(number, messages);
}
void logger::error(const std::string message) {
    clear_levelmeter_if_needed();
    std::cerr << "❌ ERROR: ";
    standard_error(message);
}

void logger::assert(bool condition, const char *message) {
    if (!condition) {
        clear_levelmeter_if_needed();
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(message);
    }
}
void logger::assert(bool condition, int number, const char **messages) {
    if (!condition) {
        clear_levelmeter_if_needed();
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(number, messages);
    }
}
void logger::assert(bool condition, const std::string message) {
    if (!condition) {
        clear_levelmeter_if_needed();
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(message);
    }
}

// Levelmeter implementation

static const std::string COLOR_GREEN = "\033[32m";
static const std::string COLOR_YELLOW = "\033[33m";
static const std::string COLOR_RED = "\033[31m";
static const std::string COLOR_RESET = "\033[0m";
static const float DB_MINIMUM = std::numeric_limits<float>::lowest();
static bool levelmeter_displayed = false;
static int screen_width = 0;
static int meter_width = 0;
static int meter_width_green = 0;
static int meter_width_yellow = 0;
static float last_level = DB_MINIMUM;
static auto last_level_update = std::chrono::high_resolution_clock::now();
static int last_level_phase = 0; // 0: fall, 1: hold

void clear_levelmeter_if_needed() {
    if (levelmeter_displayed) {
        std::cout << "\033[0K" << std::flush;
        levelmeter_displayed = false;
    }
}

void update_terminal_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int width = w.ws_col > 0 ? w.ws_col : 40;
    if (width == screen_width) {
        return;
    }
    screen_width = width;
    meter_width = std::clamp<int>(screen_width - 12, 0, 66);
    if (meter_width >= 3) {
        meter_width_green = std::round(meter_width * 54.0f / 66.0f) - 1;
        meter_width_yellow = std::round(meter_width * 60.0f / 66.0f) - 1;
    } else {
        meter_width_green = 0;
        meter_width_yellow = 0;
    }
}

void print_meter_line(float level) {
    std::string meter;
    meter.reserve(meter_width + 50);
    float db_level = level <= 0.0 ? DB_MINIMUM : 20.0 * std::log10(level);
    if (db_level >= last_level) {
        last_level = db_level;
        last_level_phase = 1;
        last_level_update = std::chrono::high_resolution_clock::now();
    } else {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_level_update).count();
        switch (last_level_phase) {
        case 0:
            if (last_level < -99.9) {
                last_level = db_level;
            } else {
                last_level -= elapsed * 60 / 1000.0f;
            }
            last_level_update = std::chrono::high_resolution_clock::now();
            break;
        case 1:
            if (elapsed >= 1000) {
                last_level_phase = 0;
                last_level_update = std::chrono::high_resolution_clock::now();
            }
            break;
        }
    }
    if (meter_width >= 3) {
        int current = std::clamp<int>(std::round(meter_width * (db_level + 60.0f) / 66.0f), 0, meter_width);
        int last = std::clamp<int>(std::round(meter_width * (last_level + 60.0f) / 66.0f), 0, meter_width);
        std::string temp = std::string(current, '*') + std::string(meter_width - current, ' ');
        if (current <= meter_width_yellow) {
            temp.replace(meter_width_yellow, 1, "|");
        }
        if (last > 0) {
            temp.replace(last - 1, 1, ">");
        }
        meter = "[" + COLOR_GREEN + temp.substr(0, meter_width_green) + COLOR_YELLOW + temp.substr(meter_width_green, meter_width_yellow - meter_width_green) + COLOR_RED + temp.substr(meter_width_yellow) + COLOR_RESET + "]";
    }
    if (screen_width > 6) {
        if (last_level == DB_MINIMUM) {
            meter += COLOR_GREEN + " -Inf ";
        } else if (last_level < -99.9f) {
            meter += COLOR_GREEN + "<-99.9";
        } else if (last_level <= 99.9f) {
            if (last_level < -6.0f) {
                meter += COLOR_GREEN;
            } else if (last_level < 0.0f) {
                meter += COLOR_YELLOW;
            } else {
                meter += COLOR_RED;
            }
            meter += std::format(" {:5.1f}", last_level);
        } else {
            meter += COLOR_RED + " >99.9";
        }
        meter += COLOR_RESET;
    }
    meter += "\033[0K\r";
    std::cout << meter << std::flush;
}

void logger::levelmeter(float level) {
    update_terminal_width();
    print_meter_line(level);
    levelmeter_displayed = true;
}

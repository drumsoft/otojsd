#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include "logger.h"
#include "const.h"

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

void logger::verify(bool condition, const char *message) {
    if (!condition) {
        clear_levelmeter_if_needed();
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(message);
    }
}
void logger::verify(bool condition, int number, const char **messages) {
    if (!condition) {
        clear_levelmeter_if_needed();
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(number, messages);
    }
}
void logger::verify(bool condition, const std::string message) {
    if (!condition) {
        clear_levelmeter_if_needed();
        std::cerr << "❌ ASSERTION FAILED: ";
        standard_error(message);
    }
}

// screen status

static bool levelmeter_displayed = false;
static bool analyzer_displayed = false;

void clear_levelmeter_if_needed() {
    if (levelmeter_displayed) {
        std::cout << "\033[0K" << std::flush;
        levelmeter_displayed = false;
    }
    if (analyzer_displayed) {
        std::cout << "\033[0K" << std::flush;
        analyzer_displayed = false;
    }
}

// Levelmeter implementation

static const float DB_MINIMUM = std::numeric_limits<float>::lowest();
static int screen_width = 0;
static int meter_width = 0;
static int meter_width_green = 0;
static int meter_width_yellow = 0;
static float last_level = DB_MINIMUM;
static auto last_level_update = std::chrono::high_resolution_clock::now();
static int last_level_phase = 0; // 0: fall, 1: hold

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
        meter += "[";
        meter += CHAR_COLOR_GREEN;
        meter += temp.substr(0, meter_width_green);
        meter += CHAR_COLOR_YELLOW;
        meter += temp.substr(meter_width_green, meter_width_yellow - meter_width_green);
        meter += CHAR_COLOR_RED;
        meter += temp.substr(meter_width_yellow);
        meter += CHAR_COLOR_RESET;
        meter += "]";
    }
    if (screen_width > 6) {
        if (last_level == DB_MINIMUM) {
            meter += CHAR_COLOR_GREEN;
            meter += " -Inf ";
        } else if (last_level < -99.9f) {
            meter += CHAR_COLOR_GREEN;
            meter += "<-99.9";
        } else if (last_level <= 99.9f) {
            if (last_level < -6.0f) {
                meter += CHAR_COLOR_GREEN;
            } else if (last_level < 0.0f) {
                meter += CHAR_COLOR_YELLOW;
            } else {
                meter += CHAR_COLOR_RED;
            }
            meter += std::format(" {:5.1f}", last_level);
        } else {
            meter += CHAR_COLOR_RED;
            meter += " >99.9";
        }
        meter += CHAR_COLOR_RESET;
    }
    meter += "\033[0K\r";
    std::cout << meter << std::flush;
}

void logger::levelmeter(float level) {
    update_terminal_width();
    print_meter_line(level);
    levelmeter_displayed = true;
}

// Dump implementation

void logger::dump(std::string_view message, size_t length, const unsigned char *data) {
    size_t dump_size = length * 3 + 1;
    char *dumped = new char[dump_size];
    for (size_t i = 0; i < length; ++i) {
        snprintf(dumped + i * 3, dump_size, "%02X ", data[i]);
        dump_size -= 3;
    }
    dumped[length * 3 - 1] = '\0';

    clear_levelmeter_if_needed();
    std::cout << message << "(" << length << "): " << dumped << std::endl;
    delete[] dumped;
}

// Analyzer output implementation

void logger::analyzer_output(SpectrumAnalyzer *sa) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int screen_width = w.ws_col > 0 ? w.ws_col : 80;
    int screen_height = w.ws_row > 0 ? w.ws_row : 25;

    const std::string *display = sa->render_display(screen_width - 4, screen_height / 2);
    std::cout << *display << std::flush;

    analyzer_displayed = true;
}

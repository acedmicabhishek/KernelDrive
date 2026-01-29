#pragma once

#include <vector>
#include <string>
#include <optional>

class LogitechPeripherals {
public:
    static bool set_dpi(int dpi);
    static int get_dpi();
    static bool set_polling_rate(int rate_hz);
    static int get_polling_rate();
    static bool is_supported();
};

#pragma once
#include <string>
#include <array>

namespace AsusKeyboard {
    
    struct Color { int r; int g; int b; };

    void init();
    int get_brightness();
    void set_brightness(int val);
    int get_max_brightness();
    int get_rgb_mode();
    void set_rgb_mode(int mode);
    Color get_color();
    void set_color(int r, int g, int b);

    bool is_supported();
    bool has_rgb();
}

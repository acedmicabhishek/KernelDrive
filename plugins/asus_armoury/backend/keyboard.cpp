#include "keyboard.h"
#include "../../../src/core/sysfs_writer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace AsusKeyboard {
    namespace fs = std::filesystem;

    static const std::string BASE_PATH = "/sys/class/leds/asus::kbd_backlight";
    static int g_max_brightness = 3; 
    static Color g_current_color = {255, 0, 0};

    void init() {
        if (fs::exists(BASE_PATH + "/max_brightness")) {
             auto val = SysfsWriter::read(BASE_PATH + "/max_brightness");
             if (val) {
                 try { g_max_brightness = std::stoi(*val); } catch(...) {}
             }
        }
    }

    int get_brightness() {
        auto val = SysfsWriter::read(BASE_PATH + "/brightness");
        if (val) {
            try { return std::stoi(*val); } catch(...) {}
        }
        return 0;
    }

    void set_brightness(int val) {
        if (val < 0) val = 0;
        if (val > g_max_brightness) val = g_max_brightness;
        SysfsWriter::write(BASE_PATH + "/brightness", std::to_string(val));
    }

    int get_max_brightness() {
        return g_max_brightness;
    }

    int get_rgb_mode() {
        auto val = SysfsWriter::read(BASE_PATH + "/kbd_rgb_mode");
        if (val) {
            try { return std::stoi(*val); } catch(...) {}
        }
        return 0;
    }

    void set_rgb_mode(int mode) {
        SysfsWriter::write(BASE_PATH + "/kbd_rgb_mode", std::to_string(mode));
    }

    Color get_color() {
        return g_current_color;
    }

    void set_color(int r, int g, int b) {
        g_current_color = {r, g, b};
        std::cout << "[AsusKeyboard] RGB Color setting is currently disabled/stubbed." << std::endl;
    }

    bool is_supported() {
        return fs::exists(BASE_PATH);
    }

    bool has_rgb() {
        return fs::exists(BASE_PATH + "/kbd_rgb_mode");
    }
}

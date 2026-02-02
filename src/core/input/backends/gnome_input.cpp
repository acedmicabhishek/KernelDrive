#include "gnome_input.h"
#include <cstdlib>
#include <string>
#include <array>
#include <memory>
#include <iostream>


static void gsettings_set(const std::string& schema, const std::string& key, const std::string& val) {
    std::string cmd = "gsettings set " + schema + " " + key + " " + val;
    system(cmd.c_str());
}

bool GnomeInputBackend::is_available() {
    const char* xdg = std::getenv("XDG_CURRENT_DESKTOP");
    return (xdg && (std::string(xdg).find("GNOME") != std::string::npos));
}

void GnomeInputBackend::set_touchpad_natural_scroll(bool enabled) {
    gsettings_set("org.gnome.desktop.peripherals.touchpad", "natural-scroll", enabled ? "true" : "false");
    gsettings_set("org.gnome.desktop.peripherals.mouse", "natural-scroll", enabled ? "true" : "false");
}

void GnomeInputBackend::set_touchpad_tap_to_click(bool enabled) {
    gsettings_set("org.gnome.desktop.peripherals.touchpad", "tap-to-click", enabled ? "true" : "false");
}

void GnomeInputBackend::set_pointer_speed(double speed) {
    
    gsettings_set("org.gnome.desktop.peripherals.mouse", "speed", std::to_string(speed));
    gsettings_set("org.gnome.desktop.peripherals.touchpad", "speed", std::to_string(speed));
}

void GnomeInputBackend::set_mouse_accel_profile(const std::string& profile) {
    
    gsettings_set("org.gnome.desktop.peripherals.mouse", "accel-profile", "'" + profile + "'");
}

void GnomeInputBackend::set_keyboard_repeat_info(int rate, int delay) {
    
    if (rate <= 0) rate = 1;
    int interval = 1000 / rate;
    
    gsettings_set("org.gnome.desktop.peripherals.keyboard", "repeat", "true");
    gsettings_set("org.gnome.desktop.peripherals.keyboard", "delay", std::to_string(delay));
    gsettings_set("org.gnome.desktop.peripherals.keyboard", "repeat-interval", std::to_string(interval));
}

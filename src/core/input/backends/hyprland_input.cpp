#include "hyprland_input.h"
#include <array>
#include <memory>
#include <cstdio>
#include <cstdlib>

std::string HyprlandInputBackend::exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool HyprlandInputBackend::is_available() {
    return std::getenv("HYPRLAND_INSTANCE_SIGNATURE") != nullptr;
}

void HyprlandInputBackend::set_touchpad_natural_scroll(bool enabled) {
    std::string cmd = "hyprctl keyword input:touchpad:natural_scroll " + std::string(enabled ? "true" : "false");
    exec(cmd.c_str());
}

void HyprlandInputBackend::set_touchpad_tap_to_click(bool enabled) {
    std::string cmd = "hyprctl keyword input:touchpad:tap-to-click " + std::string(enabled ? "true" : "false");
    exec(cmd.c_str());
}

void HyprlandInputBackend::set_pointer_speed(double speed) {
    
    std::string cmd = "hyprctl keyword input:sensitivity " + std::to_string(speed);
    exec(cmd.c_str());
}

void HyprlandInputBackend::set_mouse_accel_profile(const std::string& profile) {
    
    std::string cmd = "hyprctl keyword input:accel_profile " + profile;
    exec(cmd.c_str());
}

void HyprlandInputBackend::set_keyboard_repeat_info(int rate, int delay) {
    std::string cmd1 = "hyprctl keyword input:repeat_rate " + std::to_string(rate);
    std::string cmd2 = "hyprctl keyword input:repeat_delay " + std::to_string(delay);
    exec(cmd1.c_str());
    exec(cmd2.c_str());
}

#include "x11_input.h"
#include <cstdlib>
#include <string>
#include <array>
#include <memory>
#include <iostream>
#include <vector>
#include <sstream>
#include <X11/Xlib.h>


static std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static std::vector<std::string> get_pointer_ids() {
    
    std::vector<std::string> ids;
    std::string out = exec("xinput list --id-only");
    std::stringstream ss(out);
    std::string line;
    while(std::getline(ss, line)) {
        if(!line.empty()) ids.push_back(line);
    }
    return ids;
}


static void xinput_set_prop(const std::string& prop, const std::string& val) {
    
    
    std::string cmd = "for id in $(xinput list --id-only); do xinput set-prop $id \"" + prop + "\" " + val + " 2>/dev/null; done";
    system(cmd.c_str());
}

bool X11InputBackend::is_available() {
    const char* session = std::getenv("XDG_SESSION_TYPE");
    
     Display* d = XOpenDisplay(NULL);
    if (d) {
        XCloseDisplay(d);
        return true;
    }
    return (session && std::string(session) == "x11");
}

void X11InputBackend::set_touchpad_natural_scroll(bool enabled) {
    
    xinput_set_prop("libinput Natural Scrolling Enabled", enabled ? "1" : "0");
}

void X11InputBackend::set_touchpad_tap_to_click(bool enabled) {
    
    xinput_set_prop("libinput Tapping Enabled", enabled ? "1" : "0");
}

void X11InputBackend::set_pointer_speed(double speed) {
    
    xinput_set_prop("libinput Accel Speed", std::to_string(speed));
}

void X11InputBackend::set_mouse_accel_profile(const std::string& profile) {
    
    
    
    if (profile == "flat") {
        xinput_set_prop("libinput Accel Profile Enabled", "0, 1");
    } else {
        xinput_set_prop("libinput Accel Profile Enabled", "1, 0");
    }
}

void X11InputBackend::set_keyboard_repeat_info(int rate, int delay) {
    
    std::string cmd = "xset r rate " + std::to_string(delay) + " " + std::to_string(rate);
    exec(cmd.c_str());
}

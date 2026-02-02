#include "input_manager.h"
#include "backends/hyprland_input.h"
#include "backends/gnome_input.h"
#include "backends/x11_input.h"
#include <iostream>

class FallbackInputBackend : public IInputBackend {
public:
    bool is_available() override { return true; }
    void set_touchpad_natural_scroll(bool) override {}
    void set_touchpad_tap_to_click(bool) override {}
    void set_pointer_speed(double) override {}
    void set_mouse_accel_profile(const std::string&) override {}
    void set_keyboard_repeat_info(int, int) override {}
};

InputManager::InputManager() {
    
    auto hypr = std::make_shared<HyprlandInputBackend>();
    if (hypr->is_available()) {
        std::cout << "InputManager: Detected Hyprland" << std::endl;
        backend = hypr;
        return;
    }
    
    
    auto gnome = std::make_shared<GnomeInputBackend>();
    if (gnome->is_available()) {
        std::cout << "InputManager: Detected GNOME" << std::endl;
        backend = gnome;
        return;
    }
    
    
    auto x11 = std::make_shared<X11InputBackend>();
    if (x11->is_available()) {
        std::cout << "InputManager: Detected X11" << std::endl;
        backend = x11;
        return;
    }

    std::cout << "InputManager: Using Fallback" << std::endl;
    backend = std::make_shared<FallbackInputBackend>();
    
    apply_stored_settings();
}

InputManager& InputManager::get() {
    static InputManager instance;
    return instance;
}

std::shared_ptr<IInputBackend> InputManager::get_backend() {
    return backend;
}


#include "../config_manager.h"

void InputManager::apply_stored_settings() {
    auto& cfg = ConfigManager::get();
    
    
    bool natural = cfg.get_bool("Input", "natural_scroll", false);
    bool tap = cfg.get_bool("Input", "tap_to_click", true);
    double speed = cfg.get_double("Input", "pointer_speed", 0.0);
    int rate = cfg.get_int("Input", "kb_rate", 25);
    int delay = cfg.get_int("Input", "kb_delay", 600);
    
    if (backend) {
        backend->set_touchpad_natural_scroll(natural);
        backend->set_touchpad_tap_to_click(tap);
        backend->set_pointer_speed(speed);
        backend->set_keyboard_repeat_info(rate, delay);
    }
}

void InputManager::set_touchpad_natural_scroll(bool enabled) {
    if(backend) backend->set_touchpad_natural_scroll(enabled);
    ConfigManager::get().set_bool("Input", "natural_scroll", enabled);
}

void InputManager::set_touchpad_tap_to_click(bool enabled) {
    if(backend) backend->set_touchpad_tap_to_click(enabled);
    ConfigManager::get().set_bool("Input", "tap_to_click", enabled);
}

void InputManager::set_pointer_speed(double speed) {
    if(backend) backend->set_pointer_speed(speed);
    ConfigManager::get().set_double("Input", "pointer_speed", speed);
}

void InputManager::set_keyboard_repeat_info(int rate, int delay) {
    if(backend) backend->set_keyboard_repeat_info(rate, delay);
    ConfigManager::get().set_int("Input", "kb_rate", rate);
    ConfigManager::get().set_int("Input", "kb_delay", delay);
}

bool InputManager::get_touchpad_natural_scroll() {
    return ConfigManager::get().get_bool("Input", "natural_scroll", false);
}
bool InputManager::get_touchpad_tap_to_click() {
    return ConfigManager::get().get_bool("Input", "tap_to_click", true);
}
double InputManager::get_pointer_speed() {
    return ConfigManager::get().get_double("Input", "pointer_speed", 0.0);
}
int InputManager::get_keyboard_rate() {
    return ConfigManager::get().get_int("Input", "kb_rate", 25);
}
int InputManager::get_keyboard_delay() {
    return ConfigManager::get().get_int("Input", "kb_delay", 600);
}

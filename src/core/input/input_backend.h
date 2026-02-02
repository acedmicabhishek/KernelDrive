#pragma once
#include <string>

class IInputBackend {
public:
    virtual ~IInputBackend() = default;
    
    virtual bool is_available() = 0;
    
    // Touchpad
    virtual void set_touchpad_natural_scroll(bool enabled) = 0;
    virtual void set_touchpad_tap_to_click(bool enabled) = 0;
    virtual void set_pointer_speed(double speed) = 0; // -1.0 to 1.0 (0.0 is default)
    
    // Mouse
    virtual void set_mouse_accel_profile(const std::string& profile) = 0; // "flat", "adaptive"
    
    // Keyboard
    virtual void set_keyboard_repeat_info(int rate, int delay) = 0; // rate (cps), delay (ms)
    virtual bool get_touchpad_natural_scroll() { return false; }
    virtual bool get_touchpad_tap_to_click() { return true; }
    virtual double get_pointer_speed() { return 0.0; }
};

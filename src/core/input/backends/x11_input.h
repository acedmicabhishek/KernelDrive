#pragma once
#include "../input_backend.h"

class X11InputBackend : public IInputBackend {
public:
    bool is_available() override;
    
    void set_touchpad_natural_scroll(bool enabled) override;
    void set_touchpad_tap_to_click(bool enabled) override;
    void set_pointer_speed(double speed) override;
    void set_mouse_accel_profile(const std::string& profile) override;
    void set_keyboard_repeat_info(int rate, int delay) override;
};

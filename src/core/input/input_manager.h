#pragma once
#include "input_backend.h"
#include <memory>

class InputManager {
public:
    static InputManager& get();
    
    
    void set_touchpad_natural_scroll(bool enabled);
    void set_touchpad_tap_to_click(bool enabled);
    void set_pointer_speed(double speed);
    void set_keyboard_repeat_info(int rate, int delay);
    
    
    bool get_touchpad_natural_scroll();
    bool get_touchpad_tap_to_click();
    double get_pointer_speed();
    int get_keyboard_rate();
    int get_keyboard_delay();
    
    std::shared_ptr<IInputBackend> get_backend();
    
private:
    InputManager();
    void apply_stored_settings(); 
    std::shared_ptr<IInputBackend> backend;
};

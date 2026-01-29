// dummy file for now , some code for simulation to test the ui


#include "logitech_peripherals.h"
#include <iostream>
static int current_dpi = 1600;
static int current_rate = 1000;

bool LogitechPeripherals::is_supported() {
    return true; 
}

bool LogitechPeripherals::set_dpi(int dpi) {
    std::cout << "[Logitech Plugin] Setting DPI to " << dpi << std::endl;
    current_dpi = dpi;
    return true;
}

int LogitechPeripherals::get_dpi() {
    return current_dpi;
}

bool LogitechPeripherals::set_polling_rate(int rate_hz) {
     std::cout << "[Logitech Plugin] Setting Polling Rate to " << rate_hz << "Hz" << std::endl;
     current_rate = rate_hz;
     return true;
}

int LogitechPeripherals::get_polling_rate() {
    return current_rate;
}

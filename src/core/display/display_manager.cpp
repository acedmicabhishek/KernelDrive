#include "display_manager.h"
#include "backends/hyprland_backend.h"
#include "backends/gnome_backend.h"
#include "backends/x11_backend.h"
#include <iostream>

class FallbackBackend : public IDisplayBackend {
    bool is_available() override { return true; }
    std::vector<DisplayInfo> get_displays() override { return {}; }
    bool set_mode(const std::string&, const DisplayMode&) override { return false; }
    bool set_scale(const std::string&, double) override { return false; }
};

DisplayManager::DisplayManager() {
    
    auto hypr = std::make_shared<HyprlandBackend>();
    if (hypr->is_available()) {
        std::cout << "DisplayManager: Detected Hyprland" << std::endl;
        backend = hypr;
        return;
    }

    
    auto gnome = std::make_shared<GnomeBackend>();
    if (gnome->is_available()) {
         std::cout << "DisplayManager: Detected GNOME" << std::endl;
         backend = gnome;
         return;
    }

    
    auto x11 = std::make_shared<X11Backend>();
    if (x11->is_available()) {
         std::cout << "DisplayManager: Detected X11" << std::endl;
         backend = x11;
         return;
    }

    std::cout << "DisplayManager: Using Fallback" << std::endl;
    backend = std::make_shared<FallbackBackend>();
}

DisplayManager& DisplayManager::get() {
    static DisplayManager instance;
    return instance;
}

std::shared_ptr<IDisplayBackend> DisplayManager::get_backend() {
    return backend;
}

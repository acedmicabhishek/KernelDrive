#pragma once
#include "../display_backend.h"
#include <vector>
#include <string>

class X11Backend : public IDisplayBackend {
public:
    bool is_available() override;
    std::vector<DisplayInfo> get_displays() override;
    bool set_mode(const std::string& display_id, const DisplayMode& mode) override;
    bool set_scale(const std::string& display_id, double scale) override;
};

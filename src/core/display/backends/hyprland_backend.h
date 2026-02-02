#pragma once
#include "../display_backend.h"
#include <string>
#include <vector>

class HyprlandBackend : public IDisplayBackend {
public:
    bool is_available() override;
    std::vector<DisplayInfo> get_displays() override;
    bool set_mode(const std::string& display_id, const DisplayMode& mode) override;
    bool set_scale(const std::string& display_id, double scale) override;

private:
    std::string exec(const char* cmd);
};

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <optional>

struct DisplayMode {
    int width;
    int height;
    int refresh_rate_mHz;
    
    std::string to_string() const {
        return std::to_string(width) + "x" + std::to_string(height) + " @ " + std::to_string(refresh_rate_mHz / 1000.0) + "Hz";
    }
    
    bool operator==(const DisplayMode& other) const {
        return width == other.width && height == other.height && refresh_rate_mHz == other.refresh_rate_mHz;
    }
};

struct DisplayInfo {
    std::string id;
    std::string model;
    std::string make;
    
    DisplayMode current_mode;
    double scale;
    
    std::vector<DisplayMode> available_modes;
    
    bool primary;
    bool enabled;
};

class IDisplayBackend {
public:
    virtual ~IDisplayBackend() = default;
    
    virtual bool is_available() = 0;
    
    virtual std::vector<DisplayInfo> get_displays() = 0;
    
    virtual bool set_mode(const std::string& display_id, const DisplayMode& mode) = 0;
    virtual bool set_scale(const std::string& display_id, double scale) = 0;
};

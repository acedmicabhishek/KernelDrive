#pragma once
#include "hidpp_driver.h"
#include <map>
#include <string>
#include <memory>
#include <vector>

class Hidpp20Device {
public:
    struct FeatureInfo {
        uint8_t index;
        uint8_t type;
        uint8_t version;
    };

    Hidpp20Device(const std::string& path, const std::string& name);
    ~Hidpp20Device();

    bool connect();
    void disconnect();
    bool is_connected() const;

    uint8_t get_feature_index(uint16_t feature_id);
    
    bool set_dpi(int dpi);
    bool set_polling_rate(int ms);
    
    std::string get_path() const { return m_path; }
    std::string get_name() const { return m_name; }

private:
    std::string m_path;
    std::string m_name;
    HidppDriver m_driver;
    
    std::map<uint16_t, FeatureInfo> m_feature_cache;
    bool root_get_feature(uint16_t feature_id, uint8_t &index, uint8_t &type, uint8_t &ver);
};

#include "hidpp20_device.h"
#include "hidpp_constants.h"
#include <iostream>

Hidpp20Device::Hidpp20Device(const std::string& path, const std::string& name) 
    : m_path(path), m_name(name) {}

Hidpp20Device::~Hidpp20Device() {
    disconnect();
}

bool Hidpp20Device::connect() {
    if (m_driver.is_open()) return true;
    
    if (!m_driver.open_path(m_path.c_str())) {
        return false;
    }
    
    m_feature_cache[HIDPP_PAGE_ROOT] = {0, 0, 0};
    
    return true;
}

void Hidpp20Device::disconnect() {
    m_driver.close();
    m_feature_cache.clear();
}

bool Hidpp20Device::is_connected() const {
    return m_driver.is_open();
}

bool Hidpp20Device::root_get_feature(uint16_t feature_id, uint8_t &index, uint8_t &type, uint8_t &ver) {
    if (!is_connected()) return false;

    auto log_hex = [](const std::string& prefix, const std::vector<unsigned char>& data) {
        std::cout << prefix;
        for(auto b : data) printf("%02X ", b);
        std::cout << std::endl;
    };

    {
        std::vector<unsigned char> cmd(7, 0x00);
        cmd[0] = 0x10;
        cmd[1] = 0xFF; // Device Index (broadcast)
        cmd[2] = 0x00; // Root Feature Index
        cmd[3] = CMD_ROOT_GET_FEATURE; // 0x00
        cmd[4] = (feature_id >> 8) & 0xFF;
        cmd[5] = (feature_id & 0xFF);
        
        // log_hex("[Tx Short] ", cmd);
        auto resp = m_driver.send_recv(cmd);
        // log_hex("[Rx Short] ", resp);
        
        if (!resp.empty() && resp[0] == 0x10 && resp[2] == 0x00 && resp[3] == CMD_ROOT_GET_FEATURE) {
            index = resp[4];
            type = resp[5];
            ver = resp[6];
            return (index != 0);
        }
    }

    //Long Report (0x11) - Some devices ignore Short
    {
        std::vector<unsigned char> cmd(20, 0x00);
        cmd[0] = 0x11;
        cmd[1] = 0xFF;
        cmd[2] = 0x00;
        cmd[3] = CMD_ROOT_GET_FEATURE;
        cmd[4] = (feature_id >> 8) & 0xFF;
        cmd[5] = (feature_id & 0xFF);
        
        // log_hex("[Tx Long]  ", cmd);
        auto resp = m_driver.send_recv(cmd);
        // log_hex("[Rx Long]  ", resp);

        if (!resp.empty() && resp[0] == 0x11 && resp[2] == 0x00 && resp[3] == CMD_ROOT_GET_FEATURE) {
            index = resp[4];
            type = resp[5];
            ver = resp[6];
            return (index != 0);
        }
    }
    
    std::cout << "[Hidpp20] root_get_feature failed for ID: " << std::hex << feature_id << std::endl;
    return false;
}

uint8_t Hidpp20Device::get_feature_index(uint16_t feature_id) {
    if (m_feature_cache.find(feature_id) != m_feature_cache.end()) {
        return m_feature_cache[feature_id].index;
    }
    
    uint8_t idx, type, ver;
    if (root_get_feature(feature_id, idx, type, ver)) {
        m_feature_cache[feature_id] = {idx, type, ver};
        return idx;
    }
    
    return 0;
}

int Hidpp20Device::get_max_dpi() {
    if (!is_connected()) connect();
    
    uint8_t feat_idx = get_feature_index(HIDPP_PAGE_ADJUSTABLE_DPI);
    if (feat_idx == 0) return 0;

    // GetDpiList (Func 1 = 0x10)
    std::vector<unsigned char> cmd(7, 0x00);
    cmd[0] = 0x10;
    cmd[1] = 0xFF;
    cmd[2] = feat_idx;
    cmd[3] = CMD_ADJUSTABLE_DPI_GET_SENSOR_DPI_LIST;
    cmd[4] = 0x00;
    
    auto resp = m_driver.send_recv(cmd);
    // Response: [0x10][FF][Feat][Func][Index][Val1_MSB][Val1_LSB][Val2_MSB][Val2_LSB]...
    if (resp.size() >= 7 && resp[0] == 0x10 && resp[2] == feat_idx && resp[3] == CMD_ADJUSTABLE_DPI_GET_SENSOR_DPI_LIST) {
        // Each value is 2 bytes.
        int max_dpi = 0;
        for (size_t i = 5; i < resp.size() - 1; i += 2) {
            int val = (resp[i] << 8) | resp[i+1];
            if (val == 0) break;
            if (val > 0xE000) {
            } else {
                 if (val > max_dpi) max_dpi = val;
            }
        }
        return max_dpi;
    }
    
    return 0;
}

int Hidpp20Device::get_dpi() {
    if (!is_connected()) connect();
    
    uint8_t feat_idx = get_feature_index(HIDPP_PAGE_ADJUSTABLE_DPI);
    if (feat_idx == 0) return 0;

    // GetSensorDpi (Func 2 = 0x20)
    // Param 0: Sensor Index (0)
    std::vector<unsigned char> cmd(7, 0x00);
    cmd[0] = 0x10;
    cmd[1] = 0xFF;
    cmd[2] = feat_idx;
    cmd[3] = CMD_ADJUSTABLE_DPI_GET_SENSOR_DPI; // 0x20
    cmd[4] = 0x00; // Sensor 0
    
    auto resp = m_driver.send_recv(cmd);
    // Response: [0x10][FF][Feat][Func][SensorIdx][DPI_MSB][DPI_LSB][DefaultDPI_MSB][DefaultDPI_LSB]
    if (resp.size() >= 7 && resp[0] == 0x10 && resp[2] == feat_idx && resp[3] == CMD_ADJUSTABLE_DPI_GET_SENSOR_DPI) {
        int dpi = (resp[5] << 8) | resp[6];
        return dpi;
    }
    
    return 0;
}

int Hidpp20Device::get_polling_rate() {
    if (!is_connected()) connect();
    
    uint8_t feat_idx = get_feature_index(HIDPP_PAGE_ADJUSTABLE_REPORT_RATE);
    if (feat_idx == 0) return 0;

    // GetReportRate (Func 1 = 0x10)
    std::vector<unsigned char> cmd(7, 0x00);
    cmd[0] = 0x10;
    cmd[1] = 0xFF;
    cmd[2] = feat_idx;
    cmd[3] = CMD_ADJUSTABLE_REPORT_RATE_GET_REPORT_RATE; // 0x10
    
    auto resp = m_driver.send_recv(cmd);
    // Response: [0x10][FF][Feat][Func][Rate]
    if (resp.size() >= 5 && resp[0] == 0x10 && resp[2] == feat_idx && resp[3] == CMD_ADJUSTABLE_REPORT_RATE_GET_REPORT_RATE) {
        return resp[4];
    }
    
    return 0;
}

bool Hidpp20Device::set_dpi(int dpi) {
    if (!is_connected()) connect();
    
    uint8_t feat_idx = get_feature_index(HIDPP_PAGE_ADJUSTABLE_DPI); // 0x2201
    if (feat_idx == 0) {
        std::cerr << "[Hidpp20] DPI Feature not supported." << std::endl;
        return false;
    }

    // Command: SetSensorDpi (Func 3 = 0x30)
    std::vector<unsigned char> cmd(7, 0x00);
    cmd[0] = 0x10;
    cmd[1] = 0xFF;
    cmd[2] = feat_idx;
    cmd[3] = CMD_ADJUSTABLE_DPI_SET_SENSOR_DPI; // 0x30
    cmd[4] = 0x00; // Sensor 0
    cmd[5] = (dpi >> 8) & 0xFF;
    cmd[6] = (dpi & 0xFF);
    
    auto resp = m_driver.send_recv(cmd);
    return !resp.empty();
}

bool Hidpp20Device::set_polling_rate(int ms) {
    if (!is_connected()) connect();
    
    uint8_t feat_idx = get_feature_index(HIDPP_PAGE_ADJUSTABLE_REPORT_RATE); // 0x8060
    if (feat_idx == 0) {
         std::cerr << "[Hidpp20] Polling Rate Feature not supported." << std::endl;
         return false;
    }

    // Command: SetReportRate (Func 2 = 0x20)
    std::vector<unsigned char> cmd(7, 0x00);
    cmd[0] = 0x10;
    cmd[1] = 0xFF;
    cmd[2] = feat_idx;
    cmd[3] = CMD_ADJUSTABLE_REPORT_RATE_SET_REPORT_RATE; // 0x20
    cmd[4] = (uint8_t)ms;
    
    auto resp = m_driver.send_recv(cmd);
    return !resp.empty();
}

bool Hidpp20Device::set_led(int mode, int r, int g, int b, int period) {
    if (mode == 0) {
        mode = 1;
        r = 0; g = 0; b = 0;
    }

    if (!is_connected()) {
        std::cout << "[Hidpp20] Not connected, trying to connect..." << std::endl;
        connect();
    }
    
    uint8_t feat_idx = get_feature_index(HIDPP_PAGE_COLOR_LED_EFFECTS); // 0x8070
    bool is_8071 = false;
    
    std::cout << "[Hidpp20] Checking RGB Features..." << std::endl;
    if (feat_idx == 0) {
        std::cout << "[Hidpp20] 0x8070 not found. Checking 0x8071..." << std::endl;
        feat_idx = get_feature_index(HIDPP_PAGE_RGB_EFFECTS); // 0x8071
        if (feat_idx != 0) is_8071 = true;
    } else {
        std::cout << "[Hidpp20] Found 0x8070 at index " << (int)feat_idx << std::endl;
    }
    
    if (feat_idx == 0) {
        std::cerr << "[Hidpp20] RGB Feature 0x8070/0x8071 not found on this device." << std::endl;
        return false;
    }
    
    std::vector<unsigned char> cmd(20, 0x00);
    cmd[0] = 0x11;
    cmd[1] = 0xFF;
    cmd[2] = feat_idx;
    
    if (is_8071) {
        // 0x8071 Protocol: Func 1 (0x10) = Set Effect
        cmd[3] = 0x10; 
        // 8071 usually: [Zone][Mode][R][G][B][PeriodH][PeriodL]
        cmd[4] = 0x00;
        cmd[5] = (uint8_t)mode;
        cmd[6] = (uint8_t)r;
        cmd[7] = (uint8_t)g;
        cmd[8] = (uint8_t)b;
        if (mode == 2 || mode == 3) {
             cmd[9] = (period >> 8) & 0xFF;
             cmd[10] = (period & 0xFF);
        }
    } else {
        // 0x8070 Protocol: Func 3 (0x30) = Set Zone Effect
        cmd[3] = CMD_COLOR_LED_EFFECTS_SET_ZONE_EFFECT; // 0x30
        cmd[4] = 0x00;
        cmd[5] = (uint8_t)mode;
        cmd[6] = (uint8_t)r;
        cmd[7] = (uint8_t)g;
        cmd[8] = (uint8_t)b;
        if (mode == 2 || mode == 3) {
             cmd[9] = (period >> 8) & 0xFF;
             cmd[10] = (period & 0xFF);
        }
    }
    
    std::cout << "[Hidpp20] Sending LED Command. Is 8071? " << is_8071 << ". Mode: " << mode << std::endl;
    
    int max_zone = is_8071 ? 1 : 0;
    
    bool overall_success = false;
    for (int zone = 0; zone <= max_zone; zone++) {
        cmd[4] = (uint8_t)zone;
        auto resp = m_driver.send_recv(cmd);
        if (!resp.empty()) {
             std::cout << "[Hidpp20] Zone " << zone << " Success! Resp size: " << resp.size() << std::endl;
             overall_success = true;
        } else {
             std::cout << "[Hidpp20] Zone " << zone << " Failed/NoResp." << std::endl;
        }
    }
    
    return overall_success;
}

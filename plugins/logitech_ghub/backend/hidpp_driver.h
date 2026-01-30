#pragma once
#include <hidapi/hidapi.h>
#include <vector>
#include <string>
#include <optional>

class HidppDriver {
public:
    static bool init();
    static void shutdown();
    
    struct DeviceInfo {
        std::string path;
        unsigned short vendor_id;
        unsigned short product_id;
        std::wstring manufacturer;
        std::wstring product;
        int interface_number;
        unsigned short usage_page;
    };
    
    static std::vector<DeviceInfo> enumerate(unsigned short vendor_id = 0x046d); // 0x046d = Logitech

    HidppDriver();
    ~HidppDriver();

    bool open(unsigned short vendor_id, unsigned short product_id);
    bool open_path(const char* path);
    void close();
    bool is_open() const;

    std::vector<unsigned char> send_recv(const std::vector<unsigned char>& command);

private:
    hid_device* m_handle = nullptr;
};

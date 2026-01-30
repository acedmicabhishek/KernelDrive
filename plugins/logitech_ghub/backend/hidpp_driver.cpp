#include "hidpp_driver.h"
#include <iostream>
#include <cstring>

bool HidppDriver::init() {
    return hid_init() == 0;
}

void HidppDriver::shutdown() {
    hid_exit();
}

std::vector<HidppDriver::DeviceInfo> HidppDriver::enumerate(unsigned short vendor_id) {
    std::vector<DeviceInfo> devices;
    struct hid_device_info *devs, *cur_dev;
    
    devs = hid_enumerate(vendor_id, 0x0);
    cur_dev = devs; 
    while (cur_dev) {
        
        devices.push_back({
            cur_dev->path,
            cur_dev->vendor_id,
            cur_dev->product_id,
            cur_dev->manufacturer_string ? cur_dev->manufacturer_string : L"",
            cur_dev->product_string ? cur_dev->product_string : L"",
            cur_dev->interface_number,
            cur_dev->usage_page
        });
        cur_dev = cur_dev->next;
    }
    hid_free_enumeration(devs);
    return devices;
}

HidppDriver::HidppDriver() {}

HidppDriver::~HidppDriver() {
    close();
}

bool HidppDriver::open(unsigned short vendor_id, unsigned short product_id) {
    close();
    m_handle = hid_open(vendor_id, product_id, NULL);
    if (!m_handle) {
         return false;
    }
    return true;
}

bool HidppDriver::open_path(const char* path) {
    close();
    m_handle = hid_open_path(path);
    if (!m_handle) return false;
    return true;
}

void HidppDriver::close() {
    if (m_handle) {
        hid_close(m_handle);
        m_handle = nullptr;
    }
}

bool HidppDriver::is_open() const {
    return m_handle != nullptr;
}

std::vector<unsigned char> HidppDriver::send_recv(const std::vector<unsigned char>& command) {
    if (!m_handle || command.size() > 256) return {};

    int res = hid_write(m_handle, command.data(), command.size()); 
    if (res < 0) {
        std::wcerr << L"[HidppDriver] Write failed: " << (const wchar_t*)hid_error(m_handle) << std::endl;
        return {};
    }

    unsigned char buf[65];
    std::memset(buf, 0, sizeof(buf));
    res = hid_read(m_handle, buf, sizeof(buf));
    if (res < 0) {
        std::wcerr << L"[HidppDriver] Read failed: " << (const wchar_t*)hid_error(m_handle) << std::endl;
        return {};
    }
    
    std::vector<unsigned char> response;
    for(int i=0; i<res; i++) response.push_back(buf[i]);
    return response;
}

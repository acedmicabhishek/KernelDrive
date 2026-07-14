#pragma once
#include <string>

// Manages the systemd boot apply service (oneshot, re-applies the saved CPU
// governor / power profile) so KernelDrive's settings persist without the GUI.
// The application already runs as root, so this touches /etc/systemd/system
// and calls systemctl directly.
class ServiceManager {
public:
    static ServiceManager& get();

    // Boot apply service (Type=oneshot, "<exe> --apply").
    bool apply_is_enabled();
    bool apply_enable();
    bool apply_disable();

private:
    ServiceManager() = default;

    std::string executable_path() const;

    bool is_enabled(const char* unit) const;
    bool enable(const char* unit, const std::string& unit_text) const;
    bool disable(const char* unit) const;

    std::string apply_unit() const;
};

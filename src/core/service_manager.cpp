#include "service_manager.h"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <climits>
#include <unistd.h>

static constexpr const char* APPLY_UNIT = "kerneldrive.service";

ServiceManager& ServiceManager::get() {
    static ServiceManager instance;
    return instance;
}

std::string ServiceManager::executable_path() const {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n == -1) return "/usr/bin/kerneldrive";
    buf[n] = '\0';
    return std::string(buf);
}

std::string ServiceManager::apply_unit() const {
    return
        "[Unit]\n"
        "Description=KernelDrive - apply saved hardware settings on boot\n"
        "After=multi-user.target\n"
        "\n"
        "[Service]\n"
        "Type=oneshot\n"
        "ExecStart=" + executable_path() + " --apply\n"
        "RemainAfterExit=yes\n"
        "\n"
        "[Install]\n"
        "WantedBy=multi-user.target\n";
}

// ---- generic helpers --------------------------------------------------------

bool ServiceManager::is_enabled(const char* unit) const {
    std::string cmd = std::string("systemctl is-enabled --quiet ") + unit;
    return std::system(cmd.c_str()) == 0;
}

bool ServiceManager::enable(const char* unit, const std::string& unit_text) const {
    std::string path = std::string("/etc/systemd/system/") + unit;
    {
        std::ofstream f(path);
        if (!f.is_open()) {
            std::cerr << "[ServiceManager] cannot write " << path
                      << " (need root)" << std::endl;
            return false;
        }
        f << unit_text;
        if (!f.good()) return false;
    }

    if (std::system("systemctl daemon-reload") != 0) return false;
    std::string cmd = std::string("systemctl enable --now ") + unit;
    return std::system(cmd.c_str()) == 0;
}

bool ServiceManager::disable(const char* unit) const {
    std::string cmd = std::string("systemctl disable --now ") + unit;
    std::system(cmd.c_str()); // ignore; may already be disabled

    std::string path = std::string("/etc/systemd/system/") + unit;
    std::remove(path.c_str());
    std::system("systemctl daemon-reload");
    return true;
}

// ---- public -----------------------------------------------------------------

bool ServiceManager::apply_is_enabled()  { return is_enabled(APPLY_UNIT); }
bool ServiceManager::apply_enable()      { return enable(APPLY_UNIT, apply_unit()); }
bool ServiceManager::apply_disable()     { return disable(APPLY_UNIT); }

#include "dependency_manager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

DependencyManager& DependencyManager::get() {
    static DependencyManager instance;
    return instance;
}

DependencyManager::DependencyManager() {
    
    std::ifstream os_release("/etc/os-release");
    std::string line;
    while (std::getline(os_release, line)) {
        if (line.find("ID=") == 0) {
            distro_id = line.substr(3);
            if (!distro_id.empty() && distro_id.front() == '"') {
                distro_id = distro_id.substr(1, distro_id.size()-2);
            }
            break;
        }
    }
    
    init_dependencies();
}

void DependencyManager::init_dependencies() {
    
    dependencies.push_back({
        "ryzenadj",
        "AMD Ryzen power management tool (TDP limits, curve optimizer)",
        "which ryzenadj",
        "pkexec sh -c 'pacman -S --noconfirm git cmake base-devel pciutils && "
        "rm -rf /tmp/RyzenAdj && "
        "git clone https:
        "cd /tmp/RyzenAdj && mkdir build && cd build && "
        "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'",
        "pkexec sh -c 'apt install -y git cmake build-essential libpci-dev && "
        "rm -rf /tmp/RyzenAdj && "
        "git clone https:
        "cd /tmp/RyzenAdj && mkdir build && cd build && "
        "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'",
        "pkexec sh -c 'dnf install -y git cmake gcc-c++ pciutils-devel && "
        "rm -rf /tmp/RyzenAdj && "
        "git clone https:
        "cd /tmp/RyzenAdj && mkdir build && cd build && "
        "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'",
        false  
    });
    

}

std::vector<Dependency> DependencyManager::get_missing_dependencies() {
    std::vector<Dependency> missing;
    for (const auto& dep : dependencies) {
        if (!is_available(dep.name)) {
            missing.push_back(dep);
        }
    }
    return missing;
}

std::vector<Dependency> DependencyManager::get_all_dependencies() {
    return dependencies;
}

bool DependencyManager::is_available(const std::string& tool_name) {
    
    if (fs::exists("/usr/bin/" + tool_name)) return true;
    if (fs::exists("/usr/local/bin/" + tool_name)) return true;
    if (fs::exists("/bin/" + tool_name)) return true;
    
    
    std::string cmd = "which " + tool_name + " > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

std::string DependencyManager::get_install_command(const Dependency& dep) {
    if (distro_id == "arch" || distro_id == "manjaro" || distro_id == "endeavouros") {
        return dep.install_arch;
    } else if (distro_id == "ubuntu" || distro_id == "debian" || distro_id == "pop" || distro_id == "linuxmint") {
        return dep.install_debian;
    } else if (distro_id == "fedora" || distro_id == "rhel" || distro_id == "centos") {
        return dep.install_fedora;
    }
    return "";
}

std::string DependencyManager::get_distro_id() {
    return distro_id;
}

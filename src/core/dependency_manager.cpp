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
        "git clone https://github.com/FlyGoat/RyzenAdj.git /tmp/RyzenAdj && "
        "cd /tmp/RyzenAdj && mkdir build && cd build && "
        "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'",
        "pkexec sh -c 'apt install -y git cmake build-essential libpci-dev && "
        "rm -rf /tmp/RyzenAdj && "
        "git clone https://github.com/FlyGoat/RyzenAdj.git /tmp/RyzenAdj && "
        "cd /tmp/RyzenAdj && mkdir build && cd build && "
        "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'",
        "pkexec sh -c 'dnf install -y git cmake gcc-c++ pciutils-devel && "
        "rm -rf /tmp/RyzenAdj && "
        "git clone https://github.com/FlyGoat/RyzenAdj.git /tmp/RyzenAdj && "
        "cd /tmp/RyzenAdj && mkdir build && cd build && "
        "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'",
        false  
    });

    dependencies.push_back({
        "stress-ng",
        "CPU stress testing tool (Asus Plugin)",
        "which stress-ng",
        "pkexec pacman -S --noconfirm stress-ng",
        "pkexec apt install -y stress-ng",
        "pkexec dnf install -y stress-ng",
        false
    });

    dependencies.push_back({
        "lspci",
        "PCI utilities for hardware detection",
        "which lspci",
        "pkexec pacman -S --noconfirm pciutils",
        "pkexec apt install -y pciutils",
        "pkexec dnf install -y pciutils",
        true
    });

    dependencies.push_back({
        "gpu_burn",
        "CUDA GPU stress test tool (Requires NVIDIA CUDA)",
        "which gpu_burn",
        "pkexec sh -c 'echo \"This tool requires manual installation (CUDA). Arch: yay -S gpu-burn-git\"'",
        "pkexec sh -c 'echo \"This tool requires manual installation (CUDA). Debian/Ubuntu: Install nvidia-cuda-toolkit and build from source.\"'",
        "pkexec sh -c 'echo \"This tool requires manual installation (CUDA).\"'",
        false
    });

    dependencies.push_back({
        "nvidia-smi",
        "NVIDIA System Management Interface (for GPU Usage)",
        "which nvidia-smi",
        "pkexec sh -c 'echo \"Please install proprietary NVIDIA drivers for your distribution.\"'",
        "pkexec sh -c 'echo \"Please install proprietary NVIDIA drivers for your distribution.\"'",
        "pkexec sh -c 'echo \"Please install proprietary NVIDIA drivers for your distribution.\"'",
        false
    });

    dependencies.push_back({
        "xrandr",
        "X11 Display Control (Resolution/Scale)",
        "which xrandr",
        "pkexec pacman -S --noconfirm xorg-xrandr",
        "pkexec apt install -y x11-xserver-utils",
        "pkexec dnf install -y xorg-x11-server-utils",
        true
    });

    dependencies.push_back({
        "xinput",
        "X11 Input Device Control (Touchpad/Mouse)",
        "which xinput",
        "pkexec pacman -S --noconfirm xorg-xinput",
        "pkexec apt install -y xinput",
        "pkexec dnf install -y xorg-x11-apps",
        true
    });

    dependencies.push_back({
        "gsettings",
        "GNOME Settings Tool (Input Control)",
        "which gsettings",
        "pkexec pacman -S --noconfirm glib2",
        "pkexec apt install -y libglib2.0-bin",
        "pkexec dnf install -y glib2",
        true
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

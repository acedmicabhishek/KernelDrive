#include "plugin_installer.h"
#include <filesystem>
#include <iostream>
#include <thread>
#include <array>
#include <memory>
#include <format>

namespace fs = std::filesystem;

PluginInstaller& PluginInstaller::get() {
    static PluginInstaller instance;
    return instance;
}

bool PluginInstaller::run_command(const std::string& cmd, std::string& output) {
    std::array<char, 128> buffer;
    std::string result;
    std::string full_cmd = cmd + " 2>&1";
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(full_cmd.c_str(), "r"), pclose);
    if (!pipe) {
        output = "Failed to start command: " + cmd;
        return false;
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    output = result;
    return (pclose(pipe.release()) == 0);
}

void PluginInstaller::install_plugin(const std::string& repo_url, const std::string& slug, 
                                     std::function<void(InstallStatus, std::string)> callback) {
    
    std::thread([=, this]() {
        std::string home = getenv("HOME");
        fs::path build_base = fs::path(home) / ".cache" / "kerneldrive" / "build";
        fs::path plugin_dir = build_base / slug;
        fs::path install_dest = fs::path(home) / ".local" / "share" / "kerneldrive" / "plugins";
        
        fs::create_directories(build_base);
        fs::create_directories(install_dest);

        callback(InstallStatus::Cloning, "Cloning repository...");
        
        if (fs::exists(plugin_dir)) {
            fs::remove_all(plugin_dir);
        }

        std::string output;
        std::string clone_cmd = std::format("git clone --depth 1 {} {}", repo_url, plugin_dir.string());
        if (!run_command(clone_cmd, output)) {
            callback(InstallStatus::Failed, "Failed to clone repository.\n" + output);
            return;
        }
        
        if (!fs::exists(plugin_dir / "meson.build")) {
            callback(InstallStatus::Failed, "Repository cloned but no meson.build found.\n" + output);
            return;
        }

        callback(InstallStatus::Building, "Configuring build (Meson)...");
        std::string build_dir = (plugin_dir / "build").string();
        std::string setup_cmd = std::format("meson setup {} {} --prefix /usr", build_dir, plugin_dir.string());
        
        if (!run_command(setup_cmd, output)) {
            callback(InstallStatus::Failed, "Meson setup failed.\n" + output);
            return;
        }
        
        callback(InstallStatus::Building, "Compiling (Ninja)...");
        std::string compile_cmd = std::format("ninja -C {}", build_dir);
        if (!run_command(compile_cmd, output)) {
             callback(InstallStatus::Failed, "Compilation failed.\n" + output);
             return;
        }

        callback(InstallStatus::Installing, "Installing plugin...");
        
        bool found = false;
        try {
            for (const auto& entry : fs::recursive_directory_iterator(build_dir)) {
                if (entry.path().extension() == ".so") {
                    fs::path dest = install_dest / entry.path().filename();
                    fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing);
                    found = true;
                }
            }
        } catch (const std::exception& e) {
            callback(InstallStatus::Failed, std::string("Installation copy failed: ") + e.what());
            return;
        }

        if (found) {
            callback(InstallStatus::Success, "Plugin installed successfully!");
        } else {
            callback(InstallStatus::Failed, "Build completed but no .so file found.");
        }

    }).detach();
}

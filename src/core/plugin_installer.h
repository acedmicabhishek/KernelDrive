#pragma once
#include <string>
#include <functional>
#include <vector>

enum class InstallStatus {
    Idle,
    Cloning,
    Building,
    Installing,
    Success,
    Failed
};

class PluginInstaller {
public:
    static PluginInstaller& get();
    void install_plugin(const std::string& repo_url, const std::string& slug, 
                        std::function<void(InstallStatus, std::string)> callback);

private:
    PluginInstaller() = default;
    
    bool run_command(const std::string& cmd, std::string& output);
};

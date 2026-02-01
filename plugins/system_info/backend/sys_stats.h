#pragma once
#include <string>
#include <vector>

struct CpuStats {
    double usage_percent;
    double frequency_mhz;
    double temp_celsius;
};

struct RamStats {
    long total_mb;
    long used_mb;
    long swap_total_mb;
    long swap_used_mb;
};

struct DiskStats {
    long total_gb;
    long used_gb;
    double usage_percent;
};

struct OsInfo {
    std::string os_name;
    std::string kernel_version;
    std::string hostname;
    std::string uptime;
    std::string logo_icon; 
};

struct GpuStats {
    std::string name;
    double usage_percent;
};

struct NetStats {
    long rx_bytes_sec;
    long tx_bytes_sec;
};

class SysStats {
public:
    static CpuStats get_cpu_stats();
    static RamStats get_ram_stats();
    static DiskStats get_disk_stats();
    static OsInfo get_os_info();
    static std::vector<GpuStats> get_gpu_stats();
    static NetStats get_net_stats();
    static std::string get_cpu_model();
};

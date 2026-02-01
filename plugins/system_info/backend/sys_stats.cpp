#include "sys_stats.h"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <cmath>
#include <algorithm>
#include <map>
#include <climits>
#include <cstdlib>

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    std::string line;
    if (f) std::getline(f, line);
    return line;
}

CpuStats SysStats::get_cpu_stats() {
    static long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;
    static long prev_iowait = 0, prev_irq = 0, prev_softirq = 0, prev_steal = 0;

    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);

    std::stringstream ss(line);
    std::string label;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long active = user + nice + system + irq + softirq + steal;
    long total = active + idle + iowait;

    long prev_active = prev_user + prev_nice + prev_system + prev_irq + prev_softirq + prev_steal;
    long prev_total = prev_active + prev_idle + prev_iowait;

    long diff_active = active - prev_active;
    long diff_total = total - prev_total;
    
    double usage = 0.0;
    if (diff_total > 0) {
        usage = (double)diff_active / diff_total * 100.0;
    }

    prev_user = user; prev_nice = nice; prev_system = system; prev_idle = idle;
    prev_iowait = iowait; prev_irq = irq; prev_softirq = softirq; prev_steal = steal;

    double freq = 0;
    std::ifstream freq_file("/proc/cpuinfo");
    int cores = 0;
    while(std::getline(freq_file, line)) {
        if (line.find("cpu MHz") != std::string::npos) {
             size_t pos = line.find(":");
             if (pos != std::string::npos) {
                 freq += std::stod(line.substr(pos + 1));
                 cores++;
             }
        }
    }
    if (cores > 0) freq /= cores;

    double temp = 0;
    std::string t = read_file("/sys/class/thermal/thermal_zone0/temp");
    if (!t.empty()) temp = std::stod(t) / 1000.0;

    return {usage, freq, temp};
}

RamStats SysStats::get_ram_stats() {
    struct sysinfo si;
    sysinfo(&si);
    
    long total = si.totalram * si.mem_unit / 1024 / 1024;
    long free = si.freeram * si.mem_unit / 1024 / 1024;
    long buffer = si.bufferram * si.mem_unit / 1024 / 1024;


    long mem_avail = 0;
    std::ifstream mem("/proc/meminfo");
    std::string key;
    long val;
    std::string unit;
    while(mem >> key >> val >> unit) {
        if (key == "MemAvailable:") mem_avail = val / 1024;
    }

    if (mem_avail > 0) {
        return {total, total - mem_avail, (long)(si.totalswap * si.mem_unit / 1024 / 1024), (long)((si.totalswap - si.freeswap) * si.mem_unit / 1024 / 1024)};
    }

    return {total, total - free, 0, 0};
}

DiskStats SysStats::get_disk_stats() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) return {0,0,0};

    long total = (stat.f_blocks * stat.f_frsize) / 1024 / 1024 / 1024;
    long avail = (stat.f_bavail * stat.f_frsize) / 1024 / 1024 / 1024;
    long used = total - avail;

    return {total, used, (double)used / total * 100.0};
}

OsInfo SysStats::get_os_info() {
    std::ifstream release("/etc/os-release");
    std::string line;
    std::string name = "Linux";
    std::string id = "linux";
    
    while(std::getline(release, line)) {
        if (line.find("PRETTY_NAME=") == 0) {
            name = line.substr(12);
            name.erase(remove(name.begin(), name.end(), '\"'), name.end());
        }
        if (line.find("ID=") == 0) {
            id = line.substr(3);
            id.erase(remove(id.begin(), id.end(), '\"'), id.end());
        }
    }

    char hostname[256];
    gethostname(hostname, 256);

    struct sysinfo si;
    sysinfo(&si);
    
    long days = si.uptime / 86400;
    long hours = (si.uptime % 86400) / 3600;
    long mins = (si.uptime % 3600) / 60;
    
    std::stringstream up;
    if (days > 0) up << days << "d ";
    up << hours << "h " << mins << "m";

    return {name, read_file("/proc/sys/kernel/osrelease"), std::string(hostname), up.str(), id + "-symbolic"}; 
}

std::string SysStats::get_cpu_model() {
    std::ifstream f("/proc/cpuinfo");
    std::string line;
    while(std::getline(f, line)) {
        if (line.find("model name") != std::string::npos) {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                std::string name = line.substr(pos + 2);
                return name;
            }
        }
    }
    return "Processor";
}

static std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    while (fgets(buffer, sizeof buffer, pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

std::vector<GpuStats> SysStats::get_gpu_stats() {
    std::vector<GpuStats> results;
    
    std::stringstream ss(exec("lspci -mm | grep -E 'VGA|3D'"));
    std::string line;
    
    struct GpuInfo {
        std::string pci_slot;
        std::string vendor;
        std::string model;
    };
    std::vector<GpuInfo> detected_cards;

    while(std::getline(ss, line)) {
        std::stringstream ls(line);
        size_t first_space = line.find(' ');
        if (first_space == std::string::npos) continue;
        
        std::string id = line.substr(0, first_space);
        
        int quote_count = 0;
        size_t v_start = 0, v_end = 0;
        size_t d_start = 0, d_end = 0;
        
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '"') {
                quote_count++;
                if (quote_count == 3) v_start = i + 1;
                if (quote_count == 4) v_end = i;
                if (quote_count == 5) d_start = i + 1;
                if (quote_count == 6) { d_end = i; break; }
            }
        }
        
        if (d_end > d_start) {
            detected_cards.push_back({
                id,
                line.substr(v_start, v_end - v_start),
                line.substr(d_start, d_end - d_start)
            });
        }
    }

    for (const auto& card : detected_cards) {
        double usage = 0.0;

        if (card.vendor.find("NVIDIA") != std::string::npos) {
            std::string out = exec("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits");
            if (!out.empty()) {
                try {
                    usage = std::stod(out);
                } catch(...) { usage = 0; }
            }
        } 
        else {
             for (int i = 0; i < 4; ++i) {
                std::string card_path = "/sys/class/drm/card" + std::to_string(i);
                std::string link_path = card_path + "/device";
                char real_path[PATH_MAX];
                if (realpath(link_path.c_str(), real_path)) {
                    std::string s_path(real_path);
                    if (s_path.find(card.pci_slot) != std::string::npos) {
                        std::ifstream f(card_path + "/device/gpu_busy_percent");
                        if (f) {
                            f >> usage;
                        }
                    }
                }
             }
        }
        
        std::string pretty_name = card.model;
        if (pretty_name.find('[') != std::string::npos) {
            size_t start = pretty_name.find('[') + 1;
            size_t end = pretty_name.find(']');
            if (end > start) pretty_name = pretty_name.substr(start, end - start);
        }
        
        if (card.vendor.find("NVIDIA") != std::string::npos && pretty_name.find("NVIDIA") == std::string::npos) {
            pretty_name = "NVIDIA " + pretty_name;
        }

        results.push_back({pretty_name, usage});
    }
    
    if (results.empty()) results.push_back({"GPU", 0});
    return results;
}

NetStats SysStats::get_net_stats() {
    static long prev_rx = 0;
    static long prev_tx = 0;
    static double last_time = 0;

    long curr_rx = 0;
    long curr_tx = 0;
    
    std::ifstream f("/proc/net/dev");
    std::string line;
    std::getline(f, line);
    std::getline(f, line);
    
    while(std::getline(f, line)) {
        if (line.find("lo:") != std::string::npos) continue;
        
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string data = line.substr(colon + 1);
            std::stringstream ss(data);
            long r_bytes, r_pkt, r_err, r_drop, r_fifo, r_frame, r_comp, r_multi;
            long t_bytes;
            
            ss >> r_bytes >> r_pkt >> r_err >> r_drop >> r_fifo >> r_frame >> r_comp >> r_multi >> t_bytes;
            
            curr_rx += r_bytes;
            curr_tx += t_bytes;
        }
    }
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double now = ts.tv_sec + ts.tv_nsec / 1e9;
    
    long rx_speed = 0;
    long tx_speed = 0;

    if (last_time > 0) {
        double dt = now - last_time;
        if (dt > 0) {
            rx_speed = (long)((curr_rx - prev_rx) / dt);
            tx_speed = (long)((curr_tx - prev_tx) / dt);
        }
    }
    
    prev_rx = curr_rx;
    prev_tx = curr_tx;
    last_time = now;
    
    return {rx_speed, tx_speed};
}

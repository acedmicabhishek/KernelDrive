#include "monitor.h"
#include <fstream>
#include <array>
#include <cstdio>
#include <memory>
#include <iostream>
#include <algorithm>
#include <vector>

namespace AsusMonitor {

    static int cached_ram_speed = 0;

    struct PcloseDeleter {
        void operator()(FILE* f) const { if (f) pclose(f); }
    };
    static std::string run_cmd(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, PcloseDeleter> pipe(popen(cmd, "r"));
        if (!pipe) {
            return "";
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    void init() {
        std::string output = run_cmd("pkexec dmidecode -t memory 2>/dev/null | grep 'Speed:' | head -n 1");
        if (output.empty()) {
             output = run_cmd("dmidecode -t memory 2>/dev/null | grep 'Speed:' | head -n 1");
        }
        
        if (!output.empty()) {
            size_t colon = output.find(":");
            if (colon != std::string::npos) {
                std::string val = output.substr(colon + 1);
                std::string num_str;
                for (char c : val) {
                    if (isdigit(c)) num_str += c;
                    else if (!num_str.empty()) break;
                }
                if (!num_str.empty()) {
                    cached_ram_speed = std::stoi(num_str);
                }
            }
        }
    }

    CpuMetrics get_cpu_metrics() {
        CpuMetrics m = {0, cached_ram_speed};
        
        std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        if (f.is_open()) {
            int khz = 0;
            f >> khz;
            m.freq_mhz = khz / 1000;
        }

        return m;
    }

    GpuMetrics get_gpu_metrics() {
        GpuMetrics m = {0, 0, 0, 0};

        std::string out = run_cmd("nvidia-smi --query-gpu=clocks.gr,clocks.mem,memory.used,memory.total --format=csv,noheader,nounits 2>/dev/null");
        
        if (!out.empty()) {
            std::vector<int> values;
            std::string current;
            for (char c : out) {
                if (c == ',') {
                    if (!current.empty()) values.push_back(std::stoi(current));
                    current.clear();
                } else if (isdigit(c)) {
                    current += c;
                }
            }
            if (!current.empty()) values.push_back(std::stoi(current));

            if (values.size() >= 4) {
                m.core_clock_mhz = values[0];
                m.memory_clock_mhz = values[1];
                m.vram_used_mb = values[2];
                m.vram_total_mb = values[3];
            }
        }

        return m;
    }

    std::string format_freq(int mhz) {
        if (mhz >= 1000) {
            float ghz = mhz / 1000.0f;
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f GHz", ghz);
            return std::string(buf);
        }
        return std::to_string(mhz) + " MHz";
    }

    std::string format_speed(int speed) {
        return std::to_string(speed) + " MT/s";
    }

}

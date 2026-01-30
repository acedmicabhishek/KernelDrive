#pragma once
#include <utility>
#include <string>

namespace AsusFans {
    struct FanMetrics {
        int cpu_rpm;
        int gpu_rpm;
        int cpu_temp;
        int gpu_temp;
    };

    FanMetrics get_metrics();
    
    std::string get_cpu_name();
    std::string get_gpu_name();

    bool is_supported();
}

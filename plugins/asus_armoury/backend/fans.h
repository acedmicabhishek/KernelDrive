#pragma once
#include <utility>

namespace AsusFans {
    struct FanMetrics {
        int cpu_rpm;
        int gpu_rpm;
        int cpu_temp;
        int gpu_temp;
    };

    FanMetrics get_metrics();

    bool is_supported();
}

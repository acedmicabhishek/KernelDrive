#pragma once
#include <utility>

namespace AsusFans {
    std::pair<int, int> get_fan_speeds();
    bool is_supported();
}

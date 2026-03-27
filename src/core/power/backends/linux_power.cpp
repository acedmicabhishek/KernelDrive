#include "linux_power.h"
#include "../../sysfs_writer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

bool LinuxPowerBackend::is_available() { return true; }

std::vector<BatteryInfo> LinuxPowerBackend::get_batteries() {
  std::vector<BatteryInfo> batteries;
  std::string base = "/sys/class/power_supply/";
  if (!fs::exists(base))
    return batteries;

  for (const auto &entry : fs::directory_iterator(base)) {
    std::string name = entry.path().filename().string();

    std::string type = read_file(entry.path() / "type");
    if (remove_newline(type) != "Battery")
      continue;

    BatteryInfo info;
    info.present = true;

    std::string cap = read_file(entry.path() / "capacity");
    info.percentage = safe_stoi(cap);

    std::string st = remove_newline(read_file(entry.path() / "status"));
    if (st == "Charging")
      info.state = BatteryState::Charging;
    else if (st == "Discharging")
      info.state = BatteryState::Discharging;
    else if (st == "Full")
      info.state = BatteryState::Full;
    else if (st == "Not charging")
      info.state = BatteryState::NotCharging;
    else
      info.state = BatteryState::Unknown;

    long energy_now = safe_stol(read_file(entry.path() / "energy_now"));
    long energy_full = safe_stol(read_file(entry.path() / "energy_full"));
    long power_now = safe_stol(read_file(entry.path() / "power_now"));

    if (energy_now == 0) {
      long charge_now = safe_stol(read_file(entry.path() / "charge_now"));
      long charge_full = safe_stol(read_file(entry.path() / "charge_full"));
      long voltage = safe_stol(read_file(entry.path() / "voltage_now"));

      energy_now = (long)((double)charge_now * voltage / 1000000.0);
      energy_full = (long)((double)charge_full * voltage / 1000000.0);
    }
    if (power_now == 0) {
      long current = safe_stol(read_file(entry.path() / "current_now"));
      long voltage = safe_stol(read_file(entry.path() / "voltage_now"));
      power_now = (long)((double)current * voltage / 1000000.0);
    }

    info.energy_now_wh = energy_now / 1000000.0;
    info.energy_full_wh = energy_full / 1000000.0;
    info.energy_rate_w = power_now / 1000000.0;

    if (info.energy_rate_w > 0.1) {
      double hours = 0;
      if (info.state == BatteryState::Discharging) {
        hours = info.energy_now_wh / info.energy_rate_w;
      } else if (info.state == BatteryState::Charging) {
        hours = (info.energy_full_wh - info.energy_now_wh) / info.energy_rate_w;
      }

      if (hours > 0) {
        int h = (int)hours;
        int m = (int)((hours - h) * 60);
        info.time_remaining_str =
            std::to_string(h) + "h " + std::to_string(m) + "m";
      }
    }

    batteries.push_back(info);
  }
  return batteries;
}

PowerProfileInfo LinuxPowerBackend::get_profile_info() {
  PowerProfileInfo info;
  return info;
}

void LinuxPowerBackend::set_profile(
    [[maybe_unused]] const std::string &profile) {
}

std::vector<std::string> LinuxPowerBackend::get_cpu_governors() {
  std::vector<std::string> govs;
  std::string path =
      "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors";
  if (fs::exists(path)) {
    std::string content = read_file(path);
    std::stringstream ss(content);
    std::string item;
    while (ss >> item) {
      govs.push_back(item);
    }
  }
  return govs;
}

std::string LinuxPowerBackend::read_file(const fs::path &p) {
  if (!fs::exists(p))
    return "";
  std::ifstream f(p);
  std::stringstream buffer;
  buffer << f.rdbuf();
  return buffer.str();
}

std::string LinuxPowerBackend::remove_newline(std::string s) {
  s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
  return s;
}

int LinuxPowerBackend::safe_stoi(const std::string &s) {
  try {
    return std::stoi(s);
  } catch (...) {
    return 0;
  }
}

long LinuxPowerBackend::safe_stol(const std::string &s) {
  try {
    return std::stol(s);
  } catch (...) {
    return 0;
  }
}

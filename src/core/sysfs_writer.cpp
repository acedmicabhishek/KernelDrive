#include "sysfs_writer.h"
#include <fstream>
#include <iostream>

std::optional<std::string> SysfsWriter::read(const std::string& path) {
    std::ifstream file(path);
    if (file.is_open()) {
        std::string value;
        if (std::getline(file, value)) {
            return value;
        }
    }
    return std::nullopt;
}

bool SysfsWriter::write(const std::string& path, const std::string& value) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "SysfsWriter: cannot open " << path << " for writing" << std::endl;
        return false;
    }
    file << value;
    file.flush();
    if (!file.good()) {
        std::cerr << "SysfsWriter: failed to write '" << value << "' to " << path << std::endl;
        return false;
    }
    return true;
}

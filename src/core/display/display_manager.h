#pragma once
#include "display_backend.h"
#include <memory>

class DisplayManager {
public:
    static DisplayManager& get();
    
    std::shared_ptr<IDisplayBackend> get_backend();

private:
    DisplayManager();
    std::shared_ptr<IDisplayBackend> backend;
};

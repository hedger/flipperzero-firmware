#include "flash_manager.h"
#include <memory>

// app enter function
extern "C" int32_t flash_manager_app(void* args) {
    std::unique_ptr<FlashManager> pApp = std::make_unique<FlashManager>();
    pApp->run(reinterpret_cast<const char*>(args));

    return 0;
}

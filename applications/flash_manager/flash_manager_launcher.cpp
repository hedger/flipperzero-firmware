#include "flash_manager.h"
#include <memory>

// app enter function
extern "C" int32_t flash_manager_app(void* p) {
    std::unique_ptr<FlashManager> pApp = std::make_unique<FlashManager>();
    pApp->run();

    return 0;
}

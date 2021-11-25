#include "flash_manager.h"

// app enter function
extern "C" int32_t flash_manager_app(void* p) {
    FlashManager* app = new FlashManager();
    app->run();
    delete app;

    return 0;
}

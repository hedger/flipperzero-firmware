#pragma once
#include "../flash_manager.h"

class FlashManagerSceneByteInput : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    void result_callback(void* context);

    uint8_t data[4] = {
        0x01,
        0xA2,
        0xF4,
        0xD3,
    };
};
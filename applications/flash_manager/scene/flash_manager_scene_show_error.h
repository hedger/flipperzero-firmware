#pragma once
#include "../flash_manager.h"

class FlashManagerSceneShowError : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    static void timeout_callback(void* context);
};
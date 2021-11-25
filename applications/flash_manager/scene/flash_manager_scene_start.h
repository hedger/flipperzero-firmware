#pragma once
#include "../flash_manager.h"

class FlashManagerSceneStart : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    void submenu_callback(void* context, uint32_t index);
    uint32_t submenu_item_selected = 0;
};
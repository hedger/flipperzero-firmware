#include "flash_manager_scene_start.h"

typedef enum {
    SubmenuByteInput,
} SubmenuIndex;

void FlashManagerSceneStart::on_enter(FlashManager* app, bool need_restore) {
    auto submenu = app->view_controller.get<SubmenuVM>();
    auto callback = cbc::obtain_connector(this, &FlashManagerSceneStart::submenu_callback);

    submenu->add_item("Byte Input", SubmenuByteInput, callback, app);

    if(need_restore) {
        submenu->set_selected_item(submenu_item_selected);
    }
    app->view_controller.switch_to<SubmenuVM>();
}

bool FlashManagerSceneStart::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    if(event->type == FlashManager::EventType::MenuSelected) {
        submenu_item_selected = event->payload.menu_index;
        switch(event->payload.menu_index) {
        case SubmenuByteInput:
            app->scene_controller.switch_to_next_scene(FlashManager::SceneType::ByteInputScene);
            break;
        }
        consumed = true;
    }

    return consumed;
}

void FlashManagerSceneStart::on_exit(FlashManager* app) {
    app->view_controller.get<SubmenuVM>()->clean();
}

void FlashManagerSceneStart::submenu_callback(void* context, uint32_t index) {
    FlashManager* app = static_cast<FlashManager*>(context);
    FlashManager::Event event;

    event.type = FlashManager::EventType::MenuSelected;
    event.payload.menu_index = index;

    app->view_controller.send_event(&event);
}

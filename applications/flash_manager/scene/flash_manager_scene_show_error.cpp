#include "flash_manager_scene_show_error.h"

void FlashManagerSceneShowError::on_enter(FlashManager* app, bool need_restore) {
    auto popup = app->view_controller.get<PopupVM>();

    popup->set_icon(32, 12, &I_DolphinFirstStart7_61x51);
    popup->set_text(string_get_cstr(app->error_str), 64, 8, AlignCenter, AlignBottom);
    popup->set_context(app);
    popup->set_callback(FlashManagerSceneShowError::timeout_callback);
    popup->set_timeout(2500);
    popup->enable_timeout();

    app->view_controller.switch_to<PopupVM>();
    app->notify_error();
}

bool FlashManagerSceneShowError::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    if(event->type == FlashManager::EventType::Back) {
        //consumed = true;
    }

    return consumed;
}

void FlashManagerSceneShowError::on_exit(FlashManager* app) {
    app->view_controller.get<PopupVM>()->clean();
    string_reset(app->error_str);
}

void FlashManagerSceneShowError::timeout_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Back};
    reinterpret_cast<FlashManager*>(context)->view_controller.send_event(&event);
}
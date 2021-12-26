#include "flash_manager_scene_show_success.h"

void FlashManagerSceneShowSuccess::on_enter(FlashManager* app, bool need_restore) {
    auto popup = app->view_controller.get<PopupVM>();

    popup->set_icon(32, 5, &I_DolphinNice_96x59);
    popup->set_text("3ae6ok!", 13, 22, AlignLeft, AlignBottom);
    popup->set_context(app);
    popup->set_callback(FlashManagerSceneShowSuccess::timeout_callback);
    popup->set_timeout(2000);
    popup->enable_timeout();

    app->view_controller.switch_to<PopupVM>();
    app->notify(FlashManager::NotificationMode::Success);
}

bool FlashManagerSceneShowSuccess::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    if(event->type == FlashManager::EventType::Back) {
        // TODO: bydlocode, fixme
        if(!app->run_in_app_mode) {
            app->scene_controller.search_and_switch_to_previous_scene(
                {FlashManager::SceneType::Start});
            consumed = true;
        } else {
            app->scene_controller.switch_to_previous_scene();
        }
    }

    return consumed;
}

void FlashManagerSceneShowSuccess::on_exit(FlashManager* app) {
    app->view_controller.get<PopupVM>()->clean();
}

void FlashManagerSceneShowSuccess::timeout_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Back};
    reinterpret_cast<FlashManager*>(context)->view_controller.send_event(&event);
}
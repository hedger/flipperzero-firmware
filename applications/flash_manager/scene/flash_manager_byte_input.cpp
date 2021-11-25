#include "flash_manager_scene_byte_input.h"

void FlashManagerSceneByteInput::on_enter(FlashManager* app, bool need_restore) {
    ByteInputVM* byte_input = app->view_controller;
    auto callback = cbc::obtain_connector(this, &FlashManagerSceneByteInput::result_callback);

    byte_input->set_result_callback(callback, NULL, app, data, 4);
    byte_input->set_header_text("Enter the key");

    app->view_controller.switch_to<ByteInputVM>();
}

bool FlashManagerSceneByteInput::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    if(event->type == FlashManager::EventType::ByteEditResult) {
        app->scene_controller.switch_to_previous_scene();
        consumed = true;
    }

    return consumed;
}

void FlashManagerSceneByteInput::on_exit(FlashManager* app) {
    app->view_controller.get<ByteInputVM>()->clean();
}

void FlashManagerSceneByteInput::result_callback(void* context) {
    FlashManager* app = static_cast<FlashManager*>(context);
    FlashManager::Event event;

    event.type = FlashManager::EventType::ByteEditResult;

    app->view_controller.send_event(&event);
}

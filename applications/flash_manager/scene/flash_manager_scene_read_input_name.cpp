#include "flash_manager_scene_read_input_name.h"

#include <lib/toolbox/random_name.h>

void FlashManagerSceneReadDumpInputFilename::on_enter(FlashManager* app, bool need_restore) {
    // TODO: fix
    const char* key_name = ""; //app->worker.key.get_name();

    bool key_name_empty = !strcmp(key_name, "");
    if(key_name_empty) {
        set_random_name(app->text_store.text, app->text_store.text_size);
    } else {
        app->text_store.set("%s", key_name);
    }

    auto text_input = app->view_controller.get<TextInputVM>();
    text_input->set_header_text("Name the dump file");

    text_input->set_result_callback(save_callback, app, app->text_store.text, 32, key_name_empty);

    app->view_controller.switch_to<TextInputVM>();
}

bool FlashManagerSceneReadDumpInputFilename::on_event(
    FlashManager* app,
    FlashManager::Event* event) {
    bool consumed = false;

    if(event->type == FlashManager::EventType::Next) {
        //if(strlen(app->worker.key.get_name())) {
        //    app->delete_key(&app->worker.key);
        //}

        //app->worker.key.set_name(app->text_store.text);

        //if(app->save_key(&app->worker.key)) {
        //    app->scene_controller.switch_to_next_scene(LfRfidApp::SceneType::SaveSuccess);
        //} else {
        //    app->scene_controller.search_and_switch_to_previous_scene(
        //        {LfRfidApp::SceneType::ReadedMenu});
        //}
        FURI_LOG_I(TAG, "saving dump to '%s'", app->text_store.text);
        app->scene_controller.switch_to_next_scene(FlashManager::SceneType::ReadImgProcessScene);
    }

    return consumed;
}

void FlashManagerSceneReadDumpInputFilename::on_exit(FlashManager* app) {
    app->view_controller.get<TextInputVM>()->clean();
}

void FlashManagerSceneReadDumpInputFilename::save_callback(void* context) {
    FlashManager* app = static_cast<FlashManager*>(context);
    FlashManager::Event event{.type = FlashManager::EventType::Next};
    app->view_controller.send_event(&event);
}
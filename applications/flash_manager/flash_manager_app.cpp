#include "flash_manager.h"
#include "scene/flash_manager_scene_start.h"
#include "scene/flash_manager_scene_byte_input.h"

FlashManager::FlashManager()
    : scene_controller{this}
    , text_store{128}
    , notification{"notification"} {
}

FlashManager::~FlashManager() {
    FURI_LOG_I(TAG, "stopped");
}

void FlashManager::run() {
    FURI_LOG_I(TAG, "starting");

    scene_controller.add_scene(SceneType::Start, new FlashManagerSceneStart());
    scene_controller.add_scene(SceneType::ByteInputScene, new FlashManagerSceneByteInput());

    notification_message(notification, &sequence_blink_green_10);
    scene_controller.process(100);
}

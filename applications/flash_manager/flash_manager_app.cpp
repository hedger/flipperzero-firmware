#include "flash_manager.h"
#include "flash_manager_worker.h"
#include "scene/flash_manager_scene_start.h"
#include "scene/flash_manager_scene_chip_id.h"
#include "scene/flash_manager_scene_chip_info.h"
#include "scene/flash_manager_scene_read_input_name.h"
#include "scene/flash_manager_scene_read_dump.h"
#include "scene/flash_manager_scene_write_dump.h"
#include "scene/flash_manager_scene_show_error.h"
#include "scene/flash_manager_scene_show_success.h"

FlashManager::FlashManager()
    : scene_controller{this}
    , text_store{128}
    , notification{"notification"}
    , storage{"storage"}
    , dialogs{"dialogs"}
{
    string_init(error_str);
}

FlashManager::~FlashManager() {
    file_tools.close();
    string_clear(error_str);
    FURI_LOG_I(TAG, "stopped");
}

int32_t FlashManager::run(const char* args) {
    FURI_LOG_I(TAG, "starting");

    if(!make_app_folder()) {
        FURI_LOG_E(TAG, "Cannot create app folder");
        return 0;
    }

    worker = std::make_unique<FlashManagerWorker>();
    worker->start();

    if(strlen(args)) {
        FURI_LOG_I(TAG, "started app to write '%s'", args);
        text_store.set(args);
        run_in_app_mode = true;
        scene_controller.add_scene(SceneType::Start, new FlashManagerSceneChipID());
        scene_controller.add_scene(SceneType::ChipInfoScene, new FlashManagerSceneChipInfo());
        scene_controller.add_scene(SceneType::WriteImgProcessScene, new FlashManagerSceneWriteDump());
        scene_controller.add_scene(SceneType::ReadImgProcessScene, new FlashManagerSceneReadDump());
        scene_controller.add_scene(SceneType::ShowError, new FlashManagerSceneShowError());
        scene_controller.add_scene(SceneType::ShowSuccess, new FlashManagerSceneShowSuccess());
        scene_controller.process(TICK_LEN_MS, SceneType::Start);
    } else {
        scene_controller.add_scene(SceneType::Start, new FlashManagerSceneStart());
        scene_controller.add_scene(SceneType::ChipIDScene, new FlashManagerSceneChipID());
        scene_controller.add_scene(SceneType::ChipInfoScene, new FlashManagerSceneChipInfo());
        scene_controller.add_scene(SceneType::ReadImgFileNameInputScene, new FlashManagerSceneReadDumpInputFilename());
        scene_controller.add_scene(SceneType::ReadImgProcessScene, new FlashManagerSceneReadDump());
        scene_controller.add_scene(SceneType::ShowError, new FlashManagerSceneShowError());
        scene_controller.add_scene(SceneType::ShowSuccess, new FlashManagerSceneShowSuccess());
        scene_controller.process(TICK_LEN_MS);
    }

    worker->stop();
    return 0;
}

SpiFlashInfo_t* FlashManager::get_flash_info() {
    return &flash_info;
}

void FlashManager::notify_green_blink() {
    notification_message(notification, &sequence_blink_green_10);
}

void FlashManager::notify_yellow_blink() {
    notification_message(notification, &sequence_blink_yellow_10);
}

void FlashManager::notify_red_blink() {
    notification_message(notification, &sequence_blink_red_10);
}

void FlashManager::notify_error() {
    notification_message(notification, &sequence_error);
}

void FlashManager::notify_success() {
    notification_message(notification, &sequence_success);
}

void FlashManager::notify_green_on() {
    notification_message_block(notification, &sequence_set_green_255);
}

void FlashManager::notify_green_off() {
    notification_message(notification, &sequence_reset_green);
}

void FlashManager::notify_red_on() {
    notification_message_block(notification, &sequence_set_red_255);
}

void FlashManager::notify_red_off() {
    notification_message(notification, &sequence_reset_red);
}

bool FlashManager::make_app_folder() {
    bool success = storage_simply_mkdir(storage, FlashManagerFileTools::get_app_folder());
    if(!success) {
        dialog_message_show_storage_error(dialogs, "Cannot create\napp folder");
    }
    return success;
}
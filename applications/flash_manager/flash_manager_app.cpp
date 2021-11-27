#include "flash_manager.h"
#include "flash_manager_worker.h"
#include "scene/flash_manager_scene_start.h"
#include "scene/flash_manager_scene_byte_input.h"
#include "scene/flash_manager_scene_chip_id.h"
#include "scene/flash_manager_scene_read_input_name.h"
#include "scene/flash_manager_scene_read_dump.h"

FlashManager::FlashManager()
    : scene_controller{this}
    , text_store{128}
    , notification{"notification"}
    , storage{"storage"}
    , dialogs{"dialogs"} {
}

FlashManager::~FlashManager() {
    FURI_LOG_I(TAG, "stopped");
}

int32_t FlashManager::run(const char* args) {
    FURI_LOG_I(TAG, "starting");
    make_app_folder();

    //notification_message(notification, &sequence_blink_green_10);
    worker = std::make_unique<FlashManagerWorker>();
    worker->start();

    if(strlen(args)) {
        FURI_LOG_I(TAG, "started app to write '%s'", args);
        //load_key_data(args, &worker.key);
        scene_controller.add_scene(SceneType::ChipIDScene, new FlashManagerSceneChipID());
        scene_controller.process(100, SceneType::ChipIDScene);
    } else {
        scene_controller.add_scene(SceneType::Start, new FlashManagerSceneStart());
        //scene_controller.add_scene(SceneType::ByteInputScene, new FlashManagerSceneByteInput());
        scene_controller.add_scene(SceneType::ChipIDScene, new FlashManagerSceneChipID());
        scene_controller.add_scene(
            SceneType::ReadImgFileNameInputScene, new FlashManagerSceneReadDumpInputFilename());
        scene_controller.add_scene(
            SceneType::ReadImgProcessScene, new FlashManagerSceneReadDump());
        scene_controller.process(100);
    }

    worker->stop();
    return 0;
}

const char* FlashManager::app_folder = "/any/flash";
const char* FlashManager::app_extension = ".bin";
const char* FlashManager::app_filetype = "Flash chip dump";

void FlashManager::make_app_folder() {
    if(!storage_simply_mkdir(storage, app_folder)) {
        dialog_message_show_storage_error(dialogs, "Cannot create\napp folder");
    }
    FURI_LOG_I(TAG, "folder check ok");
}
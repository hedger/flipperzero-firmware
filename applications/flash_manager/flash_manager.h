#pragma once

#include "flash_manager_file_tools.h"

#include <furi.h>
#include <furi-hal.h>

#include <memory>

#include <generic-scene.hpp>
#include <scene-controller.hpp>
#include <view-controller.hpp>
#include <record-controller.hpp>
#include <text-store.h>

#include <view-modules/submenu-vm.h>
#include <view-modules/text-input-vm.h>
#include <view-modules/byte-input-vm.h>
#include "../lfrfid/view/container-vm.h"

#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <notification/notification-messages.h>

#define TAG "FlashManager"

class FlashManagerWorker;

class FlashManager {
public:
    enum class EventType : uint8_t {
        GENERIC_EVENT_ENUM_VALUES,
        Next,
        MenuSelected,
        OpReadChip,
        OpWriteChip,
        Cancel
    };

    enum class SceneType : uint8_t {
        GENERIC_SCENE_ENUM_VALUES,
        ByteInputScene,
        ChipIDScene,
        ReadImgFileNameInputScene,
        ReadImgProcessScene,
        WriteImgProcessScene
    };

    class Event {
    public:
        union {
            int32_t menu_index;
        } payload;

        EventType type;
    };

    SceneController<GenericScene<FlashManager>, FlashManager> scene_controller;
    TextStore text_store;
    ViewController<FlashManager, SubmenuVM, ByteInputVM, TextInputVM, ContainerVM> view_controller;

    RecordController<NotificationApp> notification;
    //RecordController<Storage> storage;
    //RecordController<DialogsApp> dialogs;

    ~FlashManager();
    FlashManager();

    std::unique_ptr<FlashManagerWorker> worker;

    int32_t run(const char* args);


    static const uint8_t file_name_size = 100;
    char file_name[file_name_size];

    FlashManagerFileTools file_tools;
};

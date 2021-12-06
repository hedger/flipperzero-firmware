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

#include <view-modules/popup-vm.h>
#include <view-modules/submenu-vm.h>
#include <view-modules/text-input-vm.h>
#include <view-modules/byte-input-vm.h>
#include "../lfrfid/view/container-vm.h"

#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <notification/notification-messages.h>

#include "spi/spi_toolkit.h"

#define TAG "FlashManager"

class FlashManagerWorker;

class FlashManager {
    static const int TICK_LEN_MS = 50;

public:
    enum class EventType : uint8_t {
        GENERIC_EVENT_ENUM_VALUES,
        Next,
        MenuSelected,
        Scan,
        Retry,
        OpReadChip,
        OpWriteChip,
        Cancel
    };

    enum class SceneType : uint8_t {
        GENERIC_SCENE_ENUM_VALUES,
        ChipIDScene, // ScanScene
        ChipInfoScene, // InfoScene
        ReadImgFileNameInputScene, // SaveNameScene
        ReadImgProcessScene, // ReadScene
        WriteImgProcessScene, // WriteScene
        ShowError,
        ShowSuccess
    };

    class Event {
    public:
        union {
            uint32_t menu_index;
        } payload;

        EventType type;
    };

    SceneController<GenericScene<FlashManager>, FlashManager> scene_controller;
    TextStore text_store;
    ViewController<FlashManager, SubmenuVM, PopupVM, ByteInputVM, TextInputVM, ContainerVM> view_controller;

    RecordController<NotificationApp> notification;
    RecordController<Storage> storage;
    RecordController<DialogsApp> dialogs;

    string_t error_str;

    std::unique_ptr<FlashManagerWorker> worker;

    static const uint8_t MAX_FILE_NAME_LEN = 100;
    char file_name[MAX_FILE_NAME_LEN];
    bool run_in_app_mode = false;
    bool runVerification = false;

    FlashManagerFileTools file_tools;

    int32_t run(const char* args);

    FlashManager();
    ~FlashManager();

    SpiFlashInfo_t* get_flash_info();

    void notify_green_blink();
    void notify_yellow_blink();
    void notify_red_blink();
    void notify_error();
    void notify_success();
    void notify_green_on();
    void notify_green_off();
    void notify_red_on();
    void notify_red_off();
    
    bool make_app_folder();
private:
    SpiFlashInfo_t flash_info;
};
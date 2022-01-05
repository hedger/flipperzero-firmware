#pragma once

#include "flash_manager_file_tools.h"

#include <furi.h>
#include <furi_hal.h>

#include <memory>

#include <generic_scene.hpp>
#include <scene_controller.hpp>
#include <view_controller.hpp>
#include <record_controller.hpp>
#include <text_store.h>

#include <view_modules/popup_vm.h>
#include <view_modules/submenu_vm.h>
#include <view_modules/text_input_vm.h>
#include <view_modules/byte_input_vm.h>
#include "../lfrfid/view/container_vm.h"

#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <notification/notification_messages.h>

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
        OpVerifyChip,
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

    enum class NotificationMode {
        None,
        GreenBlink,
        YellowBlink,
        RedBlink,
        Error,
        Success,
        GreenOn,
        GreenOff,
        RedOn,
        RedOff
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

    void notify(NotificationMode mode);
    
    bool make_app_folder();
private:
    SpiFlashInfo_t flash_info;
};
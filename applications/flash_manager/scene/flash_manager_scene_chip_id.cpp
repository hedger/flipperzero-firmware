#include "flash_manager_scene_chip_id.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"
#include "../spi/spi_chips.h"

//static void wrapper(void* env) {
//    FURI_LOG_I(TAG, "wrapper()");
//    FlashManagerSceneChipID* pp = (FlashManagerSceneChipID*)env;
//    pp->start_chip_id(nullptr);
//}

void FlashManagerSceneChipID::rescan_callback(void* context) {
    FURI_LOG_I(TAG, "wrapper2()");
    reinterpret_cast<FlashManagerSceneChipID*>(context)->start_chip_id();
}

void FlashManagerSceneChipID::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;
    chip_detected = false;
    string_init(chip_id);
    string_init(chip_extra);

    ContainerVM* container = app->view_controller;

    back_btn = container->add<ButtonElement>();
    back_btn->set_type(ButtonElement::Type::Left, "Back");
    back_btn->set_callback(this, &FlashManagerSceneChipID::back_callback);
    back_btn->set_enabled(false);
    //app, cbc::obtain_connector(this, &FlashManagerSceneChipID::back_callback));

    run_detect_btn = container->add<ButtonElement>();
    run_detect_btn->set_type(ButtonElement::Type::Center, "Scan");
    run_detect_btn->set_callback(this, &FlashManagerSceneChipID::rescan_callback);
    run_detect_btn->set_enabled(false);

    next_btn = container->add<ButtonElement>();
    next_btn->set_enabled(false);

    header_line = container->add<StringElement>();
    detail_line = container->add<StringElement>();
    status_line = container->add<StringElement>();

    header_line->set_text(
        "Looking for SPI chip...", 64, 17, AlignCenter, AlignBottom, FontSecondary);
    detail_line->set_text("Please be patient.", 64, 29, AlignCenter, AlignBottom, FontSecondary);
    status_line->set_text("detecting...", 64, 41, AlignCenter, AlignBottom, FontSecondary);

    app->view_controller.switch_to<ContainerVM>();
    start_chip_id();
}

void FlashManagerSceneChipID::start_chip_id() {
    FURI_LOG_I(TAG, "start_chip_id()");
    run_detect_btn->set_enabled(false);
    chip_id_task = std::make_unique<WorkerTask>(
        WorkerOperation::ChipId, 0, reinterpret_cast<uint8_t*>(&flash_info), sizeof(flash_info));

    app->worker->enqueue_task(chip_id_task.get());
}

void FlashManagerSceneChipID::tick() {
    bool chip_detection_not_running = !(bool)chip_id_task;

    run_detect_btn->set_enabled(chip_detection_not_running);
    back_btn->set_enabled(chip_detection_not_running);
    next_btn->set_enabled(chip_detection_not_running);

    if(chip_id_task && chip_id_task->completed()) {
        FURI_LOG_I(
            TAG,
            "id task completed: succ %d, valid %d, id %d",
            chip_id_task->success,
            flash_info.valid,
            flash_info.vendor_id);

        if(chip_id_task->success && flash_info.valid) {
            if(!chip_detected) {
                chip_detected = true;
                process_found_chip();
            }

        } else {
            string_printf(chip_extra, "NOTHING FOUND");
        }

        chip_id_task.reset();
    }
    //} else {
    //    string_printf(chip_extra, "detecting...");
    //}

    status_line->update_text(string_get_cstr(chip_extra));
}

bool FlashManagerSceneChipID::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    FURI_LOG_I(TAG, "FlashManagerSceneChipID::on_event %d", event->type);

    switch(event->type) {
    case FlashManager::EventType::Tick:
        tick();
        break;
    case FlashManager::EventType::OpReadChip:
        app->scene_controller.switch_to_next_scene(
            FlashManager::SceneType::ReadImgFileNameInputScene);
        break;
    case FlashManager::EventType::OpWriteChip:
        app->scene_controller.switch_to_scene(FlashManager::SceneType::WriteImgProcessScene);
        break;
    case FlashManager::EventType::Back:
        if ((bool)chip_id_task) {
            // don't leave view while still detecting.
            consumed = true;
        }
    default:
        break;
    }

    return consumed;
}

void FlashManagerSceneChipID::process_found_chip() {
    // TODO: better format, vendor name, ...
    string_printf(
        chip_id,
        "%s %s %dK",
        spi_vendor_get_name(flash_info.vendor_id),
        flash_info.name,
        flash_info.size / 1024);
    FURI_LOG_I(TAG, "id: '%s'", string_get_cstr(chip_id));
    string_printf(chip_extra, "VID %x: %x b", flash_info.vendor_id, flash_info.size);
    FURI_LOG_I(TAG, "extra: '%s'", string_get_cstr(chip_extra));

    detail_line->update_text(string_get_cstr(chip_id));

    const char* next_operation = "Read";
    ButtonElementCallback callback;

    // we have a file name in text store -> next OP is write
    // TODO: warn on size mismatch
    if(strlen(app->text_store.text)) {
        next_operation = "Write";
        callback = &FlashManagerSceneChipID::write_chip_callback;
    } else {
        callback = &FlashManagerSceneChipID::read_chip_callback;
    }

    next_btn->set_type(ButtonElement::Type::Right, next_operation);
    next_btn->set_callback(this, callback);
    next_btn->set_enabled(true);
}

void FlashManagerSceneChipID::on_exit(FlashManager* app) {
    app->view_controller.get<ContainerVM>()->clean();
    string_clear(chip_id);
    string_clear(chip_extra);
}

void FlashManagerSceneChipID::read_chip_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpReadChip};
    reinterpret_cast<FlashManagerSceneChipID*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneChipID::write_chip_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpWriteChip};
    reinterpret_cast<FlashManagerSceneChipID*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneChipID::back_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Back};
    reinterpret_cast<FlashManagerSceneChipID*>(context)->app->view_controller.send_event(&event);
}
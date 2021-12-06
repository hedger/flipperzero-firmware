#include "flash_manager_scene_chip_id.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"
#include "../spi/spi_chips.h"

void FlashManagerSceneChipID::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;
    this->flash_info = app->get_flash_info();

    ContainerVM* container = app->view_controller;

    back_btn = container->add<ButtonElement>();
    back_btn->set_type(ButtonElement::Type::Left, "Back");
    back_btn->set_callback(this, &FlashManagerSceneChipID::back_callback);
    back_btn->set_enabled(false);

    scan_btn = container->add<ButtonElement>();
    scan_btn->set_type(ButtonElement::Type::Center, "Scan");
    scan_btn->set_callback(this, &FlashManagerSceneChipID::scan_callback);
    scan_btn->set_enabled(false);

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

    FlashManager::Event event{.type = FlashManager::EventType::Scan};
    app->view_controller.send_event(&event);

    chip_id_task = std::make_unique<WorkerTask>(
        WorkerOperation::ChipId, 0, reinterpret_cast<uint8_t*>(flash_info), sizeof(*flash_info));
    app->worker->enqueue_task(chip_id_task.get());
}

void FlashManagerSceneChipID::tick() {
    if(chip_id_task) {
        if(chip_id_task->completed()) {
            FURI_LOG_I(
                TAG,
                "id task completed: succ %d, valid %d, id %d",
                chip_id_task->success,
                flash_info->valid,
                flash_info->vendor_id);

            if(chip_id_task->success && flash_info->valid) {
                app->scene_controller.switch_to_scene(FlashManager::SceneType::ChipInfoScene);
            } else {
                status_line->update_text("NOT FOUND");
                scan_btn->set_enabled(true);
                back_btn->set_enabled(true);
            }

            chip_id_task.reset();
        } else {
            app->notify_green_blink();
        }
    }
}

bool FlashManagerSceneChipID::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    //FURI_LOG_I(TAG, "FlashManagerSceneChipID::on_event %d", event->type);

    switch(event->type) {
    case FlashManager::EventType::Scan:
        status_line->update_text("detecting...");
        scan_btn->set_enabled(false);
        back_btn->set_enabled(false);
        break;
    case FlashManager::EventType::Tick:
        tick();
        break;
    case FlashManager::EventType::Back:
        if((bool)chip_id_task) {
            // don't leave view while still detecting.
            consumed = true;
        }
    default:
        break;
    }

    return consumed;
}

void FlashManagerSceneChipID::on_exit(FlashManager* app) {
    app->view_controller.get<ContainerVM>()->clean();
    //chip_id_task.reset();
}

void FlashManagerSceneChipID::back_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Back};
    reinterpret_cast<FlashManagerSceneChipID*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneChipID::scan_callback(void* context) {
    reinterpret_cast<FlashManagerSceneChipID*>(context)->start_chip_id();
}
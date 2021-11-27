#include "flash_manager_scene_chip_id.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"

void FlashManagerSceneChipID::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;
    chip_detected = false;
    string_init(chip_id);

    ContainerVM* container = app->view_controller;

    auto button = container->add<ButtonElement>();
    button->set_type(ButtonElement::Type::Left, "Back");
    button->set_callback(
        app, cbc::obtain_connector(this, &FlashManagerSceneChipID::back_callback));

    auto line_1 = container->add<StringElement>();
    auto line_2 = container->add<StringElement>();
    status_line = container->add<StringElement>();

    line_1->set_text("Looking for SPI chip...", 64, 17, AlignCenter, AlignBottom, FontSecondary);
    line_2->set_text("Please be patient.", 64, 29, AlignCenter, AlignBottom, FontSecondary);
    status_line->set_text("...", 64, 41, AlignCenter, AlignBottom, FontSecondary);

    app->view_controller.switch_to<ContainerVM>();

    chip_id_task = std::make_unique<WorkerTask>(
        WorkerOperation::ChipId, 0, reinterpret_cast<uint8_t*>(&flash_info), sizeof(flash_info));

    app->worker->enqueue_task(chip_id_task.get());
}

void FlashManagerSceneChipID::tick() {
    if(chip_id_task->completed()) {
        if(chip_id_task->success && flash_info.valid) {
            if(!chip_detected) {
                chip_detected = true;
                process_found_chip();
            }

        } else {
            string_printf(chip_id, "NOTHING FOUND");
        }
    } else {
        string_printf(chip_id, "detecting...");
    }

    status_line->update_text(string_get_cstr(chip_id));
}

bool FlashManagerSceneChipID::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    switch(event->type) {
    case FlashManager::EventType::Tick:
        tick();
        break;
    case FlashManager::EventType::OpReadChip:
        app->scene_controller.switch_to_next_scene(
            FlashManager::SceneType::ReadImgFileNameInputScene);
        break;
    case FlashManager::EventType::OpWriteChip:
        app->scene_controller.switch_to_scene(
            FlashManager::SceneType::WriteImgProcessScene);
        break;
    default:
        break;
    }

    return consumed;
}

void FlashManagerSceneChipID::process_found_chip() {
    // TODO: better format, vendor name, ...
    string_printf(chip_id, "VID %x: %x b", flash_info.vendor_id, flash_info.size);

    ContainerVM* container = app->view_controller;
    auto button = container->add<ButtonElement>();

    const char* next_operation = "Read";
    ButtonElementCallback callback;

    // we have a file name in text store -> next OP is write
    // TODO: warn on size mismatch
    if (strlen(app->text_store.text)) {
        next_operation = "Write";
        callback = cbc::obtain_connector(this, &FlashManagerSceneChipID::write_chip_callback);
    } else {
        callback = cbc::obtain_connector(this, &FlashManagerSceneChipID::read_chip_callback);
    }

    button->set_type(ButtonElement::Type::Right, next_operation);
    button->set_callback(app, callback);
}

void FlashManagerSceneChipID::on_exit(FlashManager* app) {
    app->view_controller.get<ContainerVM>()->clean();
    string_clear(chip_id);
}

void FlashManagerSceneChipID::read_chip_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpReadChip};
    app->view_controller.send_event(&event);
}

void FlashManagerSceneChipID::write_chip_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpWriteChip};
    app->view_controller.send_event(&event);
}

void FlashManagerSceneChipID::back_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Back};
    app->view_controller.send_event(&event);
}
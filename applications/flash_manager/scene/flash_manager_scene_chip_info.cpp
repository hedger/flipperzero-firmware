#include "flash_manager_scene_chip_info.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../spi/spi_chips.h"

void FlashManagerSceneChipInfo::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;

    string_init(chip_id);
    string_init(chip_extra);

    auto flash_info = app->get_flash_info();

    string_printf(
        chip_id,
        "%s %s %dK",
        spi_vendor_get_name(flash_info->vendor_id),
        flash_info->name,
        flash_info->size / 1024);
    string_printf(
        chip_extra,
        "VID %02X %02X %02X",
        flash_info->vendor_id,
        flash_info->type_id,
        flash_info->capacity_id);

    FURI_LOG_I(TAG, "id: '%s'", string_get_cstr(chip_id));
    FURI_LOG_I(TAG, "extra: '%s'", string_get_cstr(chip_extra));

    ContainerVM* container = app->view_controller;

    // Retry Button
    retry_btn = container->add<ButtonElement>();
    retry_btn->set_type(ButtonElement::Type::Left, "Retry");
    retry_btn->set_callback(this, &FlashManagerSceneChipInfo::retry_callback);
    retry_btn->set_enabled(true);

    // Next/Write Button
    // TODO: move "Write" action to "readed-menu" scene
    const char* next_op = "Next";
    ButtonElementCallback next_cb = FlashManagerSceneChipInfo::next_callback;
    verify_btn = nullptr;

    if(app->run_in_app_mode) {
        next_op = "Write";
        next_cb = &FlashManagerSceneChipInfo::write_callback;

        verify_btn = container->add<ButtonElement>();
        verify_btn->set_type(ButtonElement::Type::Center, "Verify");
        verify_btn->set_callback(this, &FlashManagerSceneChipInfo::verify_callback);
        verify_btn->set_enabled(true);
    }

    next_btn = container->add<ButtonElement>();
    next_btn->set_type(ButtonElement::Type::Right, next_op);
    next_btn->set_callback(this, next_cb);
    next_btn->set_enabled(true);

    header_line = container->add<StringElement>();
    chip_id_line = container->add<StringElement>();
    chip_extra_line = container->add<StringElement>();

    header_line->set_text("Detected SPI chip:", 64, 17, AlignCenter, AlignBottom, FontSecondary);
    chip_id_line->set_text(
        string_get_cstr(chip_id), 64, 29, AlignCenter, AlignBottom, FontSecondary);
    chip_extra_line->set_text(
        string_get_cstr(chip_extra), 64, 41, AlignCenter, AlignBottom, FontSecondary);

    app->view_controller.switch_to<ContainerVM>();
}

bool FlashManagerSceneChipInfo::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    //FURI_LOG_I(TAG, "FlashManagerSceneChipInfo::on_event %d", event->type);

    switch(event->type) {
    case FlashManager::EventType::Back:
        // TODO: fix me
        if(app->run_in_app_mode) {
            app->scene_controller.switch_to_previous_scene();
            //consumed = true;
        } else {
            app->scene_controller.search_and_switch_to_previous_scene(
                {FlashManager::SceneType::Start});
            consumed = true;
        }
        break;
    case FlashManager::EventType::Retry:
        // TODO: fix me
        if(app->run_in_app_mode) {
            app->scene_controller.switch_to_scene(FlashManager::SceneType::Start);
        } else {
            app->scene_controller.switch_to_scene(FlashManager::SceneType::ChipIDScene);
        }
        break;
    case FlashManager::EventType::OpReadChip:
        app->scene_controller.switch_to_next_scene(
            FlashManager::SceneType::ReadImgFileNameInputScene);
        break;
    case FlashManager::EventType::OpWriteChip:
        app->scene_controller.switch_to_scene(FlashManager::SceneType::WriteImgProcessScene);
        break;
    case FlashManager::EventType::OpVerifyChip:
        app->runVerification = true;
        app->scene_controller.switch_to_scene(FlashManager::SceneType::ReadImgProcessScene);
        break;
    default:
        break;
    }

    return consumed;
}

void FlashManagerSceneChipInfo::on_exit(FlashManager* app) {
    app->view_controller.get<ContainerVM>()->clean();
    string_clear(chip_id);
    string_clear(chip_extra);
}

void FlashManagerSceneChipInfo::next_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpReadChip};
    reinterpret_cast<FlashManagerSceneChipInfo*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneChipInfo::write_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpWriteChip};
    reinterpret_cast<FlashManagerSceneChipInfo*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneChipInfo::verify_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::OpVerifyChip};
    reinterpret_cast<FlashManagerSceneChipInfo*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneChipInfo::retry_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Retry};
    reinterpret_cast<FlashManagerSceneChipInfo*>(context)->app->view_controller.send_event(&event);
}
#include "flash_manager_scene_read_dump.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"

void FlashManagerSceneReadDump::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;

    bytes_read = 0;
    string_init(status_text);
    read_buffer = std::make_unique<uint8_t[]>(DUMP_READ_BLOCK_BYTES);

    ContainerVM* container = app->view_controller;
    //auto callback = cbc::obtain_connector(this, &FlashManagerSceneReadDump::result_callback);

    container->add<StringElement>();

    auto button = container->add<ButtonElement>();
    button->set_type(ButtonElement::Type::Left, "Back");
    button->set_callback(
        app, cbc::obtain_connector(this, &FlashManagerSceneReadDump::back_callback));

    auto line_1 = container->add<StringElement>();
    auto line_2 = container->add<StringElement>();
    status_line = container->add<StringElement>();

    line_1->set_text("Reading dump...", 64, 17, AlignCenter, AlignBottom, FontSecondary);
    line_2->set_text("Please be patient.", 64, 29, AlignCenter, AlignBottom, FontSecondary);
    status_line->set_text("...", 64, 41, AlignCenter, AlignBottom, FontSecondary);

    app->view_controller.switch_to<ContainerVM>();
}

bool FlashManagerSceneReadDump::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    //if(event->type == FlashManager::EventType::ByteEditResult) {
    //    app->scene_controller.switch_to_previous_scene();
    //    consumed = true;
    //}

    if(event->type == FlashManager::EventType::Tick) {
        tick();
    } else if(event->type == FlashManager::EventType::Next) {
        app->scene_controller.switch_to_next_scene(FlashManager::SceneType::Start);
    }

    return consumed;
}

void FlashManagerSceneReadDump::tick() {
    //if (chip_id_task->completed()) {
    //    if (chip_id_task->success && flash_info.valid) {
    //        string_printf(chip_id, "Read failed :(");
    //    } else {
    //        string_printf(chip_id, "Read failed :(");
    //    }
    //} else {
    //    string_printf(chip_id, "detecting...");
    //}
    const SpiFlashInfo_t* flash = app->worker->toolkit->get_info();
    furi_assert(flash && flash->valid);

    if(!reader_task) {
        enqueue_next_block();
    }

    if(bytes_read >= flash->size) {
        ContainerVM* container = app->view_controller;
        auto button = container->add<ButtonElement>();
        button->set_type(ButtonElement::Type::Right, "Done");
        button->set_callback(
            app, cbc::obtain_connector(this, &FlashManagerSceneReadDump::done_callback));
    }

    if(reader_task->completed()) {
        if(reader_task->success) {
            // TODO: write 'read_buffer' to file
            bytes_read += reader_task->size;
            if(bytes_read < flash->size) {
                enqueue_next_block();
            }
        }
    } else {
        // TODO: update intermediate progress
    }

    //FURI_LOG_I(TAG, "progress: %d of %d", bytes_read, flash->size);

    uint32_t progress = bytes_read * 100 / flash->size;
    string_printf(status_text, "%d%% done", progress);
    status_line->update_text(string_get_cstr(status_text));
}

bool FlashManagerSceneReadDump::enqueue_next_block() {
    FURI_LOG_I(TAG, "enqueue_next_block: read %d", bytes_read);
    // TODO: fix tail
    size_t block_size = DUMP_READ_BLOCK_BYTES;

    reader_task = std::make_unique<WorkerTask>(
        WorkerOperation::BlockRead, bytes_read, read_buffer.get(), block_size);

    // TODO: check result
    FURI_LOG_I(TAG, "enqueue_next_block: adding");
    return app->worker->enqueue_task(reader_task.get());
}

void FlashManagerSceneReadDump::process_found_chip() {
    //string_printf(status_text, "VID %x: %x b", flash_info.vendor_id, flash_info.size);

    ContainerVM* container = app->view_controller;
    auto button = container->add<ButtonElement>();
    button->set_type(ButtonElement::Type::Right, "Read");
    button->set_callback(
        app, cbc::obtain_connector(this, &FlashManagerSceneReadDump::done_callback));
}

void FlashManagerSceneReadDump::on_exit(FlashManager* app) {
    app->view_controller.get<ContainerVM>()->clean();
    string_clear(status_text);
    read_buffer.reset();
}

void FlashManagerSceneReadDump::done_callback(void* context) {
    FlashManager* app = static_cast<FlashManager*>(context);
    FlashManager::Event event;

    event.type = FlashManager::EventType::Next;
    app->view_controller.send_event(&event);
}

void FlashManagerSceneReadDump::back_callback(void* context) {
    FlashManager* app = static_cast<FlashManager*>(context);
    FlashManager::Event event;

    event.type = FlashManager::EventType::Back;
    app->view_controller.send_event(&event);
}
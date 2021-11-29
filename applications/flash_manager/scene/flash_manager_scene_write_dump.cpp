#include "flash_manager_scene_write_dump.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"

void FlashManagerSceneWriteDump::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;

    bytes_written = 0;
    write_completed = false;
    cancelled = false;

    string_init(status_text);
    write_buffer = std::make_unique<uint8_t[]>(DUMP_READ_BLOCK_BYTES);

    ContainerVM* container = app->view_controller;

    cancel_button = container->add<ButtonElement>();
    cancel_button->set_type(ButtonElement::Type::Left, "Cancel");
    cancel_button->set_callback(
        app, cbc::obtain_connector(this, &FlashManagerSceneWriteDump::cancel_callback));

    header_line = container->add<StringElement>();
    auto line_2 = container->add<StringElement>();
    status_line = container->add<StringElement>();

    header_line->set_text("Erasing chip...", 64, 17, AlignCenter, AlignBottom, FontSecondary);
    line_2->set_text("Please be patient.", 64, 29, AlignCenter, AlignBottom, FontSecondary);
    status_line->set_text("...", 64, 41, AlignCenter, AlignBottom, FontSecondary);

    // TODO: error check, empty check
    app->file_tools.open_dump_file_read(app->text_store.text, ChipType::SPI);
    FURI_LOG_I(TAG, "file size to write: %d", app->file_tools.get_size());

    app->view_controller.switch_to<ContainerVM>();
}

bool FlashManagerSceneWriteDump::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    switch(event->type) {
    case FlashManager::EventType::Tick:
        tick();
        break;
    case FlashManager::EventType::Back:
        if (!write_completed || !cancelled) {
            // no going back!
            consumed = true;
        }
        break;
    case FlashManager::EventType::Cancel:
    case FlashManager::EventType::Next:
        app->scene_controller.search_and_switch_to_previous_scene(
            {FlashManager::SceneType::Exit});
        break;
    default:
        break;
    }

    return consumed;
}

void FlashManagerSceneWriteDump::finish_write() {
    if(!write_completed) {
        write_completed = true;

        app->file_tools.close();

        ContainerVM* container = app->view_controller;
        auto button = container->add<ButtonElement>();
        button->set_type(ButtonElement::Type::Right, "Done");
        button->set_callback(
            app, cbc::obtain_connector(this, &FlashManagerSceneWriteDump::done_callback));
        cancel_button->set_enabled(false);
    }
}

void FlashManagerSceneWriteDump::tick() {
    if(cancelled) {
        return;
    }

    const SpiFlashInfo_t* flash = app->worker->toolkit->get_info();
    furi_assert(flash && flash->valid);

    if(!writer_task) {
        writer_task = std::make_unique<WorkerTask>(WorkerOperation::ChipErase);
        app->worker->enqueue_task(writer_task.get());
        return;
    }

    if(bytes_written >= flash->size) {
        finish_write();
    }

    if(write_completed) {
        return;
    }

    uint32_t progress = 0;

    if(writer_task->completed()) {
        header_line->update_text("Writing chip...");
        if(writer_task->success) {
            // TODO: check result
            app->file_tools.read_buffer(writer_task->data, writer_task->size);
            bytes_written += writer_task->size;
            if(bytes_written < flash->size) {
                enqueue_next_block();
            }
        }
        progress = bytes_written * 100 / flash->size;
    } else {
        progress = (bytes_written + (writer_task->progress * writer_task->size / 100)) * 100 /
                   flash->size;
    }

    string_printf(status_text, "%d%% done", progress);
    status_line->update_text(string_get_cstr(status_text));
}

bool FlashManagerSceneWriteDump::enqueue_next_block() {
    FURI_LOG_I(TAG, "enqueue_next_block: written %d", bytes_written);
    // TODO: fix tail
    size_t block_size = DUMP_READ_BLOCK_BYTES;

    writer_task = std::make_unique<WorkerTask>(
        WorkerOperation::BlockWrite, bytes_written, write_buffer.get(), block_size);

    // TODO: check result
    FURI_LOG_I(TAG, "enqueue_next_block: adding");
    return app->worker->enqueue_task(writer_task.get());
}

void FlashManagerSceneWriteDump::on_exit(FlashManager* app) {
    app->file_tools.close();
    app->view_controller.get<ContainerVM>()->clean();
    string_clear(status_text);
    app->text_store.set("");
    write_buffer.reset();
}

void FlashManagerSceneWriteDump::done_callback(void* context) {
    FlashManager::Event event{.type = FlashManager::EventType::Next};
    app->view_controller.send_event(&event);
}

void FlashManagerSceneWriteDump::cancel_callback(void* context) {
    cancelled = true;
    FlashManager::Event event{.type = FlashManager::EventType::Cancel};
    app->view_controller.send_event(&event);
}
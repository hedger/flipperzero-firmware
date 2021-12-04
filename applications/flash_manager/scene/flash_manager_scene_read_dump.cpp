#include "flash_manager_scene_read_dump.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"

void FlashManagerSceneReadDump::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;

    bytes_read = 0;
    read_completed = false;
    cancelled = false;

    string_init(detail_text);
    string_init(status_text);
    read_buffer = std::make_unique<uint8_t[]>(DUMP_READ_BLOCK_BYTES);

    ContainerVM* container = app->view_controller;

    cancel_button = container->add<ButtonElement>();
    cancel_button->set_type(ButtonElement::Type::Left, "Cancel");
    cancel_button->set_callback(this, &FlashManagerSceneReadDump::cancel_callback);

    auto line_1 = container->add<StringElement>();
    detail_line = container->add<StringElement>();
    status_line = container->add<StringElement>();

    string_printf(status_text, "Please be patient.");

    const char* operationHintText = app->runVerification ? "Verifying..." : "Reading dump...";
    FURI_LOG_I(TAG, "hint: %s, file: '%s'", operationHintText, app->text_store.text);

    line_1->set_text(operationHintText, 64, 17, AlignCenter, AlignBottom, FontSecondary);
    detail_line->set_text("", 64, 29, AlignCenter, AlignBottom, FontSecondary);
    status_line->set_text("...", 64, 41, AlignCenter, AlignBottom, FontSecondary);

    // TODO: error check, empty check
    if(app->runVerification) {
        app->file_tools.open_dump_file_read(app->text_store.text, ChipType::SPI);
        verification_buffer = std::make_unique<uint8_t[]>(DUMP_READ_BLOCK_BYTES);
    } else {
        app->file_tools.open_dump_file_write(app->text_store.text, ChipType::SPI);
    }

    app->view_controller.switch_to<ContainerVM>();
}

bool FlashManagerSceneReadDump::on_event(FlashManager* app, FlashManager::Event* event) {
    bool consumed = false;

    switch(event->type) {
    case FlashManager::EventType::Tick:
        tick();
        break;
    case FlashManager::EventType::Back:
        if(!read_completed && !cancelled) {
            // no going back!
            consumed = true;
            break;
        }

    case FlashManager::EventType::Cancel:
    case FlashManager::EventType::Next:
        if(!app->run_in_app_mode) {
            app->scene_controller.search_and_switch_to_previous_scene(
                {FlashManager::SceneType::Start});
        } else {
            app->scene_controller.switch_to_previous_scene();
        }
        break;
    default:
        break;
    }

    return consumed;
}

void FlashManagerSceneReadDump::finish_read() {
    if(!read_completed) {
        read_completed = true;

        ContainerVM* container = app->view_controller;
        auto button = container->add<ButtonElement>();
        button->set_type(ButtonElement::Type::Right, "Exit");
        button->set_callback(this, &FlashManagerSceneReadDump::done_callback);
        cancel_button->set_enabled(false);
    }
}

void FlashManagerSceneReadDump::tick() {
    if(cancelled) {
        return;
    }

    const SpiFlashInfo_t* flash = app->worker->toolkit->get_info();
    furi_assert(flash && flash->valid);

    if(!reader_task) {
        enqueue_next_block();
    }

    if(bytes_read >= flash->size) {
        finish_read();
    }

    if(read_completed) {
        return;
    }

    uint32_t progress = 0;

    if(reader_task->completed()) {
        if(reader_task->success) {
            if(app->runVerification) {
                app->file_tools.read_buffer(verification_buffer.get(), reader_task->size);
                if(!memcmp(verification_buffer.get(), reader_task->data, reader_task->size)) {
                    FURI_LOG_I(TAG, "verify: block @%x OK", reader_task->offset);
                    string_printf(
                        detail_text, "Block %x+%x OK", reader_task->offset, reader_task->size);
                } else {
                    FURI_LOG_I(TAG, "verify: block @%x MISMATCHED");
                    string_printf(
                        detail_text, "Block %x+%x FAILED!", reader_task->offset, reader_task->size);
                    // TODO: break
                }
            } else {
                app->file_tools.write_buffer(reader_task->data, reader_task->size);
            }
            bytes_read += reader_task->size;
            if(bytes_read < flash->size) {
                enqueue_next_block();
            }
        } else {
            // TODO
        }
        progress = bytes_read * 100 / flash->size;
    } else {
        progress =
            (bytes_read + (reader_task->progress * reader_task->size / 100)) * 100 / flash->size;
    }

    detail_line->update_text(string_get_cstr(detail_text));
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

void FlashManagerSceneReadDump::on_exit(FlashManager* app) {
    app->file_tools.close();
    if(cancelled && !app->runVerification) {
        FURI_LOG_I(TAG, "removing unfinished dump '%s'", app->text_store.text);
        app->file_tools.remove_dump_file(app->text_store.text, ChipType::SPI);
    }

    app->view_controller.get<ContainerVM>()->clean();
    string_clear(detail_text);
    string_clear(status_text);
    app->text_store.set("");
    read_buffer.reset();
    app->runVerification = false;
}

void FlashManagerSceneReadDump::done_callback(void* context) {
    FlashManagerSceneReadDump* thisScene = reinterpret_cast<FlashManagerSceneReadDump*>(context);

    FlashManager::Event event{
        .type = thisScene->app->runVerification ? FlashManager::EventType::Back :
                                                  FlashManager::EventType::Next};
    reinterpret_cast<FlashManagerSceneReadDump*>(context)->app->view_controller.send_event(&event);
}

void FlashManagerSceneReadDump::cancel_callback(void* context) {
    FlashManagerSceneReadDump* thisScene = reinterpret_cast<FlashManagerSceneReadDump*>(context);
    thisScene->cancelled = true;

    FlashManager::Event event{.type = FlashManager::EventType::Cancel};
    thisScene->app->view_controller.send_event(&event);
}
#include "flash_manager_scene_read_dump.h"

#include "../../lfrfid/view/elements/button-element.h"
#include "../../lfrfid/view/elements/icon-element.h"
#include "../../lfrfid/view/elements/string-element.h"

#include "../flash_manager_worker.h"

void FlashManagerSceneReadDump::on_enter(FlashManager* app, bool need_restore) {
    this->app = app;

    bytes_read = 0;
    bytes_queued = 0;
    verification_file_size = 0;
    read_completed = false;
    cancelled = false;

    string_init(detail_text);
    string_init(status_text);

    for(int i = 0; i < TASK_DEPTH; ++i) {
        read_buffers[i] = std::make_unique<uint8_t[]>(DUMP_READ_BLOCK_BYTES);
    }

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
        verification_file_size = app->file_tools.get_size();
        if(!verification_file_size) {
            // TODO: warn/err?
        }
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

        if(!cancelled) {
            app->scene_controller.switch_to_scene(FlashManager::SceneType::ShowSuccess);
        }
    }
}

void FlashManagerSceneReadDump::tick() {
    if(cancelled) {
        return;
    }

    const SpiFlashInfo_t* flash = app->worker->toolkit->get_info();
    furi_assert(flash && flash->valid);

    if(bytes_read >= get_job_size()) {
        finish_read();
    }

    if(read_completed) {
        return;
    }

    check_tasks_update_progress();
}

void FlashManagerSceneReadDump::check_tasks_update_progress() {
    uint32_t progress = 0;

    for(int i = 0; i < TASK_DEPTH; ++i) {
        check_task_state(reader_tasks[i]);
    }

    //header_line->update_text("Writing chip...");
    progress = bytes_read * 100 / get_job_size();

    detail_line->update_text(string_get_cstr(detail_text));
    string_printf(status_text, "%d%% done", progress);
    status_line->update_text(string_get_cstr(status_text));

    if(app->runVerification) {
        app->notify_green_blink();
    } else {
        app->notify_yellow_blink();
    }
}

bool FlashManagerSceneReadDump::check_task_state(std::unique_ptr<WorkerTask>& task) {
    if(!(bool)task) {
        return enqueue_next_block();
    }

    if(task->completed()) {
        if(app->runVerification) {
            app->file_tools.seek(task->offset);
            app->file_tools.read_buffer(verification_buffer.get(), task->size);
            if(!memcmp(verification_buffer.get(), task->data, task->size)) {
                FURI_LOG_I(TAG, "verify: block @%x OK", task->offset);
                string_printf(detail_text, "Block %x+%x OK", task->offset, task->size);
            } else {
                FURI_LOG_I(TAG, "verify: block @%x MISMATCHED");
                string_printf(detail_text, "Block %x+%x FAILED!", task->offset, task->size);
                // TODO: break
            }
        } else {
            app->file_tools.seek(task->offset);
            app->file_tools.write_buffer(task->data, task->size);
        }

        if(task->success) {
            bytes_read += task->size;
            return enqueue_next_block();
        } else {
            cancelled = true;
            status_line->update_text("FAILED :(");
            finish_read();
            return false;
        }
    }
    return false;
}

bool FlashManagerSceneReadDump::enqueue_next_block() {
    if(bytes_queued >= get_job_size()) {
        return false;
    }

    FURI_LOG_I(TAG, "enqueue_next_block: read %d, queued %d", bytes_read, bytes_queued);
    // TODO: fix tail
    size_t block_size = DUMP_READ_BLOCK_BYTES;

    bool free_task_found = false;
    int free_task_id = 0;
    for(int i = 0; i < TASK_DEPTH; ++i) {
        auto& task = reader_tasks[i];

        if(!task || (task && task->completed())) {
            free_task_id = i;
            free_task_found = true;
            break;
        }
    }

    if(!free_task_found) {
        return false;
    }

    furi_assert(free_task_id < TASK_DEPTH);

    auto& target_task = reader_tasks[free_task_id];
    auto& target_buffer = read_buffers[free_task_id];

    target_task = std::make_unique<WorkerTask>(
        WorkerOperation::BlockRead, bytes_queued, target_buffer.get(), block_size);
    FURI_LOG_I(
        TAG,
        "enqueue_next_block(): put block from file @ %x + %x to queue task %d to flash @ %x",
        bytes_queued,
        block_size,
        free_task_id,
        bytes_queued);

    bytes_queued += block_size;
    // TODO: check result
    FURI_LOG_I(TAG, "enqueue_next_block: adding");
    return app->worker->enqueue_task(target_task.get());
}

size_t FlashManagerSceneReadDump::get_job_size() const {
    return verification_file_size ? verification_file_size :
                                    app->worker->toolkit->get_info()->size;
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
    for(int i = 0; i < TASK_DEPTH; ++i) {
        read_buffers[i].reset();
    }
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
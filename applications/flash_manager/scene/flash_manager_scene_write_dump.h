#pragma once
#include "../flash_manager.h"
#include "../spi/spi_toolkit.h"

class StringElement;
class ButtonElement;
class WorkerTask;

class FlashManagerSceneWriteDump : public GenericScene<FlashManager> {
    static const size_t DUMP_WRITE_BLOCK_BYTES = 4 * 1024;

public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    FlashManager* app;

    void finish_write();
    void tick();
    bool enqueue_next_block();
    void check_tasks_update_progress();
    bool check_task_state(std::unique_ptr<WorkerTask>& task);

    //void result_callback(void* context);
    static void cancel_callback(void* context);
    static void verify_callback(void* context);
    static void done_callback(void* context);

    StringElement* header_line;
    StringElement* status_line;
    ButtonElement *cancel_btn, *run_verification_btn;

    bool write_completed;
    bool cancelled;
    string_t status_text;

    size_t bytes_written, bytes_queued, write_to_chip_size;

    static const int TASK_DEPTH = 3;
    std::unique_ptr<WorkerTask> writer_tasks[TASK_DEPTH];
    std::unique_ptr<uint8_t[]> write_buffers[TASK_DEPTH];
};

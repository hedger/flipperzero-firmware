#pragma once
#include "../flash_manager.h"
#include "../spi/spi_toolkit.h"

class ButtonElement;
class StringElement;
class WorkerTask;
class ProgressBarElement;

class FlashManagerSceneReadDump : public GenericScene<FlashManager> {
    static const size_t DUMP_READ_BLOCK_BYTES = 8 * 1024;

public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    FlashManager* app;

    void finish_read(bool cancelled);
    void tick();

    void check_tasks_update_progress();
    bool enqueue_next_block(size_t prev_block_size);
    bool check_task_state(std::unique_ptr<WorkerTask>& task);

    size_t get_job_size() const;

    //void result_callback(void* context);
    static void cancel_callback(void* context);
    static void done_callback(void* context);

    StringElement *detail_line, *status_line;
    ProgressBarElement* progress;
    ButtonElement* cancel_button;

    bool read_completed;
    bool cancelled;
    string_t detail_text, status_text;
    size_t bytes_read, bytes_queued, verification_file_size;

    static const int TASK_DEPTH = 3;
    std::unique_ptr<WorkerTask> reader_tasks[TASK_DEPTH];
    std::unique_ptr<uint8_t[]> read_buffers[TASK_DEPTH];
    //std::unique_ptr<WorkerTask> reader_task;
    std::unique_ptr<uint8_t[]> verification_buffer;
};
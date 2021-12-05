#pragma once
#include "../flash_manager.h"
#include "../spi/spi_toolkit.h"

class ButtonElement;
class StringElement;
class WorkerTask;

class FlashManagerSceneReadDump : public GenericScene<FlashManager> {
    static const size_t DUMP_READ_BLOCK_BYTES = 8 * 1024;

public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    FlashManager* app;

    void finish_read();
    void tick();
    bool enqueue_next_block();
    size_t get_job_size() const;

    //void result_callback(void* context);
    static void cancel_callback(void* context);
    static void done_callback(void* context);

    StringElement *detail_line, *status_line;
    ButtonElement* cancel_button;

    bool read_completed;
    bool cancelled;
    string_t detail_text, status_text;
    size_t bytes_read, verification_file_size;
    std::unique_ptr<WorkerTask> reader_task;
    std::unique_ptr<uint8_t[]> read_buffer, verification_buffer;
};
#pragma once
#include "../flash_manager.h"
#include "../spi/spi_api.h"

class StringElement;
class WorkerTask;

class FlashManagerSceneReadDump : public GenericScene<FlashManager> {
    static const size_t DUMP_READ_BLOCK_BYTES = 4 * 1024;
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    FlashManager* app;

    void process_found_chip();
    void tick();
    bool enqueue_next_block();

    //void result_callback(void* context);
    void back_callback(void* context);
    void done_callback(void* context);

    StringElement* status_line;

    string_t status_text;
    size_t bytes_read;
    std::unique_ptr<WorkerTask> reader_task;
    std::unique_ptr<uint8_t[]> read_buffer;
};
#pragma once
#include "../flash_manager.h"
#include "../spi/spi_api.h"

class StringElement;
class WorkerTask;

class FlashManagerSceneChipID : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    void process_found_chip(FlashManager* app);

    //void result_callback(void* context);
    void back_callback(void* context);
    void read_chip_callback(void* context);

    StringElement* status_line;

    string_t chip_id;
    SpiFlashInfo_t flash_info;
    size_t chip_size;

    std::unique_ptr<WorkerTask> chip_id_task;
};
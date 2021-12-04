#pragma once
#include "../flash_manager.h"
#include "../spi/spi_toolkit.h"

class ButtonElement;
class StringElement;
class WorkerTask;

class FlashManagerSceneChipID : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

    void start_chip_id();

private:
    FlashManager* app;
    SpiFlashInfo_t* flash_info;

    void tick();
    static void back_callback(void* context);
    static void scan_callback(void* context);

    ButtonElement *back_btn, *scan_btn;
    StringElement *header_line, *detail_line, *status_line;

    std::unique_ptr<WorkerTask> chip_id_task;
};
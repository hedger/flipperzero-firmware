#pragma once
#include "../flash_manager.h"
#include "../spi/spi_api.h"

class ButtonElement;
class StringElement;
class WorkerTask;

class FlashManagerSceneChipID : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

    void start_chip_id(void* = nullptr);
private:
    FlashManager* app;
    void process_found_chip();

    //void result_callback(void* context);
    void tick();
    void rescan_callback(void* context);
    void back_callback(void* context);
    void read_chip_callback(void* context);
    void write_chip_callback(void* context);

    ButtonElement* run_detect_btn;
    StringElement* header_line;
    StringElement* detail_line;
    StringElement* status_line;

    bool chip_detected;
    string_t chip_id, chip_extra;
    SpiFlashInfo_t flash_info;
    size_t chip_size;

    std::unique_ptr<WorkerTask> chip_id_task;
};
#pragma once
#include "../flash_manager.h"
#include "../spi/spi_toolkit.h"

class ButtonElement;
class StringElement;
class WorkerTask;

class FlashManagerSceneChipInfo : public GenericScene<FlashManager> {
public:
    void on_enter(FlashManager* app, bool need_restore) final;
    bool on_event(FlashManager* app, FlashManager::Event* event) final;
    void on_exit(FlashManager* app) final;

private:
    FlashManager* app;

    static void next_callback(void* context);
    static void write_callback(void* context);
    static void retry_callback(void* context);

    ButtonElement *retry_btn, *next_btn;
    StringElement *header_line, *chip_id_line, *chip_extra_line;

    string_t chip_id, chip_extra;
};
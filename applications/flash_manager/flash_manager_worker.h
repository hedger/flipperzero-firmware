#pragma once
#include <furi.h>
#include <furi-hal.h>
#include <memory>

class SpiToolkit;

class FlashManagerWorker {
public:
    FlashManagerWorker();
    ~FlashManagerWorker();

    void start();
    void stop();

    volatile bool worker_running;
    FuriThread* thread;
    std::unique_ptr<SpiToolkit> toolkit;
};

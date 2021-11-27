#pragma once

#include <stddef.h>
#include <stdint.h>

#include "../flash_manager_worker.h"

class TaskExecutor {
public:
    TaskExecutor(SpiToolkit* _toolkit);
    // NOT thread-safe!
    void run(WorkerTask* task);

private:
    void run_chip_erase(WorkerTask* task);
    void run_block_read(WorkerTask* task);
    void run_block_write(WorkerTask* task);

    SpiToolkit* toolkit;
};
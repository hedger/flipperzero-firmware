#include "flash_manager_worker.h"

#define TAG "FlashWorker"


static int32_t flash_manager_worker_thread(void* context);

FlashManagerWorker::FlashManagerWorker() : worker_running(true) {
    FURI_LOG_I(TAG, "ctor()");
    thread = furi_thread_alloc();
    furi_thread_set_name(thread, "FlashManagerWorker");
    furi_thread_set_stack_size(thread, 1024);
    furi_thread_set_context(thread, this);
    furi_thread_set_callback(thread, flash_manager_worker_thread); 
}

void FlashManagerWorker::start() {
  furi_thread_start(thread);
}


void FlashManagerWorker::stop() {
    FURI_LOG_I(TAG, "worker stop flag set");
    worker_running = false;
    furi_thread_join(thread);
    FURI_LOG_I(TAG, "join() done");
}

FlashManagerWorker::~FlashManagerWorker() {
    furi_assert(thread);
    furi_thread_free(thread);
    FURI_LOG_I(TAG, "dtor() done");
}


/** Worker thread
 * 
 * @param context 
 * @return exit code 
 */ 
static int32_t flash_manager_worker_thread(void* context) {
    FlashManagerWorker* instance = static_cast<FlashManagerWorker*>(context);

    while (instance->worker_running) {
        osDelay(2000);
        FURI_LOG_I(TAG, "worker is alive");
    }

    FURI_LOG_I(TAG, "worker is done");
    return 0;
}
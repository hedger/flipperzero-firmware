#include "flash_manager_worker.h"
#include "spi/spi_api.h"
#include <furi.h>

#define TAG "FlashWorker"

static int32_t flash_manager_worker_thread(void *context);

WorkerTask::WorkerTask() : WorkerTask(WorkerOperation::Unknown, 0, nullptr, 0) {
}

WorkerTask::WorkerTask(WorkerOperation _operation, size_t _offset, uint8_t* _data, size_t _size) 
  : offset(_offset), data(_data), size(_size), operation(_operation), progress(0), success(false) {
}

FlashManagerWorker::FlashManagerWorker()
    : toolkit(std::make_unique<SpiToolkit>()),
      worker_running(false), task_progress(0) {

    FURI_LOG_I(TAG, "ctor()");

    

    thread = furi_thread_alloc();
    furi_thread_set_name(thread, "FlashManagerWorker");
    furi_thread_set_stack_size(thread, 1024);
    furi_thread_set_context(thread, this);
    furi_thread_set_callback(thread, flash_manager_worker_thread);
}

void FlashManagerWorker::start() {
    furi_thread_start(thread);
    worker_running = true;
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

bool FlashManagerWorker::enqueue_task(WorkerTask* task) {
    furi_assert(task);
    furi_assert(task->operation > WorkerOperation::None);

    if (is_busy()) {
        return false;
    }

    active_task = task;
    return true;
}

bool FlashManagerWorker::is_busy() const {
    return (active_task != nullptr && !active_task->completed());
}

/** Worker thread
 *
 * @param context
 * @return exit code
 */
static int32_t flash_manager_worker_thread(void *context) {
    FlashManagerWorker* instance = static_cast<FlashManagerWorker*>(context);
    SpiToolkit* pToolkit = instance->toolkit.get();

    while (instance->worker_running) {
      WorkerTask* pTask = instance->active_task;
  
      if ((pTask == nullptr) || pTask->operation <= WorkerOperation::None) {
          osDelay(2000);
          FURI_LOG_I(TAG, "worker is alive and has nothing to do");
          continue;
      }

      FURI_LOG_I(TAG, "running operation %d", pTask->operation);
      pTask->progress = 0;
      const size_t final_offs = pTask->offset + pTask->size;

      switch (pTask->operation) {
          case WorkerOperation::ChipId:
            pTask->success = pToolkit->detect_flash();
            // TODO: implement
            break;

          case WorkerOperation::ChipErase:
            // TODO: implement
            pTask->success = pToolkit->chip_erase();
            break;

          case WorkerOperation::BlockRead:
            // TODO: implement read in 256-byte blocks
            for (size_t offs = pTask->offset; offs < final_offs; offs += SpiToolkit::SPI_MAX_BLOCK_SIZE) {
                size_t block_size = SpiToolkit::SPI_MAX_BLOCK_SIZE;
                if (offs + block_size > final_offs) {
                    block_size = final_offs - offs;
                }
                if (!pToolkit->write_block(offs, pTask->data + offs, block_size)) {
                    pTask->success = false;
                    break;
                }
                pTask->progress = (offs - pTask->offset) / pTask->size;
            }
            pTask->success = true;
            break;

          case WorkerOperation::BlockWrite:
            // TODO: implement write in 256-byte blocks
            for (size_t offs = pTask->offset; offs < final_offs; offs += SpiToolkit::SPI_MAX_BLOCK_SIZE) {
                size_t block_size = SpiToolkit::SPI_MAX_BLOCK_SIZE;
                if (offs + block_size > final_offs) {
                    block_size = final_offs - offs;
                }
                if (!pToolkit->read_block(offs, const_cast<uint8_t*>(pTask->data + offs), block_size)) {
                    pTask->success = false;
                    break;
                }
                pTask->progress = (offs - pTask->offset) / pTask->size;
            }
            pTask->success = true;
            break;

          default:
            FURI_LOG_W(TAG, "unhandled operation!");
      }

      pTask->progress = WorkerTask::COMPLETE;
      FURI_LOG_I(TAG, "task done");
    }

    FURI_LOG_I(TAG, "worker is done");
    return 0;
}
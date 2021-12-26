#include "flash_manager_worker.h"
#include "spi/spi_toolkit.h"
#include "spi/task_executor.h"
#include <furi.h>

#undef TAG
#define TAG "FlashWorker"

static int32_t flash_manager_worker_thread(void* context);

WorkerTask::WorkerTask(WorkerOperation _operation, size_t _offset, uint8_t* _data, size_t _size)
    : offset(_offset)
    , data(_data)
    , size(_size)
    , operation(_operation)
    , progress(0)
    , success(false) {
}

FlashManagerWorker::FlashManagerWorker()
    : toolkit(std::make_unique<SpiToolkit>())
    , task_executor(std::make_unique<TaskExecutor>(toolkit.get())) {
    FURI_LOG_T(TAG, "ctor()");

    task_queue = osMessageQueueNew(8, sizeof(WorkerTask*), nullptr);

    FURI_LOG_D(TAG, "spawning thread()");
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
    FURI_LOG_D(TAG, "worker stop flag set");
    worker_running = false;

    furi_thread_join(thread);
    FURI_LOG_D(TAG, "join() done");
}

FlashManagerWorker::~FlashManagerWorker() {
    furi_assert(thread);
    furi_thread_free(thread);
    osMessageQueueDelete(task_queue);
    FURI_LOG_D(TAG, "dtor() done");
}

bool FlashManagerWorker::enqueue_task(WorkerTask* task) {
    FURI_LOG_T(TAG, "posting task");
    furi_assert(task);
    furi_assert(task->operation > WorkerOperation::None);

    FURI_LOG_D(
        TAG,
        "op: task=%x, code=%d, offs=%x, size=%x, data=%x",
        task,
        task->operation,
        task->offset,
        task->size,
        task->data);

    return osMessageQueuePut(task_queue, &task, 0, 300) == osOK;

    //FURI_LOG_I(TAG, "task posted");
    //return true;
}

bool FlashManagerWorker::is_busy() const {
    return osMessageQueueGetCount(task_queue) != 0;
}

/** Worker thread
 *
 * @param context
 * @return exit code
 */
static int32_t flash_manager_worker_thread(void* context) {
    FlashManagerWorker* instance = static_cast<FlashManagerWorker*>(context);
    // SpiToolkit *pToolkit = instance->toolkit.get();

    WorkerTask* p_task = nullptr;

    while(instance->worker_running) {
        osStatus_t task_status = osMessageQueueGet(instance->task_queue, &p_task, nullptr, 100);

        if(task_status == osErrorTimeout) {
            continue;
        }

        if(task_status != osOK) {
            FURI_LOG_W(TAG, "worker queue dequeue result: %d", task_status);
            break;
        }

        instance->task_executor->run(p_task);

        // instance->active_task = nullptr;
        FURI_LOG_D(
            TAG,
            "task done: op=%d @ %02X, res=%d",
            p_task->operation,
            p_task->offset,
            p_task->success);
    }

    FURI_LOG_I(TAG, "worker is done");
    return 0;
}
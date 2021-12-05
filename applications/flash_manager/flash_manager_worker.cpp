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
    FURI_LOG_I(TAG, "ctor()");

    init_mutex(&tasks_mutex, &tasks, sizeof(tasks));
    tasks_semaphore = osSemaphoreNew(1, 0, nullptr);

    FURI_LOG_I(TAG, "spawning thread()");
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
    osSemaphoreRelease(tasks_semaphore);

    furi_thread_join(thread);
    FURI_LOG_I(TAG, "join() done");
}

FlashManagerWorker::~FlashManagerWorker() {
    furi_assert(thread);
    furi_thread_free(thread);
    delete_mutex(&tasks_mutex);
    osSemaphoreDelete(tasks_semaphore);
    FURI_LOG_I(TAG, "dtor() done");
}

bool FlashManagerWorker::enqueue_task(WorkerTask* task) {
    FURI_LOG_I(TAG, "posting task");
    furi_assert(task);
    furi_assert(task->operation > WorkerOperation::None);

    FURI_LOG_I(
        TAG,
        "op: task=%x, code=%d, offs=%x, size=%x, data=%x",
        task,
        task->operation,
        task->offset,
        task->size,
        task->data);

    with_value_mutex_cpp(&tasks_mutex, [&](void*) { tasks.push(task); });

    osSemaphoreRelease(tasks_semaphore);

    FURI_LOG_I(TAG, "task posted");
    return true;
}

bool FlashManagerWorker::is_busy() const {
    return !tasks.empty();
}

/** Worker thread
 *
 * @param context
 * @return exit code
 */
static int32_t flash_manager_worker_thread(void* context) {
    FlashManagerWorker* instance = static_cast<FlashManagerWorker*>(context);
    // SpiToolkit *pToolkit = instance->toolkit.get();

    auto& tasks = instance->tasks;

    while(instance->worker_running) {
        if(tasks.empty()) {
            FURI_LOG_I(TAG, "awaiting tasks");

            osSemaphoreAcquire(instance->tasks_semaphore, osWaitForever);
            FURI_LOG_I(TAG, "worker is released");

            if(tasks.empty()) {
                FURI_LOG_I(TAG, "worker is released with no tasks!");
                if(instance->worker_running) {
                    FURI_LOG_E(TAG, "THIS IS A BUG");
                }
                continue;
            }
            osDelay(100);
        }

        WorkerTask* pTask = nullptr;

        with_value_mutex_cpp(&instance->tasks_mutex, [&](void*) {
            pTask = tasks.front();
            tasks.pop();
        });

        instance->task_executor->run(pTask);

        // instance->active_task = nullptr;
        FURI_LOG_I(
            TAG,
            "task done: op=%d @ %02X, res=%d",
            pTask->operation,
            pTask->offset,
            pTask->success);
    }

    FURI_LOG_I(TAG, "worker is done");
    return 0;
}
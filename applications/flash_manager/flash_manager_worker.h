#pragma once
#include <furi-hal.h>
#include <furi.h>
#include <memory>

class SpiToolkit;
class TaskExecutor;

// enum class WorkerState {
//     Unknown,
//     Idle,
//     Operating
// };

enum class WorkerOperation : uint8_t { Unknown, None, ChipId, ChipErase, BlockRead, BlockWrite };

struct WorkerTask {
    static const uint8_t COMPLETE = 100;

    const size_t offset;
    uint8_t* const data;
    const size_t size;

    const WorkerOperation operation;
    volatile uint8_t progress;
    bool success;

    // WorkerTask();
    WorkerTask(
        WorkerOperation _operation,
        size_t _offset = 0,
        uint8_t* _data = 0,
        size_t _size = 0);
    // void clear();

    inline bool completed() {
        return progress >= COMPLETE;
    }
};

class FlashManagerWorker {
public:
    FlashManagerWorker();
    ~FlashManagerWorker();

    void start();
    void stop();

    bool enqueue_task(WorkerTask* task);

    inline bool is_busy() const;

    std::unique_ptr<SpiToolkit> toolkit;
    std::unique_ptr<TaskExecutor> task_executor;

    osMessageQueueId_t task_queue;

    volatile bool worker_running;
    FuriThread* thread;
};
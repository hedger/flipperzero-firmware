#pragma once
#include <furi-hal.h>
#include <furi.h>
#include <memory>

class SpiToolkit;

//enum class WorkerState {
//    Unknown,
//    Idle,
//    Operating
//};

enum class WorkerOperation : uint8_t {
    Unknown,
    None,
    ChipId,
    ChipErase,
    BlockRead,
    BlockWrite
};

struct WorkerTask {
    static const uint8_t COMPLETE = 100;

    const size_t offset;
    const uint8_t* data;
    const size_t size;

    const WorkerOperation operation;
    volatile uint8_t progress;
    bool success;

    WorkerTask();
    WorkerTask(WorkerOperation _operation, size_t _offset = 0, uint8_t* _data = 0, size_t _size = 0);
    //void clear();

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

    //void enqueue_task(WorkerOperation operation, size_t offset, void *data, size_t size);
    bool enqueue_task(WorkerTask* task);

    int get_task_progress();

    inline bool is_busy() const;

    std::unique_ptr<SpiToolkit> toolkit;

    WorkerTask* active_task;

    volatile bool worker_running;
    //volatile WorkerOperation current_operation;
    FuriThread *thread;
    int task_progress;
};
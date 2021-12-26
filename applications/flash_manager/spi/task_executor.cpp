#include "task_executor.h"
#include "spi_toolkit.h"

#include <furi.h>

#define TAG "TaskExecutor"

TaskExecutor::TaskExecutor(SpiToolkit* _toolkit)
    : toolkit(_toolkit) {
}

void TaskExecutor::run(WorkerTask* pTask) {
    FURI_LOG_D(TAG, "running operation %d", pTask->operation);
    FURI_LOG_T(
        TAG,
        "op: task=%x, code=%d, offs=%x, size=%x, data=%x",
        pTask,
        pTask->operation,
        pTask->offset,
        pTask->size,
        pTask->data);
    pTask->progress = 0;
    pTask->success = false;

    switch(pTask->operation) {
    case WorkerOperation::ChipId:
        furi_assert(pTask->size == sizeof(SpiFlashInfo_t));
        furi_assert(pTask->data);

        pTask->success = toolkit->detect_flash();
        if(pTask->success) {
            *(reinterpret_cast<SpiFlashInfo_t*>(pTask->data)) = *toolkit->get_info();
        }

        // TODO: implement
        break;

    case WorkerOperation::ChipErase:
        run_chip_erase(pTask);
        break;

    case WorkerOperation::BlockRead:
        run_block_read(pTask);
        break;

    case WorkerOperation::BlockWrite:
        run_block_write(pTask);
        break;

    default:
        FURI_LOG_W(TAG, "unhandled operation!");
    }

    pTask->progress = WorkerTask::COMPLETE;
}

void TaskExecutor::run_chip_erase(WorkerTask* pTask) {
    // TODO: implement
    pTask->success = toolkit->chip_erase();
}

void TaskExecutor::run_block_read(WorkerTask* pTask) {
    // TODO: implement read in 256-byte blocks
    for(size_t offs = 0; offs < pTask->size; offs += SpiToolkit::SPI_MAX_BLOCK_SIZE) {
        size_t block_size = SpiToolkit::SPI_MAX_BLOCK_SIZE;
        if(offs + block_size > pTask->size) {
            block_size = pTask->size - offs;
        }
        if(!toolkit->read_block(
               pTask->offset + offs, const_cast<uint8_t*>(pTask->data) + offs, block_size)) {
            pTask->success = false;
            return;
        }
        pTask->progress = static_cast<uint32_t>(offs * 100) / pTask->size;
    }
    pTask->success = true;
}

void TaskExecutor::run_block_write(WorkerTask* pTask) {
    // TODO: implement write in 256-byte blocks
    for(size_t offs = 0; offs < pTask->size; offs += SpiToolkit::SPI_MAX_BLOCK_SIZE) {
        size_t block_size = SpiToolkit::SPI_MAX_BLOCK_SIZE;
        if(offs + block_size > pTask->size) {
            block_size = pTask->size - offs;
        }
        if(!toolkit->write_block(
               pTask->offset + offs, const_cast<uint8_t*>(pTask->data) + offs, block_size)) {
            pTask->success = false;
            return;
        }
        pTask->progress = static_cast<uint32_t>(offs * 100) / pTask->size;
    }
    pTask->success = true;
}
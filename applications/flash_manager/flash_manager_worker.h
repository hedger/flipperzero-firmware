#pragma once
#include <furi.h>
#include <furi-hal.h>

class FlashManagerWorker {
public:
  FlashManagerWorker();
  ~FlashManagerWorker();

  void start();
  void stop();

  volatile bool worker_running;
  FuriThread* thread;
};

#include "Task.h"
#include "platform.h"

void Task::checkTask() {
  uint16_t clock = read_clock_ms();
  if (clock_diff(lastExecution, clock) > interval || starting) {
    run();
    lastExecution = clock;
    starting = false;
  }
}

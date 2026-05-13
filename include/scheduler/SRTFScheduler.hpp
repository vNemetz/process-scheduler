#include "../core/Clock.hpp"
#include "../core/Task.hpp"
#include "../scheduler/IScheduler.hpp"

namespace sim{
class SRTFScheduler : public IScheduler {
public:
  Task *selectNextTask(std::vector<Task *> &readyQueue, Task *currentlyRunning,
                   int currentTick) override;

  std::string name() const override { return "SRTF"; }

  bool isShorter(Task *a, Task *b, Task *running);
};
}
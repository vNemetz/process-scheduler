#include "../../include/scheduler/SRTFScheduler.hpp"
#include "core/Task.hpp"

namespace sim
{

  // Returns true if "a" should take "b" place as the selected task
  bool SRTFScheduler::isShorter(Task *a, Task *b, Task *running)
  {
    // Shortest remaining time task is selected
    if (a->remainingTime < b->remainingTime)
    {
      return true;
    }
    else if (a->remainingTime > b->remainingTime)
    {
      return false;
    }

    // Tie-break criteria

    // 1. Already in CPU has priority
    if (a == running)
    {
      return true;
    }
    if (b == running)
    {
      return false;
    }

    // 2. Order of arrival (the one that arrived first has priority)
    if (a->arrivalTime < b->arrivalTime)
    {
      return true;
    }
    else if (a->arrivalTime > b->arrivalTime)
    {
      return false;
    }

    // 3. Total duration (shorter has priority)
    if (a->totalDuration < b->totalDuration)
    {
      return true;
    }
    else if (a->totalDuration > b->totalDuration)
    {
      return false;
    }

    // 50% choice (last criteria)
    return a->id < b->id;
  }

  Task *SRTFScheduler::selectNextTask(std::vector<Task *> &readyQueue,
                                      Task *currentlyRunning, int currentTick)
  {
    std::vector<Task *> candidates = readyQueue;
    if (currentlyRunning != nullptr)
    {
      candidates.push_back(currentlyRunning);
    }
    if (candidates.empty())
      return nullptr;

    // First candidate
    Task *selectedTask = candidates[0];

    for (int i = 1; i < candidates.size(); i++)
    {
      if (this->isShorter(candidates[i], selectedTask, currentlyRunning))
      {
        selectedTask = candidates[i];
      }
    }
    return selectedTask;
  }
}
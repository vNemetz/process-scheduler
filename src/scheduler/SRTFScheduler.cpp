#include "../../include/scheduler/SRTFScheduler.hpp"
#include "core/Task.hpp"

namespace sim
{
  Task *SRTFScheduler::selectNextTask(std::vector<Task *> &readyQueue,
                                      Task *currentlyRunning, int currentTick)
  {
    Task *selectedTask = nullptr;

    for (int i = 1; i < readyQueue.size(); i++)
    {
      if(selectedTask == nullptr) selectedTask = readyQueue[i];

      bool remainingTimeDraw = readyQueue[i]->remainingTime == selectedTask->remainingTime;
      bool firstArrivalDraw = remainingTimeDraw && readyQueue[i]->arrivalTime == selectedTask->arrivalTime;
      bool totalDurationDraw = readyQueue[i]->totalDuration == selectedTask->totalDuration;

      // Shortest remaining time task is selected
      if (selectedTask == nullptr || readyQueue[i]->remainingTime < selectedTask->remainingTime)
      {
        selectedTask = readyQueue[i];
      }
      // Priority for the currently running task
      else if (remainingTimeDraw && readyQueue[i] == currentlyRunning)
      {
        selectedTask = readyQueue[i];
      }

      // Priority for the one that arrived first
      else if (remainingTimeDraw && readyQueue[i]->arrivalTime < selectedTask->arrivalTime)
      {
        selectedTask = readyQueue[i];
      }
      // Priority for smaller total duration
      else if (remainingTimeDraw && firstArrivalDraw && readyQueue[i]->totalDuration < selectedTask->totalDuration)
      {
        selectedTask = readyQueue[i];
      }
      else if (remainingTimeDraw && firstArrivalDraw && totalDurationDraw)
      {
        // 50% chance
        bool alterSelected = rand() % 2 == 0;
        if (alterSelected)
          selectedTask = readyQueue[i];
      }
    }
    return selectedTask;
  }
}
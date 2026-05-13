#include <algorithm>
#include <utility>
#include "core/OperatingSystem.hpp"
#include <scheduler/PRIOpScheduler.hpp>
#include <scheduler/SRTFScheduler.hpp>

namespace sim
{

  OperatingSystem::OperatingSystem(std::string schedulerType,
                                   int quantumValue,
                                   int numCpus,
                                   std::vector<Task> initialTasks)
      : tasks(std::move(initialTasks)), currentIndex(0), quantum(quantumValue)
  {
    clock.setTime(0);

    // Creates CPUs
    for (int i = 0; i < numCpus; i++)
    {
      cpus.emplace_back(i); // Adds CPU with id = i
    }

    if (schedulerType == "SRTF")
    {
      scheduler = new SRTFScheduler();
    }
    else if (schedulerType == "PRIOP")
    {
      scheduler = new PRIOpScheduler();
    }
    else
    {
      // If not defined, uses SRTF as default
      scheduler = new SRTFScheduler();
    }

    // Saves initial snapshot (time zero)
    saveSnapshot();
  }

  void OperatingSystem::admitArrivals()
  {
    for (int i = 0; i < this->tasks.size(); i++)
    {
      if (this->tasks[i].arrivalTime == clock.getTime())
      {
        this->tasks[i].state = TaskState::READY;
      }
    }
  }

  Task *OperatingSystem::findTaskById(int id)
  {
    if (id == -1)
      return nullptr;
    for (auto &t : tasks)
    {
      if (t.id == id)
        return &t;
    }
    return nullptr;
  }

  void OperatingSystem::handleRunningTasks()
  {
    for (int i = 0; i < this->cpus.size(); i++)
    {
      int taskId = cpus[i].currentTaskId;
      Task *task = findTaskById(taskId);
      if (!task)
        continue;
      task->remainingTime--;
      cpus[i].currentQuantumTime++;
      if (task->remainingTime <= 0) // Task terminated, free CPU
      {
        task->state = TaskState::TERMINATED;
        this->cpus[i].currentTaskId = -1;
        this->cpus[i].currentQuantumTime = 0;
      }

      // Quantum preemption, running task goes back to the ready queue
      else if (cpus[i].currentQuantumTime >= this->quantum)
      {
        task->state = TaskState::READY;
        this->cpus[i].currentTaskId = -1;
        this->cpus[i].currentQuantumTime = 0;
      }
    }
  }

  std::vector<Task *> OperatingSystem::getReadyTasks()
  {
    std::vector<Task *> readyQueue;
    for (int i = 0; i < this->tasks.size(); i++)
    {
      if (this->tasks[i].state == TaskState::READY)
      {
        readyQueue.push_back(&this->tasks[i]);
      }
    }
    return readyQueue;
  }

  void OperatingSystem::dispatch()
  {
    if (!this->scheduler)
      return;

    std::vector<Task *> readyQueue = getReadyTasks();

    for (int i = 0; i < cpus.size(); i++)
    {
      Task *runningTask = findTaskById(cpus[i].currentTaskId);
      int currentTick = getCurrentTick();

      Task *nextTask = this->scheduler->selectNextTask(readyQueue, runningTask, currentTick);
      if (nextTask != nullptr)
      {

        if (nextTask != runningTask)
        {
          if (runningTask != nullptr)
            // Place running task back to the ready queue as it won't run next cycle
            runningTask->state = TaskState::READY;

          this->cpus[i].currentTaskId = nextTask->id;
          this->cpus[i].currentQuantumTime = 0;
          nextTask->state = TaskState::RUNNING;
        }
        // Removes the now RUNNING tasks from the readyQueue
        std::vector<sim::Task *>::iterator it = std::find(readyQueue.begin(), readyQueue.end(), nextTask);
        if (it != readyQueue.end())
        {
          readyQueue.erase(it);
        }
      }
      else
      {
        // Turns the CPU down (idle)
        this->cpus[i].currentTaskId = -1;
      }
    }
  }

  void OperatingSystem::saveSnapshot()
  {
    if (!this->tasks.size() || !this->cpus.size())
      return;
    GlobalState globalState;
    globalState.tick = getCurrentTick();

    globalState.tasks = this->tasks;
    globalState.cpus = this->cpus;

    this->globalStates.push_back(globalState);
  }

  void OperatingSystem::executeOneTick()
  {
    admitArrivals();      // Admits the arrival of READY tasks
    handleRunningTasks(); // CPU "works" on the running tasks in the current tick
    dispatch();           // Select next tasks using the scheduler
    saveSnapshot();       // Save current tick's state

    this->clock.increment(); // Moves to next tick
  }

  bool OperatingSystem::isFinished() const
  {
    for (int i = 0; i < this->tasks.size(); i++)
    {
      if (this->tasks[i].state != TaskState::TERMINATED)
        return false;
    }
    return true;
  }

  void OperatingSystem::execute()
  {
    while (!isFinished())
    {
      this->executeOneTick();
    }
  }

}
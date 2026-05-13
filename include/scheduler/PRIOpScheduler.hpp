#pragma once

#include "scheduler/IScheduler.hpp"

namespace sim {

class PRIOpScheduler : public IScheduler {
public:
    Task* selectNextTask(std::vector<Task*>& readyQueue,Task* currentlyRunning,int currentTick) override;

    std::string name() const override { return "PRIOp"; }

private:
    // Retorna true se 'a' deve substituir 'b' como proximo a executar.
    static bool beats(Task* a, Task* b, Task* running);
};

} 

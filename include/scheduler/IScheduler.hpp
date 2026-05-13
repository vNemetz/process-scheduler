
#pragma once

#include "core/Task.hpp"
#include <string>
#include <vector>

namespace sim {

class IScheduler {
public:

    virtual ~IScheduler() = default;

    virtual Task* selectNextTask(std::vector<Task*>& readyQueue,
                             Task* currentTask,
                             int currentTick) = 0;

    virtual std::string name() const = 0;
};

}
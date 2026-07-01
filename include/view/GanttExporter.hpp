#pragma once

#include <string>
#include <vector>

#include "core/OperatingSystem.hpp"
#include "core/Task.hpp"

namespace view {

bool exportGanttPng(const sim::OperatingSystem& os,
                    const std::vector<sim::Task>& initialTasks,
                    const std::string& filename = "gantt.png");

}  // namespace view

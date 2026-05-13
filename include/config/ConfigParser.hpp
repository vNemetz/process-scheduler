#pragma once

#include <string>
#include <vector>
#include "core/Task.hpp"

namespace sim
{
    class ConfigParser
    {
    public:
        // Retorna os dados crus lidos do arquivo
        static bool parse(const std::string& filename, 
                          std::string& outSchedulerName,
                          int& outQuantum,
                          int& outNumCpus,
                          std::vector<Task>& outTasks);
    };
}
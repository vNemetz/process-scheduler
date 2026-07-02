#pragma once

#include <string>
#include <vector>
#include "core/Task.hpp"

namespace sim
{
    class ConfigParser
    {
    public:
        // Retorna os dados crus lidos do arquivo.
        // outAlpha e' opcional (usado apenas pelo escalonador PRIOPEnv).
        // Quando o quarto campo da primeira linha nao esta presente, outAlpha
        // recebe 0 (equivalente a sem envelhecimento).
        static bool parse(const std::string& filename,
                          std::string& outSchedulerName,
                          int& outQuantum,
                          int& outNumCpus,
                          int& outAlpha,
                          std::vector<Task>& outTasks);
    };
}

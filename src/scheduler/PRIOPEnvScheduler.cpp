#include "scheduler/PRIOPEnvScheduler.hpp"
#include "scheduler/SchedulerFactory.hpp"

#include <cstdlib>

namespace sim {

namespace {
    // Auto-registro no factory (mesmo padrao dos outros escalonadores).
    const bool priopEnvRegistered = SchedulerFactory::registerScheduler(
        "PRIOPENV", []{ return std::unique_ptr<IScheduler>(new PRIOPEnvScheduler()); });
}

// Retorna <0 se 'a' vence, >0 se 'b' vence, 0 se empate real (sorteio).
// Criterios (req 1.3 do Projeto B):
//   principal : dynamicPriority maior vence
//   desempate 1: staticPriority maior vence
//   desempate 2: tarefa que estava rodando ganha (evita context switch)
//   desempate 3: menor arrival_time
//   desempate 4: menor duracao total
//   desempate 5: sorteio
static int comparePRIOPEnv(Task* a, Task* b, Task* running) {
    if (a->dynamicPriority != b->dynamicPriority)
        return b->dynamicPriority - a->dynamicPriority;

    if (a->staticPriority != b->staticPriority)
        return b->staticPriority - a->staticPriority;

    if (a == running && b != running) return -1;
    if (b == running && a != running) return  1;

    if (a->arrivalTime != b->arrivalTime)
        return a->arrivalTime - b->arrivalTime;

    if (a->totalDuration != b->totalDuration)
        return a->totalDuration - b->totalDuration;

    return 0;
}

Task* PRIOPEnvScheduler::selectNextTask(std::vector<Task*>& readyQueue,
                                        Task* currentlyRunning,
                                        int /*currentTick*/)
{
    std::vector<Task*> candidates = readyQueue;
    if (currentlyRunning != nullptr) candidates.push_back(currentlyRunning);
    if (candidates.empty()) return nullptr;

    std::vector<Task*> best;
    best.push_back(candidates[0]);

    for (size_t i = 1; i < candidates.size(); ++i) {
        int cmp = comparePRIOPEnv(candidates[i], best[0], currentlyRunning);
        if (cmp < 0) {
            best.clear();
            best.push_back(candidates[i]);
        } else if (cmp == 0) {
            best.push_back(candidates[i]);
        }
    }

    Task* selected = nullptr;
    if (best.size() == 1) {
        selected = best[0];
    } else {
        int idx = std::rand() % static_cast<int>(best.size());
        selected = best[idx];
        selected->wonByLottery = true;  // UI desenha marcador.
    }

    return selected;
}

}  // namespace sim

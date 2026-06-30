#include "scheduler/PRIOpScheduler.hpp"
#include "scheduler/SchedulerFactory.hpp"

#include <cstdlib>

namespace sim {

// Auto-registro no factory. Ver SRTFScheduler.cpp para a justificativa.
namespace {
    const bool priopRegistered = SchedulerFactory::registerScheduler(
        "PRIOP", []{ return std::unique_ptr<IScheduler>(new PRIOpScheduler()); });
}

// Compara duas tarefas pelos criterios PRIOp (req 4.4).
// Retorna <0 se 'a' vence, >0 se 'b' vence, 0 se empate real (sorteio).
static int comparePRIOp(Task* a, Task* b, Task* running) {
    // Criterio primario PRIOp (req 4.4): maior prioridade estatica vence.
    if (a->staticPriority != b->staticPriority)
        return b->staticPriority - a->staticPriority;

    // A partir daqui, aplicam-se os mesmos 4 criterios da req 4.3.

    // (1) Continuidade da tarefa em execucao.
    if (a == running && b != running) return -1;
    if (b == running && a != running) return  1;

    // (2) Menor arrival_time.
    if (a->arrivalTime != b->arrivalTime)
        return a->arrivalTime - b->arrivalTime;

    // (3) Menor duracao total.
    if (a->totalDuration != b->totalDuration)
        return a->totalDuration - b->totalDuration;

    // (4) Empate real -> sorteio.
    return 0;
}

Task* PRIOpScheduler::selectNextTask(std::vector<Task*>& readyQueue,
                                     Task* currentlyRunning,
                                     int /*currentTick*/)
{
    std::vector<Task*> candidates = readyQueue;
    if (currentlyRunning != nullptr) candidates.push_back(currentlyRunning);
    if (candidates.empty()) return nullptr;

    std::vector<Task*> best;
    best.push_back(candidates[0]);

    for (size_t i = 1; i < candidates.size(); ++i) {
        int cmp = comparePRIOp(candidates[i], best[0], currentlyRunning);
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

// Mantido por compatibilidade do header. Versao bool de comparePRIOp.
bool PRIOpScheduler::beats(Task* a, Task* b, Task* running) {
    return comparePRIOp(a, b, running) < 0;
}

}  // namespace sim

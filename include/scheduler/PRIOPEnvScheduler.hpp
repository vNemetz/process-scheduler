#pragma once

#include "scheduler/IScheduler.hpp"

namespace sim {

// Escalonador preemptivo por prioridades com envelhecimento
// (Projeto B — req 1). Decisao baseada em dynamicPriority (mantida
// pelo OperatingSystem, que soma alpha por tick para tarefas READY
// e reseta para staticPriority quando a tarefa esta RUNNING).
//
// O criterio principal e' o valor de dynamicPriority (maior vence).
// Em caso de empate, aplica os criterios da req 1.3 do Projeto B.
class PRIOPEnvScheduler : public IScheduler {
public:
    Task* selectNextTask(std::vector<Task*>& readyQueue,
                         Task* currentlyRunning,
                         int currentTick) override;

    std::string name() const override { return "PRIOPEnv"; }
};

}  // namespace sim

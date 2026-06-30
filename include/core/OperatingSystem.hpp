#pragma once

#include <memory>
#include <vector>
#include <string>

#include "core/Task.hpp"
#include "core/CPU.hpp"
#include "core/Clock.hpp"
#include "scheduler/IScheduler.hpp"

namespace sim {

// Fotografia completa do estado do SO em um dado tick.
// Usada para o historico (req 1.5.2: avancar/retroceder).
struct GlobalState {
    int tick;
    std::vector<Task> tasks;
    std::vector<CPU> cpus;
};

class OperatingSystem {
public:
    OperatingSystem(std::string schedulerType,
                    int quantum,
                    int cpus,
                    std::vector<Task> tasks);

    // Roda a simulacao do inicio ao fim sem intervencao (req 1.5.b).
    bool execute();

    // Avanca UM tick. Retorna true se algo foi feito, false se ja terminou.
    // Exposto para o modo passo-a-passo (req 1.5.a): a UI chama isso.
    bool executeOneTick();

    bool isFinished() const;
    const std::vector<GlobalState>& getSnapshotsHistory() const;

    int getCurrentTick() const { return clock.getTime(); }
    const std::vector<Task>& getTasks() const { return tasks; }
    const std::vector<CPU>&  getCpus()  const { return cpus; }
    const IScheduler&        getScheduler() const { return *scheduler; }
    const std::string&       getSchedulerName() const { return schedulerName; }
    int                      getQuantum() const { return quantum; }

    // Permite a UI editar o estado de uma tarefa (req 1.5.2 / 3.4).
    // Devolve true se a alteracao foi aplicada.
    bool setTaskState(int taskId, TaskState newState);

private:
    std::vector<Task> tasks;
    std::vector<CPU>  cpus;
    Clock clock;
    std::unique_ptr<IScheduler> scheduler;
    std::string schedulerName;
    int quantum;

    std::vector<GlobalState> globalStates;  // Um snapshot por tick.

    void saveSnapshot();
    void admitArrivals();
    void handleRunningTasks();
    void accumulateWaitMetrics();
    void dispatch();

    std::vector<Task*> getReadyTasks();
    Task* findTaskById(int id);
};

}  // namespace sim

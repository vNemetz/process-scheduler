#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "core/Task.hpp"
#include "core/CPU.hpp"
#include "core/Clock.hpp"
#include "core/Mutex.hpp"
#include "scheduler/IScheduler.hpp"

namespace sim {

// I/O pendente: um par (tarefa, tick em que a IRQ vai disparar).
// A IRQ e' processada no inicio do tick indicado, movendo a tarefa
// de SUSPENDED de volta para READY (req 3.4 do Projeto B).
struct PendingIO {
    int taskId;
    int completionTick;
};

// Registro de uma acao que disparou em um tick especifico.
// Usada pela UI para desenhar icones (lock/unlock/IO) no Gantt.
struct ActionEvent {
    int taskId;
    ActionType type;
    int mutexId;      // para MUTEX_LOCK / MUTEX_UNLOCK
    int ioDuration;   // para IO
};

// Fotografia completa do estado do SO em um dado tick.
// Usada para o historico (req 1.5.2: avancar/retroceder).
// Inclui mutexes e I/O pendente para que o retroceder preserve tambem
// esses estados (Projeto B).
struct GlobalState {
    int tick;
    std::vector<Task> tasks;
    std::vector<CPU>  cpus;
    std::map<int, Mutex>     mutexes;
    std::vector<PendingIO>   pendingIOs;
    std::vector<ActionEvent> tickActions;
};

class OperatingSystem {
public:
    OperatingSystem(std::string schedulerType,
                    int quantum,
                    int cpus,
                    int alpha,
                    std::vector<Task> tasks);

    // Roda a simulacao do inicio ao fim sem intervencao (req 1.5.b).
    bool execute();

    // Avanca UM tick. Retorna true se algo foi feito, false se ja terminou.
    // Exposto para o modo passo-a-passo (req 1.5.a): a UI chama isso.
    bool executeOneTick();

    bool isFinished() const;
    const std::vector<GlobalState>& getSnapshotsHistory() const;
    bool restoreSnapshot(std::size_t index);
    bool updateCurrentSnapshot();
    void truncateHistoryAfterCurrentTick();

    int getCurrentTick() const { return clock.getTime(); }
    const std::vector<Task>& getTasks() const { return tasks; }
    const std::vector<CPU>&  getCpus()  const { return cpus; }
    const IScheduler&        getScheduler() const { return *scheduler; }
    const std::string&       getSchedulerName() const { return schedulerName; }
    int                      getQuantum() const { return quantum; }
    int                      getAlpha()   const { return alpha; }

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
    int alpha;

    std::map<int, Mutex>   mutexes;
    std::vector<PendingIO> pendingIOs;

    // Buffer de acoes disparadas no tick corrente. Populado por
    // processTaskActions(), copiado para o snapshot no fim do tick e
    // limpado no inicio do proximo.
    std::vector<ActionEvent> currentTickActions;

    std::vector<GlobalState> globalStates;  // Um snapshot por tick.

    void saveSnapshot();
    void admitArrivals();
    void handleRunningTasks();
    void accumulateWaitMetrics();
    void dispatch();
    void processIOCompletions();
    void processTaskActions();
    void applyAging();

    // Utilitarios usados pelas acoes de mutex/IO.
    Mutex& getOrCreateMutex(int id);
    void   grantMutexToNextWaiter(Mutex& mtx);
    void   releaseCpuOfTask(Task& t);

    std::vector<Task*> getReadyTasks();
    Task* findTaskById(int id);
};

}  // namespace sim

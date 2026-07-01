#include "core/OperatingSystem.hpp"
#include "scheduler/SchedulerFactory.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

namespace sim {

OperatingSystem::OperatingSystem(std::string schedulerType,
                                 int quantumValue,
                                 int numCpus,
                                 int alphaValue,
                                 std::vector<Task> initialTasks)
    : tasks(std::move(initialTasks)),
      quantum(quantumValue),
      alpha(alphaValue < 0 ? 0 : alphaValue)
{
    clock.setTime(0);

    if (numCpus < 2) numCpus = 2;  // Req geral 2: minimo 2 CPUs.
    for (int i = 0; i < numCpus; ++i) cpus.emplace_back(i);

    // Factory plugavel (req 4.2). Se o nome nao for conhecido, faz fallback
    // para SRTF e avisa no stderr. Nada de crash silencioso.
    scheduler = SchedulerFactory::create(schedulerType);
    if (!scheduler) {
        std::cerr << "[OperatingSystem] Scheduler desconhecido '"
                  << schedulerType << "', usando SRTF como padrao.\n";
        scheduler = SchedulerFactory::create("SRTF");
        schedulerName = "SRTF";
    } else {
        schedulerName = schedulerType;
    }
}

// Promove tarefas NEW -> READY quando o instante de ingresso e' atingido.
void OperatingSystem::admitArrivals() {
    int now = clock.getTime();
    for (auto& t : tasks) {
        if (t.state == TaskState::NEW && t.arrivalTime <= now) {
            t.state = TaskState::READY;
        }
    }
}

Task* OperatingSystem::findTaskById(int id) {
    if (id == -1) return nullptr;
    for (auto& t : tasks) {
        if (t.id == id) return &t;
    }
    return nullptr;
}

// "Trabalha" 1 tick em cada tarefa que esta RUNNING.
// Decrementa remainingTime, conta quantum, incrementa cpuTimeConsumed
// (relogio local usado pelas acoes ML/MU/IO), e lida com termino/preempcao.
void OperatingSystem::handleRunningTasks() {
    int now = clock.getTime();

    for (auto& cpu : cpus) {
        Task* task = findTaskById(cpu.currentTaskId);
        if (!task) continue;

        if (task->startTime < 0) task->startTime = now;  // Primeira execucao.
        task->remainingTime--;
        task->cpuTimeConsumed++;
        cpu.currentQuantumTime++;

        if (task->remainingTime <= 0) {
            // Tarefa terminou.
            task->state = TaskState::TERMINATED;
            task->finishTime = now + 1;   // Termina ao FIM deste tick.
            task->cpuAssigned = -1;
            cpu.currentTaskId = -1;
            cpu.currentQuantumTime = 0;
        } else if (cpu.currentQuantumTime >= quantum) {
            // Quantum esgotado: volta para a fila de prontos.
            task->state = TaskState::READY;
            task->cpuAssigned = -1;
            task->preemptions++;
            cpu.currentTaskId = -1;
            cpu.currentQuantumTime = 0;
        }
    }
}

// Mantem contadores de "tempo gasto esperando". Util para o relatorio
// final e para a UI mostrar metricas por tarefa.
void OperatingSystem::accumulateWaitMetrics() {
    for (auto& t : tasks) {
        if (t.state == TaskState::READY)     t.waitingTime++;
        if (t.state == TaskState::SUSPENDED) t.suspendedTime++;
    }
}

std::vector<Task*> OperatingSystem::getReadyTasks() {
    std::vector<Task*> readyQueue;
    for (auto& t : tasks) {
        if (t.state == TaskState::READY) readyQueue.push_back(&t);
    }
    return readyQueue;
}

void OperatingSystem::dispatch() {
    if (!scheduler) return;

    // Limpa marcador de sorteio do tick anterior — o scheduler reativa
    // se houver empate real neste tick. Sem isso, o icone "ficaria aceso"
    // por toda a barra subsequente.
    for (auto& t : tasks) t.wonByLottery = false;

    std::vector<Task*> readyQueue = getReadyTasks();
    int currentTick = getCurrentTick();

    for (auto& cpu : cpus) {
        Task* runningTask = findTaskById(cpu.currentTaskId);
        Task* nextTask = scheduler->selectNextTask(readyQueue, runningTask, currentTick);

        if (nextTask != nullptr) {
            if (nextTask != runningTask) {
                if (runningTask != nullptr) {
                    runningTask->state = TaskState::READY;
                    runningTask->cpuAssigned = -1;
                    runningTask->preemptions++;
                    readyQueue.push_back(runningTask);
                }

                cpu.currentTaskId = nextTask->id;
                cpu.currentQuantumTime = 0;
                nextTask->state = TaskState::RUNNING;
                nextTask->cpuAssigned = cpu.id;
            }

            // Remove a tarefa selecionada da fila local para nao ser
            // escolhida de novo por outra CPU.
            auto it = std::find(readyQueue.begin(), readyQueue.end(), nextTask);
            if (it != readyQueue.end()) readyQueue.erase(it);
        } else {
            // Sem tarefa elegivel: CPU "desligada" (req 1.2).
            if (runningTask != nullptr) runningTask->cpuAssigned = -1;
            cpu.currentTaskId = -1;
            cpu.currentQuantumTime = 0;
        }
    }
}

Mutex& OperatingSystem::getOrCreateMutex(int id) {
    auto it = mutexes.find(id);
    if (it == mutexes.end()) {
        it = mutexes.emplace(id, Mutex(id)).first;
    }
    return it->second;
}

void OperatingSystem::releaseCpuOfTask(Task& t) {
    if (t.cpuAssigned >= 0 && t.cpuAssigned < static_cast<int>(cpus.size())) {
        cpus[t.cpuAssigned].currentTaskId = -1;
        cpus[t.cpuAssigned].currentQuantumTime = 0;
    }
    t.cpuAssigned = -1;
}

// Ao liberar um mutex, se ha' esperantes na fila, o primeiro se torna
// owner e volta para READY (recebeu o recurso, pode competir por CPU).
void OperatingSystem::grantMutexToNextWaiter(Mutex& mtx) {
    if (mtx.waitingTaskIds.empty()) {
        mtx.ownerTaskId = -1;
        return;
    }
    int nextId = mtx.waitingTaskIds.front();
    mtx.waitingTaskIds.pop_front();
    mtx.ownerTaskId = nextId;
    Task* next = findTaskById(nextId);
    if (next) {
        next->state = TaskState::READY;
        next->suspendReason = SuspendReason::NONE;
    }
}

// Verifica se ha' I/Os cujo tick de conclusao (IRQ) e' o tick atual.
// Nesse caso a tarefa suspensa por I/O volta para READY (req 3.4).
void OperatingSystem::processIOCompletions() {
    int now = clock.getTime();
    auto it = pendingIOs.begin();
    while (it != pendingIOs.end()) {
        if (it->completionTick <= now) {
            Task* t = findTaskById(it->taskId);
            if (t && t->state == TaskState::SUSPENDED
                  && t->suspendReason == SuspendReason::IO) {
                t->state = TaskState::READY;
                t->suspendReason = SuspendReason::NONE;
            }
            it = pendingIOs.erase(it);
        } else {
            ++it;
        }
    }
}

// Dispara acoes ML/MU/IO cujas relativeTime coincidem com o cpuTimeConsumed
// atual da tarefa RUNNING. Chamado apos handleRunningTasks e depois de
// dispatch, para cobrir tanto acoes durante a execucao quanto acoes em
// relativeTime=0 no momento em que a tarefa (re)inicia (req 2.6 / 3.6).
void OperatingSystem::processTaskActions() {
    int now = clock.getTime();

    for (auto& t : tasks) {
        // So faz sentido para tarefas correndo agora.
        while (t.state == TaskState::RUNNING
               && t.nextActionIndex < t.actions.size()
               && t.actions[t.nextActionIndex].relativeTime == t.cpuTimeConsumed) {
            const TaskAction action = t.actions[t.nextActionIndex];
            t.nextActionIndex++;

            currentTickActions.push_back({
                t.id, action.type, action.mutexId, action.ioDuration });

            switch (action.type) {
                case ActionType::MUTEX_LOCK: {
                    Mutex& mtx = getOrCreateMutex(action.mutexId);
                    if (mtx.isFree()) {
                        mtx.ownerTaskId = t.id;
                    } else if (mtx.ownerTaskId == t.id) {
                        // Ja e' dona do mutex; ignora (lock reentrante).
                    } else {
                        // Recurso ocupado — suspende a tarefa e libera a CPU.
                        mtx.waitingTaskIds.push_back(t.id);
                        t.state = TaskState::SUSPENDED;
                        t.suspendReason = SuspendReason::MUTEX;
                        releaseCpuOfTask(t);
                    }
                    break;
                }
                case ActionType::MUTEX_UNLOCK: {
                    Mutex& mtx = getOrCreateMutex(action.mutexId);
                    if (mtx.ownerTaskId == t.id) {
                        grantMutexToNextWaiter(mtx);
                    }
                    // Se a tarefa nao era dona, e' um unlock invalido — ignora.
                    break;
                }
                case ActionType::IO: {
                    // Suspende a tarefa e agenda IRQ para daqui a "ioDuration"
                    // ticks. A IRQ vai ser processada no inicio do tick alvo
                    // (processIOCompletions), voltando a tarefa a READY.
                    pendingIOs.push_back({ t.id, now + action.ioDuration });
                    t.state = TaskState::SUSPENDED;
                    t.suspendReason = SuspendReason::IO;
                    releaseCpuOfTask(t);
                    break;
                }
            }
        }
    }
}

// Envelhecimento (Projeto B, req 1): tarefas READY ganham alpha por tick;
// tarefas RUNNING tem sua prioridade dinamica resetada para a estatica.
// Sem efeito quando alpha == 0 (ou seja, todos os cenarios que nao usam
// PRIOPEnv).
void OperatingSystem::applyAging() {
    if (alpha <= 0) return;
    for (auto& t : tasks) {
        if (t.state == TaskState::READY) {
            t.dynamicPriority += alpha;
        } else if (t.state == TaskState::RUNNING) {
            t.dynamicPriority = t.staticPriority;
        }
    }
}

void OperatingSystem::saveSnapshot() {
    GlobalState snap;
    snap.tick        = getCurrentTick();
    snap.tasks       = tasks;   // Copia profunda — historico imutavel.
    snap.cpus        = cpus;
    snap.mutexes     = mutexes;
    snap.pendingIOs  = pendingIOs;
    snap.tickActions = currentTickActions;
    globalStates.push_back(std::move(snap));
}

bool OperatingSystem::executeOneTick() {
    if (isFinished()) return false;

    currentTickActions.clear();

    admitArrivals();
    processIOCompletions();    // Wakes de I/O antes de contabilizar work.
    handleRunningTasks();
    processTaskActions();      // Acoes com relativeTime que atingiu cpuTimeConsumed.
    dispatch();
    processTaskActions();      // Acoes em relativeTime=0 de tarefas recem-dispachadas.
    applyAging();
    accumulateWaitMetrics();
    saveSnapshot();

    clock.increment();
    return true;
}

bool OperatingSystem::isFinished() const {
    for (const auto& t : tasks) {
        if (t.state != TaskState::TERMINATED) return false;
    }
    return true;
}

bool OperatingSystem::execute() {
    while (!isFinished()) executeOneTick();
    return true;
}

bool OperatingSystem::setTaskState(int taskId, TaskState newState) {
    Task* t = findTaskById(taskId);
    if (!t) return false;

    if (t->cpuAssigned >= 0 && t->cpuAssigned < static_cast<int>(cpus.size())) {
        cpus[t->cpuAssigned].currentTaskId = -1;
        cpus[t->cpuAssigned].currentQuantumTime = 0;
    }

    for (auto& cpu : cpus) {
        if (cpu.currentTaskId == taskId) {
            cpu.currentTaskId = -1;
            cpu.currentQuantumTime = 0;
        }
    }

    t->cpuAssigned = -1;
    t->suspendReason = SuspendReason::NONE;
    t->state = newState;

    if (newState == TaskState::TERMINATED) {
        t->remainingTime = 0;
        if (t->finishTime < 0) t->finishTime = clock.getTime();
    } else if (newState == TaskState::READY) {
        if (t->remainingTime <= 0) t->remainingTime = 1;
        t->finishTime = -1;
    }

    return true;
}

bool OperatingSystem::restoreSnapshot(std::size_t index) {
    if (index >= globalStates.size()) return false;

    const GlobalState& snap = globalStates[index];
    tasks              = snap.tasks;
    cpus               = snap.cpus;
    mutexes            = snap.mutexes;
    pendingIOs         = snap.pendingIOs;
    currentTickActions = snap.tickActions;
    clock.setTime(snap.tick + 1);
    return true;
}

bool OperatingSystem::updateCurrentSnapshot() {
    if (globalStates.empty()) return false;

    int snapshotTick = clock.getTime() - 1;
    for (auto& snap : globalStates) {
        if (snap.tick == snapshotTick) {
            snap.tasks       = tasks;
            snap.cpus        = cpus;
            snap.mutexes     = mutexes;
            snap.pendingIOs  = pendingIOs;
            snap.tickActions = currentTickActions;
            return true;
        }
    }

    return false;
}

void OperatingSystem::truncateHistoryAfterCurrentTick() {
    int snapshotTick = clock.getTime() - 1;
    globalStates.erase(
        std::remove_if(globalStates.begin(), globalStates.end(),
                       [snapshotTick](const GlobalState& snap) {
                           return snap.tick > snapshotTick;
                       }),
        globalStates.end());
}

const std::vector<GlobalState>& OperatingSystem::getSnapshotsHistory() const {
    return globalStates;
}

}  // namespace sim

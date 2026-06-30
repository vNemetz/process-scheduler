#include "core/OperatingSystem.hpp"
#include "scheduler/SchedulerFactory.hpp"

#include <algorithm>
#include <iostream>
#include <utility>

namespace sim {

OperatingSystem::OperatingSystem(std::string schedulerType,
                                 int quantumValue,
                                 int numCpus,
                                 std::vector<Task> initialTasks)
    : tasks(std::move(initialTasks)),
      quantum(quantumValue)
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
// Bug antes: comparava `arrivalTime == clock.getTime()`. Trocado por `<=`
// (alem de checar o state) para suportar edicao manual de estado onde
// uma tarefa pode "perder" o seu tick exato de admissao.
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
// Decrementa remainingTime, conta quantum, lida com termino e preempcao.
void OperatingSystem::handleRunningTasks() {
    int now = clock.getTime();

    for (auto& cpu : cpus) {
        Task* task = findTaskById(cpu.currentTaskId);
        if (!task) continue;

        if (task->startTime < 0) task->startTime = now;  // Primeira execucao.
        task->remainingTime--;
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

void OperatingSystem::saveSnapshot() {
    GlobalState snap;
    snap.tick  = getCurrentTick();
    snap.tasks = tasks;   // Copia profunda — historico imutavel.
    snap.cpus  = cpus;
    globalStates.push_back(std::move(snap));
}

bool OperatingSystem::executeOneTick() {
    if (isFinished()) return false;

    admitArrivals();
    handleRunningTasks();
    dispatch();
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

    // Se a tarefa estava em uma CPU, libera a CPU.
    if (t->cpuAssigned >= 0 && t->cpuAssigned < static_cast<int>(cpus.size())) {
        cpus[t->cpuAssigned].currentTaskId = -1;
        cpus[t->cpuAssigned].currentQuantumTime = 0;
        t->cpuAssigned = -1;
    }
    t->state = newState;
    return true;
}

const std::vector<GlobalState>& OperatingSystem::getSnapshotsHistory() const {
    return globalStates;
}

}  // namespace sim

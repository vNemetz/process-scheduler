#pragma once

#include <SFML/Graphics/Color.hpp>
#include <string>
#include <vector>

namespace sim {

enum class TaskState {
    NEW,         // Criada, mas instante de ingresso ainda nao chegou
    READY,       // Na fila de prontos, esperando uma CPU
    RUNNING,     // Executando em alguma CPU neste tick
    SUSPENDED,   // Bloqueada (mutex / I/O) — usado no Projeto B
    TERMINATED   // Concluida (remainingTime chegou a zero)
};

inline const char* toString(TaskState s) {
    switch (s) {
        case TaskState::NEW:        return "NEW";
        case TaskState::READY:      return "READY";
        case TaskState::RUNNING:    return "RUNNING";
        case TaskState::SUSPENDED:  return "SUSPENDED";
        case TaskState::TERMINATED: return "TERMINATED";
    }
    return "?";
}

enum class SuspendReason {
    NONE,        // Nao esta suspensa
    IO,          // Operacao de I/O em andamento
    MUTEX        // Esperando um mutex
};

// TCB (Task Control Block) — concentra TODAS as informacoes da tarefa
// antes, durante e depois da simulacao (requisito 1.3 do enunciado).
struct Task {

    // ---- Parametros lidos do arquivo de configuracao ----
    int id;
    sf::Color color;
    int arrivalTime;
    int totalDuration;
    int staticPriority;          // Usado pelo PRIOp
    std::vector<std::string> rawEvents;  // Reservado para Projeto B (mutex/IO)

    // ---- Estado em tempo de simulacao ----
    TaskState state = TaskState::NEW;
    SuspendReason suspendReason = SuspendReason::NONE;
    int remainingTime;
    int cpuAssigned = -1;        // -1 = nao esta em CPU
    int quantumTicksLeft = 0;

    // Sinaliza, durante UM tick, que esta tarefa venceu o desempate por
    // sorteio (criterio 4 da req 4.3). A UI usa isso para desenhar um
    // marcador no bloco de Gantt correspondente.
    bool wonByLottery = false;

    // ---- Metricas preenchidas durante a simulacao ----
    int finishTime    = -1;      // Tick em que a tarefa terminou (-1 = nao terminou)
    int startTime     = -1;      // Tick da primeira execucao
    int waitingTime   = 0;       // Total de ticks gastos em READY
    int suspendedTime = 0;       // Total de ticks gastos em SUSPENDED
    int preemptions   = 0;       // Quantas vezes foi tirada da CPU

    Task() = default;

    Task(int _id,
         sf::Color _color,
         int _arrival,
         int _duration,
         int _priority,
         std::vector<std::string> _events)
        : id(_id),
          color(_color),
          arrivalTime(_arrival),
          totalDuration(_duration),
          staticPriority(_priority),
          rawEvents(std::move(_events)),
          remainingTime(_duration)
    {}

    bool isRunning()    const { return state == TaskState::RUNNING; }
    bool isReady()      const { return state == TaskState::READY; }
    bool isSuspended()  const { return state == TaskState::SUSPENDED; }
    bool isTerminated() const { return state == TaskState::TERMINATED; }
    bool isNew()        const { return state == TaskState::NEW; }

    bool hasArrived(int currentTick) const {
        return currentTick >= arrivalTime;
    }

    // Turnaround = finishTime - arrivalTime. So vale se ja terminou.
    int turnaround() const {
        return (finishTime < 0) ? -1 : (finishTime - arrivalTime);
    }
};

}

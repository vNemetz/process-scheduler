#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstddef>
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

// Acoes que uma tarefa executa durante sua vida (Projeto B).
// Sao lidas do arquivo de configuracao no formato:
//   ML01:5  -> tenta lock do mutex 1 no tick relativo 5
//   MU01:8  -> libera o mutex 1 no tick relativo 8
//   IO:3-4  -> comeca I/O no tick relativo 3, dura 4 unidades
// O "tick relativo" e' o tempo que a tarefa esteve efetivamente em CPU
// (cpuTimeConsumed), nao o tempo global do sistema.
enum class ActionType { MUTEX_LOCK, MUTEX_UNLOCK, IO };

struct TaskAction {
    ActionType type;
    int relativeTime;   // Momento de disparo (em cpuTimeConsumed).
    int mutexId;        // Para MUTEX_LOCK / MUTEX_UNLOCK.
    int ioDuration;     // Para IO.
};

// TCB (Task Control Block) — concentra TODAS as informacoes da tarefa
// antes, durante e depois da simulacao (requisito 1.3 do enunciado).
struct Task {

    // ---- Parametros lidos do arquivo de configuracao ----
    int id;
    sf::Color color;
    int arrivalTime;
    int totalDuration;
    int staticPriority;          // Usado pelo PRIOp / PRIOPEnv
    std::vector<std::string> rawEvents;   // Lista original (util para relatorio).
    std::vector<TaskAction>   actions;    // Lista parseada (usada em runtime).

    // ---- Estado em tempo de simulacao ----
    TaskState state = TaskState::NEW;
    SuspendReason suspendReason = SuspendReason::NONE;
    int remainingTime;
    int cpuAssigned = -1;        // -1 = nao esta em CPU
    int quantumTicksLeft = 0;

    // Tempo total (em ticks) em que a tarefa esteve em RUNNING.
    // Serve de "relogio local" para disparar as acoes de mutex/IO
    // (req 2.4 e 3.3 da parte B: instantes relativos ao inicio da tarefa).
    int cpuTimeConsumed = 0;
    std::size_t nextActionIndex = 0;

    // Prioridade dinamica usada pelo PRIOPEnv (envelhecimento).
    // Inicia igual a staticPriority; o OS atualiza a cada tick.
    int dynamicPriority = 0;

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
          remainingTime(_duration),
          dynamicPriority(_priority)
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

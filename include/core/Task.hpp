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
    TERMINATED   // Concluida (remaining_time chegou a zero)
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


//TCB (Task Control Block)
struct Task {

    //Params read from config file
    int id;                       
    sf::Color color;              
    int arrivalTime;             
    int totalDuration;           
    int staticPriority;  //Used in PROp's scheduler algorithm 


    std::vector<std::string> rawEvents;

    TaskState state = TaskState::NEW;
    SuspendReason suspendReason = SuspendReason::NONE;

    int remainingTime;

    int cpuAssigned = -1;  //The CPU that's running the task (-1 if not running)

    int quantumTicksLeft = 0;

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
          rawEvents(std::move(_events)),  // 'move' evita copiar o vetor
          remainingTime(_duration)
    {}
    bool isRunning()    const { return state == TaskState::RUNNING; }
    bool isReady()      const { return state == TaskState::READY; }
    bool isSuspended()  const { return state == TaskState::SUSPENDED; }
    bool isTerminated() const { return state == TaskState::TERMINATED; }

    bool hasArrived(int current_tick) const {
        return current_tick >= arrivalTime;
    }
};

}
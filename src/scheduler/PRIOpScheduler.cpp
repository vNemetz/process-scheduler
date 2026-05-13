#include "scheduler/PRIOpScheduler.hpp"

#include <cstdlib> 

namespace sim
{

// Retorna true se 'a' deve ser escolhido no lugar de 'b'.
bool PRIOpScheduler::beats(Task* a, Task* b, Task* running)
{   
    if (a->staticPriority > b->staticPriority) {
        return true;   
    } else if (a->staticPriority < b->staticPriority) {
        return false;  
    }
    
    // Criterios de desmparte

    if (a == running) {
        return true;    
    }
    if (b == running) {
        return false;   
    }

    if (a->arrivalTime < b->arrivalTime) {
        return true;
    } else if (a->arrivalTime > b->arrivalTime) {
        return false;
    }

    if (a->totalDuration < b->totalDuration) {
        return true;
    } else if (a->totalDuration > b->totalDuration) {
        return false;
    }

    return rand() % 2 == 0; // desempate aleatorio- ultimo criterio
}


Task* PRIOpScheduler::selectNextTask(std::vector<Task*>& readyQueue, Task* currentlyRunning, int currentTick)
{
    // Inclui a tarefa em execucao como candidata
    std::vector<Task*> candidates = readyQueue;
    if (currentlyRunning != nullptr)
        candidates.push_back(currentlyRunning);

    if (candidates.empty()) return nullptr;

    Task* selected = candidates[0];
    for (int i = 1; i < candidates.size(); i++) {
        if (beats(candidates[i], selected, currentlyRunning))
            selected = candidates[i];
    }
    return selected;
}

}
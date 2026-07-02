#pragma once

#include <deque>

namespace sim {

// Mutex simples usado nas acoes ML/MU (req 2 do Projeto B).
// - ownerTaskId = -1 significa livre.
// - Tarefas que tentam lock quando ha' owner vao para a fila FIFO
//   waitingTaskIds (mantida em ordem de chegada).
struct Mutex {
    int id;
    int ownerTaskId = -1;
    std::deque<int> waitingTaskIds;

    Mutex() : id(0) {}
    explicit Mutex(int _id) : id(_id) {}

    bool isFree() const { return ownerTaskId == -1; }
};

}  // namespace sim

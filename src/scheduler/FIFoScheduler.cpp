// ============================================================================
// FIFOScheduler.cpp — Implementacao do FIFO provisorio
// ============================================================================

#include "scheduler/FIFoScheduler.hpp"

#include <algorithm>

namespace sim {

Task* FIFOScheduler::selectNext(std::vector<Task*>& ready_queue,
                                Task* currently_running,
                                int /*current_tick*/)
{
    // Se ja ha uma tarefa rodando nesta CPU, mantenha-a (FIFO eh
    // nao-preemptivo). Ela so muda quando termina.
    if (currently_running != nullptr) {
        return currently_running;
    }

    // CPU ociosa + sem prontas = continua desligada.
    if (ready_queue.empty()) {
        return nullptr;
    }

    // Escolhe a tarefa com menor arrival_time (a que chegou primeiro).
    // std::min_element retorna iterador para o menor elemento segundo o
    // comparador fornecido. O(n), suficiente para nosso tamanho tipico.
    auto it = std::min_element(
        ready_queue.begin(), ready_queue.end(),
        [](const Task* a, const Task* b) {
            return a->arrival_time < b->arrival_time;
        }
    );

    return *it;
}

} // namespace sim
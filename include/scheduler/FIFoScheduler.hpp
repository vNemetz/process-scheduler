// ============================================================================
// FIFOScheduler.hpp — Escalonador FIFO (First-In, First-Out) PROVISORIO
// ============================================================================
// ATENCAO: este escalonador NAO atende ao requisito 4 do enunciado.
// Eh apenas a implementacao mais simples possivel da interface IScheduler,
// usada para VALIDAR o esqueleto do Simulator antes de implementar
// SRTF/PRIOp reais.
//
// FIFO eh nao-preemptivo: escolhe sempre a tarefa que esta ha mais tempo
// na fila de prontos. Eh basicamente "ordem de chegada".
//
// Sera substituido pelos escalonadores reais (SRTF, PRIOp) na proxima etapa.
// ============================================================================

#pragma once

#include "scheduler/IScheduler.hpp"

namespace sim {

class FIFOScheduler : public IScheduler {
public:
    // 'override' explicita que estamos sobrescrevendo um metodo virtual da
    // base. Boa pratica em C++11+: se errar a assinatura, o compilador avisa.
    Task* selectNext(std::vector<Task*>& ready_queue,
                     Task* currently_running,
                     int current_tick) override;

    std::string name() const override { return "FIFO (provisorio)"; }
};

} // namespace sim
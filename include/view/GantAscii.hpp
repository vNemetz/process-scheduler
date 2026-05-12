// ============================================================================
// GanttAscii.hpp — Renderizador de Gantt em texto puro
// ============================================================================
// Util para debug e validacao da logica de simulacao. Eh totalmente
// independente da SFML — funciona em qualquer terminal. Usaremos para
// TESTAR a simulacao antes de implementar a renderizacao grafica.
//
// Tambem fica util como "log compacto" durante a apresentacao: o
// professor consegue ver o que aconteceu olhando o terminal.
// ============================================================================

#pragma once

#include "core/Simulator.hpp"

#include <ostream>
#include <vector>

namespace sim {

class GanttAscii {
public:
    // Imprime o estado atual do sistema em uma linha.
    // Formato exemplo (2 CPUs, 4 tarefas):
    //   t=05  CPU0:T1  CPU1:T3  | T1:RUN T2:RDY T3:RUN T4:NEW
    static void printTickLine(std::ostream& os, const Simulator& sim);

    // Imprime o Gantt completo em tabela (chamado ao fim da simulacao).
    // Recebe um historico: para cada tick, qual tarefa estava em cada CPU.
    static void printFinalChart(std::ostream& os,
                                const std::vector<std::vector<int>>& history);

    // Imprime o relatorio final: tempos, metricas, ociosidade.
    static void printReport(std::ostream& os, const Simulator& sim);
};

} // namespace sim
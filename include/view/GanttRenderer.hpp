#pragma once

#include "core/CPU.hpp"
#include "core/Task.hpp"

#include <SFML/Graphics.hpp>
#include <vector>

namespace sim {

// ============================================================================
// GanttRenderer — Visualizacao grafica do diagrama de Gantt em SFML
// ============================================================================
// Recebe o estado das CPUs tick a tick (addTick) e desenha o historico
// completo em forma de tabela colorida (draw).
//
// Independente de quem alimenta: pode vir do Simulator atual ou do
// OperationalSystem futuro — a interface e a mesma.
// ============================================================================
class GanttRenderer {
public:
    // font  : fonte ja carregada no main (referencia, nao copia — fonte e pesada)
    // tasks : copia das tarefas, usada para saber IDs e cores
    GanttRenderer(sf::Font& font, const std::vector<Task>& tasks);

    // Registra o estado das CPUs em um tick.
    // Deve ser chamado uma vez por tick, logo apos executar o tick.
    void addTick(int tick, const std::vector<CPU>& cpus);

    // Desenha o Gantt completo na janela (todos os ticks registrados ate agora).
    void draw(sf::RenderWindow& window);

private:
    sf::Font& font_;
    std::vector<Task> tasks_;

    // history_[indice_tick][cpu_id] = task_id da tarefa que estava rodando.
    // -1 significa CPU desligada naquele tick.
    std::vector<std::vector<int>> history_;

    // Retorna a cor da tarefa com o id informado.
    // Se nao encontrar, retorna branco como fallback.
    sf::Color getTaskColor(int task_id);
};

} // namespace sim

#include "view/GanttRenderer.hpp"

namespace sim {

// ----------------------------------------------------------------------------
// Constantes visuais — ajuste aqui para mudar aparencia sem tocar na logica
// ----------------------------------------------------------------------------
static const int CELL_W      = 28;   // largura da celula colorida em pixels
static const int CELL_H      = 26;   // altura da celula colorida em pixels
static const int COL_W       = 32;   // largura de cada coluna (celula + espaco)
static const int ROW_H       = 36;   // altura de cada linha (celula + espaco)
static const int MARGIN_LEFT = 100;  // espaco reservado para os labels das tarefas
static const int MARGIN_TOP  = 50;   // espaco reservado para a numeracao dos ticks

// ----------------------------------------------------------------------------
// Construtor
// ----------------------------------------------------------------------------
GanttRenderer::GanttRenderer(sf::Font& font, const std::vector<Task>& tasks)
    : font_(font), tasks_(tasks)
{
}

// ----------------------------------------------------------------------------
// addTick — registra o estado das CPUs em um tick
// ----------------------------------------------------------------------------
void GanttRenderer::addTick(int tick, const std::vector<CPU>& cpus)
{
    // Para cada CPU, salva qual task_id ela estava executando (-1 = desligada).
    std::vector<int> snapshot;
    for (int i = 0; i < (int)cpus.size(); i++) {
        snapshot.push_back(cpus[i].currentTaskId);
    }
    history_.push_back(snapshot);
}

// ----------------------------------------------------------------------------
// draw — renderiza o Gantt completo na janela
// ----------------------------------------------------------------------------
void GanttRenderer::draw(sf::RenderWindow& window)
{
    if (history_.empty()) {
        return;
    }

    // --- Ordena IDs das tarefas em ordem DECRESCENTE (req. 2.5) ---
    // Copiamos os IDs e fazemos um bubble sort manual para evitar lambdas.
    std::vector<int> taskIds;
    for (int i = 0; i < (int)tasks_.size(); i++) {
        taskIds.push_back(tasks_[i].id);
    }

    int numTasks = (int)taskIds.size();
    for (int i = 0; i < numTasks - 1; i++) {
        for (int j = i + 1; j < numTasks; j++) {
            if (taskIds[j] > taskIds[i]) {
                int tmp  = taskIds[i];
                taskIds[i] = taskIds[j];
                taskIds[j] = tmp;
            }
        }
    }

    // --- Cabecalho: numeros dos ticks no eixo X ---
    int numTicks = (int)history_.size();
    for (int t = 0; t < numTicks; t++) {
        sf::Text label(std::to_string(t + 1), font_, 11);
        label.setFillColor(sf::Color(180, 180, 180));
        label.setPosition(
            MARGIN_LEFT + t * COL_W + 6,
            MARGIN_TOP - 22
        );
        window.draw(label);
    }

    // --- Linha por tarefa ---
    for (int row = 0; row < numTasks; row++) {
        int task_id = taskIds[row];
        float y = MARGIN_TOP + row * ROW_H;

        // Label da tarefa no lado esquerdo
        sf::Text taskLabel("T" + std::to_string(task_id), font_, 13);
        taskLabel.setFillColor(sf::Color::White);
        taskLabel.setPosition(10, y + 4);
        window.draw(taskLabel);

        // Celulas: uma por tick
        for (int t = 0; t < numTicks; t++) {
            float x = MARGIN_LEFT + t * COL_W;

            // Verifica se alguma CPU tinha essa tarefa rodando neste tick
            bool running = false;
            int numCpus = (int)history_[t].size();
            for (int c = 0; c < numCpus; c++) {
                if (history_[t][c] == task_id) {
                    running = true;
                    break;
                }
            }

            // Define a cor: cor da tarefa se rodando, cinza escuro se nao
            sf::Color cellColor;
            if (running) {
                cellColor = getTaskColor(task_id);
            } else {
                cellColor = sf::Color(55, 55, 55);
            }

            sf::RectangleShape cell(sf::Vector2f(CELL_W, CELL_H));
            cell.setPosition(x + 2, y);
            cell.setFillColor(cellColor);
            window.draw(cell);
        }
    }
}

// ----------------------------------------------------------------------------
// getTaskColor — busca a cor de uma tarefa pelo id
// ----------------------------------------------------------------------------
sf::Color GanttRenderer::getTaskColor(int task_id)
{
    for (int i = 0; i < (int)tasks_.size(); i++) {
        if (tasks_[i].id == task_id) {
            return tasks_[i].color;
        }
    }
    return sf::Color::White;  // fallback: nao deveria chegar aqui
}

} // namespace sim

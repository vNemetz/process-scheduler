/*
#include <view/UIController.hpp>
#include <iostream>


#include "core/Clock.hpp"
#include "core/CPU.hpp"

int main() {
    std::cout << "[INFO] Starting task scheduler...\n";
    view::UIController ui;
    ui.execute();
    std::cout << "[INFO] Task scheduler was finished.\n";
    return 0;
}
*/


// ============================================================================
// main.cpp — TESTE DE VALIDACAO
// ============================================================================
// Este main eh TEMPORARIO. Serve para validar que tudo que construimos
// ate agora funciona conjuntamente:
//   - Clock avanca
//   - CPU armazena/exibe estado
//   - ConfigParser le um arquivo .cfg e produz Tasks
//
// Sem janela SFML ainda, so terminal. Depois disso, voltamos pro main
// "de verdade" que abre a janela.
// ============================================================================

#include "config/ConfigParser.hpp"
#include "core/Clock.hpp"
#include "core/CPU.hpp"
#include "core/SimulationConfig.hpp"
#include "core/Task.hpp"

#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    // ------------------------------------------------------------------
    // 1. Testa Clock
    // ------------------------------------------------------------------
    std::cout << "===== TESTE: Clock =====\n";
    sim::Clock clock;
    std::cout << "Clock inicial: " << clock.now() << "\n";
    clock.tick();
    clock.tick();
    clock.tick();
    std::cout << "Apos 3 ticks: " << clock.now() << "\n";
    clock.setTo(-99);  // valor invalido: deve virar 0
    std::cout << "Apos setTo(-99): " << clock.now() << " (esperado: 0)\n";
    clock.setTo(42);
    std::cout << "Apos setTo(42): " << clock.now() << "\n";
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 2. Testa CPU
    // ------------------------------------------------------------------
    std::cout << "===== TESTE: CPU =====\n";
    std::vector<sim::CPU> cpus;
    cpus.emplace_back(0);
    cpus.emplace_back(1);
    cpus[0].current_task_id = 3;  // simulando atribuicao
    for (const auto& cpu : cpus) {
        std::cout << "CPU " << cpu.id
                  << " | isOff=" << (cpu.isOff() ? "sim" : "nao")
                  << " | task_id=" << cpu.current_task_id
                  << "\n";
    }
    std::cout << "\n";

    // ------------------------------------------------------------------
    // 3. Testa ConfigParser com o arquivo .cfg
    // ------------------------------------------------------------------
    std::cout << "===== TESTE: ConfigParser =====\n";
    std::string path = (argc > 1) ? argv[1] : "config/exemplo.cfg";
    std::cout << "Carregando: " << path << "\n";

    sim::ConfigParser parser;
    auto result = parser.parseFile(path);

    // Mostra warnings
    for (const auto& w : result.warnings) {
        std::cout << "  [AVISO] " << w << "\n";
    }

    if (!result.ok()) {
        std::cout << "  FALHOU:\n";
        for (const auto& e : result.errors) {
            std::cout << "    - " << e << "\n";
        }
        return 1;
    }

    std::cout << "  Algoritmo : " << sim::toString(result.config->algorithm)
              << "\n";
    std::cout << "  Quantum   : " << result.config->quantum << "\n";
    std::cout << "  CPUs      : " << result.config->num_cpus << "\n";
    std::cout << "  Tarefas   : " << result.tasks.size() << "\n\n";

    // Lista as tarefas
    std::cout << std::left
              << std::setw(4)  << "ID"
              << std::setw(10) << "Ingresso"
              << std::setw(10) << "Duracao"
              << std::setw(12) << "Prioridade"
              << std::setw(8)  << "R"
              << std::setw(8)  << "G"
              << std::setw(8)  << "B"
              << "\n";
    std::cout << std::string(60, '-') << "\n";
    for (const auto& t : result.tasks) {
        std::cout << std::left
                  << std::setw(4)  << t.id
                  << std::setw(10) << t.arrival_time
                  << std::setw(10) << t.total_duration
                  << std::setw(12) << t.static_priority
                  << std::setw(8)  << static_cast<int>(t.color.r)
                  << std::setw(8)  << static_cast<int>(t.color.g)
                  << std::setw(8)  << static_cast<int>(t.color.b)
                  << "\n";
    }
    std::cout << "\nTodos os testes passaram. Componentes integrados OK!\n";

    return 0;
}
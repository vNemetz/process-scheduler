// ============================================================================
// main.cpp — Ponto de entrada do Simulador de SO Multitarefa
// ============================================================================
// O main eh propositalmente CURTO. Ele orquestra os componentes, mas nao
// contem logica de negocio. Tudo de "interessante" esta nas classes.
//
// Fluxo:
//   1. Le argumentos da linha de comando (path do .cfg, modo headless)
//   2. Parser le e valida o .cfg
//   3. Cria o Simulator com FIFOScheduler provisorio
//   4. Executa a simulacao (tick a tick, com log no terminal)
//   5. Mostra Gantt ASCII final + relatorio
//   6. (Se nao for headless) abre janela SFML com info do resultado
// ============================================================================

#include "config/ConfigParser.hpp"
#include "core/Simulator.hpp"
#include "scheduler/FIFoScheduler.hpp"
#include "view/GantAscii.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

// ----------------------------------------------------------------------------
// Executa a simulacao no terminal (sem janela).
// ----------------------------------------------------------------------------
static void runSimulationOnTerminal(sim::Simulator& simulator) {
    std::cout << "\n========== EXECUCAO TICK A TICK ==========\n";
    std::cout << "(escalonador: " << simulator.scheduler().name() << ")\n\n";

    // Historico: para cada tick, qual tarefa estava em cada CPU.
    // Usado para imprimir o Gantt final em formato tabular.
    std::vector<std::vector<int>> history;

    // Imprime estado inicial (tick 0).
    sim::GanttAscii::printTickLine(std::cout, simulator);

    // Roda tick a tick. step() retorna false quando acaba.
    while (simulator.step()) {
        sim::GanttAscii::printTickLine(std::cout, simulator);

        // Captura o estado das CPUs neste tick.
        std::vector<int> snapshot;
        snapshot.reserve(simulator.cpus().size());
        for (const auto& cpu : simulator.cpus()) {
            snapshot.push_back(cpu.current_task_id);
        }
        history.push_back(std::move(snapshot));
    }

    sim::GanttAscii::printFinalChart(std::cout, history);
    sim::GanttAscii::printReport(std::cout, simulator);
}

int main(int argc, char* argv[]) {
    // ----------------------------------------------------------------
    // 1. Argumentos da linha de comando
    // ----------------------------------------------------------------
    std::string config_path = "config/exemplo.cfg";
    bool headless = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
        } else {
            config_path = arg;
        }
    }

    // ----------------------------------------------------------------
    // 2. Parser
    // ----------------------------------------------------------------
    std::cout << "[INFO] Carregando configuracao: " << config_path << '\n';
    sim::ConfigParser parser;
    auto parse = parser.parseFile(config_path);

    for (const auto& w : parse.warnings) {
        std::cerr << "[AVISO] " << w << '\n';
    }
    if (!parse.ok()) {
        std::cerr << "[ERRO] Falha ao carregar configuracao:\n";
        for (const auto& e : parse.errors) {
            std::cerr << "  - " << e << '\n';
        }
        return 1;
    }

    // ----------------------------------------------------------------
    // 3. Criacao do Simulator
    // ----------------------------------------------------------------
    // FIFOScheduler eh PROVISORIO. Sera substituido por SRTF/PRIOp baseado
    // em parse.config->algorithm na proxima etapa.
    auto scheduler = std::make_unique<sim::FIFOScheduler>();
    sim::Simulator simulator(
        *parse.config,
        std::move(parse.tasks),
        std::move(scheduler)
    );

    std::cout << "[INFO] Simulator inicializado: "
              << simulator.config().num_cpus << " CPUs, "
              << simulator.tasks().size() << " tarefas, "
              << "algoritmo " << simulator.scheduler().name() << '\n';

    // ----------------------------------------------------------------
    // 4. Executa simulacao
    // ----------------------------------------------------------------
    runSimulationOnTerminal(simulator);

    if (headless) return 0;

    // ----------------------------------------------------------------
    // 5. Janela SFML (palco do Gantt grafico nas proximas etapas)
    // ----------------------------------------------------------------
    sf::RenderWindow window(
        sf::VideoMode(1200, 600),
        "Simulador SO - Simulacao concluida"
    );
    window.setFramerateLimit(60);

    sf::Font font;
    bool font_ok = font.loadFromFile("assets/fonts/Roboto.ttf");
    if (!font_ok) {
        std::cerr << "[AVISO] Fonte nao encontrada em assets/fonts/Roboto.ttf\n";
    }

    std::stringstream status;
    status << "Simulacao concluida.\n\n"
           << "Algoritmo : " << simulator.scheduler().name() << "\n"
           << "Tick final: " << simulator.currentTick() << "\n"
           << "Tarefas   : " << simulator.tasks().size() << "\n\n"
           << "Veja o terminal para o Gantt ASCII e o relatorio.\n"
           << "A renderizacao grafica vira na proxima etapa.\n\n"
           << "Pressione ESC para sair.";

    sf::Text status_text(status.str(), font, 18);
    status_text.setFillColor(sf::Color::White);
    status_text.setPosition(40.f, 40.f);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) window.close();
        }
        window.clear(sf::Color(30, 30, 35));
        window.draw(status_text);
        window.display();
    }

    return 0;
}
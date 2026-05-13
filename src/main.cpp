// ============================================================================
// main.cpp — Ponto de entrada do Simulador de SO Multitarefa
// ============================================================================
// Fluxo:
//   1. Le argumentos da linha de comando (path do .cfg, modo headless)
//   2. Parser le e valida o .cfg
//   3. Cria o Simulator
//   4. Carrega fonte e cria o GanttRenderer (antes da simulacao!)
//   5. Executa a simulacao — alimenta terminal E renderer ao mesmo tempo
//   6. Abre janela SFML e entra no loop de desenho
// ============================================================================

#include "config/ConfigParser.hpp"
#include "core/Simulator.hpp"
#include "scheduler/FIFoScheduler.hpp"
#include "view/GantAscii.hpp"
#include "view/GanttRenderer.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <vector>

// ----------------------------------------------------------------------------
// Executa a simulacao tick a tick, alimentando o terminal e o renderer.
// ----------------------------------------------------------------------------
static void runSimulation(sim::Simulator& simulator, sim::GanttRenderer& renderer) {
    std::cout << "\n========== EXECUCAO TICK A TICK ==========\n";
    std::cout << "(escalonador: " << simulator.scheduler().name() << ")\n\n";

    std::vector<std::vector<int>> history;

    sim::GanttAscii::printTickLine(std::cout, simulator);

    while (simulator.step()) {
        sim::GanttAscii::printTickLine(std::cout, simulator);

        // Alimenta o GanttRenderer com o estado atual das CPUs.
        renderer.addTick(simulator.currentTick(), simulator.cpus());

        // Captura historico para o Gantt ASCII final.
        std::vector<int> snapshot;
        for (int i = 0; i < (int)simulator.cpus().size(); i++) {
            snapshot.push_back(simulator.cpus()[i].currentTaskId);
        }
        history.push_back(snapshot);
    }

    sim::GanttAscii::printFinalChart(std::cout, history);
    sim::GanttAscii::printReport(std::cout, simulator);
}

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------
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

    for (int i = 0; i < (int)parse.warnings.size(); i++) {
        std::cerr << "[AVISO] " << parse.warnings[i] << '\n';
    }
    if (!parse.ok()) {
        std::cerr << "[ERRO] Falha ao carregar configuracao:\n";
        for (int i = 0; i < (int)parse.errors.size(); i++) {
            std::cerr << "  - " << parse.errors[i] << '\n';
        }
        return 1;
    }

    // ----------------------------------------------------------------
    // 3. Criacao do Simulator
    // ----------------------------------------------------------------
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
    // 4. Fonte e GanttRenderer
    // ----------------------------------------------------------------
    // A fonte eh carregada aqui — antes da simulacao — porque o renderer
    // precisa existir durante o loop de execucao para receber os ticks.
    sf::Font font;
    if (!font.loadFromFile("assets/fonts/tuffy.ttf")) {
        std::cerr << "[AVISO] Fonte nao encontrada em assets/fonts/tuffy.ttf\n";
    }

    sim::GanttRenderer renderer(font, simulator.tasks());

    // ----------------------------------------------------------------
    // 5. Executa simulacao
    // ----------------------------------------------------------------
    runSimulation(simulator, renderer);

    if (headless) return 0;

    // ----------------------------------------------------------------
    // 6. Janela SFML — loop de desenho
    // ----------------------------------------------------------------
    sf::RenderWindow window(
        sf::VideoMode(1200, 700),
        "Simulador SO - Gantt"
    );
    window.setFramerateLimit(60);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) window.close();
        }

        window.clear(sf::Color(30, 30, 35));
        renderer.draw(window);
        window.display();
    }

    return 0;
}

// ============================================================================
// main.cpp — Ponto de entrada do Simulador de SO
// ============================================================================
// PASSO 1: este eh apenas um "Hello World" para validar que:
//   - O CMakeLists.txt compila o projeto corretamente.
//   - A biblioteca SFML foi baixada, compilada e linkada.
//   - Uma janela SFML abre na nossa maquina.
//
// Quando essa janela aparecer rodando, sabemos que toda a infraestrutura
// esta funcionando, e podemos comecar a construir o simulador de verdade.
//
// Este arquivo sera completamente reescrito nos proximos passos.
// ============================================================================
#include <core/Task.hpp>

#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
    std::cout << "[INFO] Iniciando Simulador SO...\n";

    // ------------------------------------------------------------------
    // Cria a janela SFML
    // ------------------------------------------------------------------
    // sf::VideoMode(largura, altura) define o tamanho em pixels.
    // 1200x600 eh um bom ponto de partida para o Gantt futuro.
    sf::RenderWindow window(
        sf::VideoMode(1200, 600),
        "Simulador SO - Hello World"
    );

    // Limita o framerate em 60 FPS:
    //   - Padrao de monitor (sem tearing)
    //   - Reduz uso de CPU (nao faz sentido renderizar 1000fps numa simulacao)
    window.setFramerateLimit(60);

    // ------------------------------------------------------------------
    // Loop principal de eventos
    // ------------------------------------------------------------------
    // Padrao classico de qualquer aplicacao grafica:
    //   while (janela aberta):
    //     1. Processar eventos (teclado, mouse, fechar)
    //     2. Atualizar estado da aplicacao
    //     3. Limpar tela
    //     4. Desenhar
    //     5. Apresentar (double buffering)
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            // Usuario clicou no X da janela
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            // Usuario pressionou ESC
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
        }

        // Cor de fundo cinza-escuro. Escolha justificada:
        //  - Reduz fadiga visual em apresentacoes
        //  - Faz as cores das tarefas (que o usuario define no .cfg) se
        //    destacarem mais quando comecarmos a desenhar o Gantt
        window.clear(sf::Color(30, 30, 35));

        // (Nada para desenhar ainda — janela ficara em branco escuro)

        // Troca o buffer back pelo front
        window.display();
    }

    std::cout << "[INFO] Simulador SO encerrado.\n";
    return 0;
}
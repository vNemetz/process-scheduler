#include "view/UIController.hpp"
#include <cmath>
#include <iostream>

namespace view {

sf::Vector2f UIController::mapToScreen(float x, float y, const view::PlotArea& p) {
    float nx = (x - p.xmin) / (p.xmax - p.xmin);
    float ny = (y - p.ymin) / (p.ymax - p.ymin);
    return {
        p.rect.left + nx * p.rect.width,
        p.rect.top + (1.0f - ny) * p.rect.height
    };
}

// Construtor
UIController::UIController(const std::vector<sim::GlobalState>& history, 
                           const std::vector<sim::Task>& initialTasks)
    : window(sf::VideoMode(1200, 600), "Task Scheduler Simulator - Gantt Chart"),
      historyData(history),
      currentTimeIndex(0) 
{
    window.setFramerateLimit(60);

    if (!font.loadFromFile("/config/Roboto-Regular.ttf")) {
        std::cerr << "Could not get font for labels\n";
    }

    // Salva as cores das tarefas em um mapa para acesso rápido pelo ID
    int maxTaskId = 0;
    for (int i = 0; i < initialTasks.size(); i++) {
        taskColors[initialTasks[i].id] = initialTasks[i].color;
        if (initialTasks[i].id > maxTaskId) {
            maxTaskId = initialTasks[i].id;
        }
    }

    // Configura a área do gráfico dinamicamente
    float maxTime = historyData.empty() ? 10.0f : static_cast<float>(historyData.size());
    plot = {
        {80.f, 60.f, 1000.f, 460.f}, 
        0.0f, maxTime,             // X: de 0 até o tempo máximo da simulação
        0.0f, maxTaskId + 1.0f     // Y: de 0 até o maior ID de tarefa + um respiro
    };
}

void UIController::execute() {
    // Começa mostrando o final da simulação (tudo desenhado)
    if (!historyData.empty()) {
        currentTimeIndex = historyData.size() - 1;
    }

    while (window.isOpen()) {
        processEvents();
        render();
    }
}

void UIController::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        
        // A nossa "Máquina do Tempo" - Controle pelas Setas
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
            else if (event.key.code == sf::Keyboard::Right) {
                // Avança no tempo
                if (currentTimeIndex < historyData.size() - 1) {
                    currentTimeIndex++;
                }
            }
            else if (event.key.code == sf::Keyboard::Left) {
                // Retrocede no tempo
                if (currentTimeIndex > 0) {
                    currentTimeIndex--;
                }
            }
        }
    }
}

void UIController::render() {
    window.clear(sf::Color(30, 30, 35));

    // Desenhamos a grade baseada no tamanho dos dados
    int xTicks = plot.xmax; 
    int yTicks = plot.ymax;
    drawGrid(window, plot, xTicks, yTicks);
    drawAxes(window, plot);
    
    drawLabels(window, plot);
    // Desenha as barras de Gantt
    drawGantt(window, plot);

    window.display();
}

void UIController::drawLabels(sf::RenderTarget& target, const view::PlotArea& p) {
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(14); // Tamanho da letra
    text.setFillColor(sf::Color(200, 200, 200));

    // 1. Desenhar Eixo X (Tempo / Quantuns)
    // Vamos de 0 até o tempo máximo da simulação
    int maxTime = static_cast<int>(p.xmax);
    for (int t = 0; t <= maxTime; ++t) {
        text.setString(std::to_string(t));
        
        // Pega a posição na tela equivalente ao tempo 't' e Y = 0 (linha de baixo)
        sf::Vector2f pos = mapToScreen(static_cast<float>(t), 0.0f, p);
        
        // Ajusta a posição para ficar um pouco abaixo da linha do eixo X
        // e centralizado na marcação
        text.setPosition(pos.x - text.getLocalBounds().width / 2.0f, pos.y + 10.0f);
        
        target.draw(text);
    }

    // 2. Desenhar Eixo Y (IDs das Tarefas)
    // Vamos iterar pelas tarefas cadastradas no mapa de cores
    std::map<int, sf::Color>::iterator it;
    for (it = taskColors.begin(); it != taskColors.end(); ++it) {
        int taskId = it->first;
        
        text.setString("T" + std::to_string(taskId)); // Ex: "T1", "T2"
        
        // Pega a posição na tela: X = 0 (linha da esquerda), Y = ID da tarefa
        sf::Vector2f pos = mapToScreen(0.0f, static_cast<float>(taskId), p);
        
        // Ajusta a posição para ficar à esquerda do eixo Y
        // Centraliza verticalmente subtraindo metade da altura da letra
        text.setPosition(pos.x - text.getLocalBounds().width - 15.0f, 
                         pos.y - text.getLocalBounds().height / 2.0f - 5.0f);
        
        target.draw(text);
    }
    
    // Títulos dos Eixos (Opcional, mas fica bonito)
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);
    
    // Título do Eixo X
    text.setString("Tempo (Ticks/Quantuns)");
    text.setPosition(p.rect.left + p.rect.width / 2.0f - 80.0f, p.rect.top + p.rect.height + 35.0f);
    target.draw(text);
    
    // Título do Eixo Y
    text.setString("Tasks");
    text.setPosition(p.rect.left - 60.0f, p.rect.top - 25.0f);
    target.draw(text);
}

void UIController::drawGantt(sf::RenderTarget& target, const view::PlotArea& p) {
    if (historyData.empty()) return;

    // Percorre a simulação do instante 0 até o momento atual que o usuário selecionou nas setinhas
    for (int t = 0; t <= currentTimeIndex; ++t) {
        const sim::GlobalState& state = historyData[t];

        // Para cada CPU, verifica qual tarefa estava rodando
        for (int i = 0; i < state.cpus.size(); i++) {
            int taskId = state.cpus[i].currentTaskId;

            if (taskId != -1) { // -1 significa CPU ociosa
                // Acha a cor da tarefa (se não achar, usa branco)
                sf::Color blockColor = sf::Color::White;
                if (taskColors.find(taskId) != taskColors.end()) {
                    blockColor = taskColors[taskId];
                }

                // O bloco dura 1 unidade de tempo (de t até t+1)
                // Vamos desenhar ele na altura correspondente ao seu ID (eixo Y)
                float yCenter = static_cast<float>(taskId);
                
                // Mapeia os cantos do bloco para coordenadas da tela
                sf::Vector2f bottomLeft = mapToScreen(t, yCenter - 0.4f, p);
                sf::Vector2f topRight = mapToScreen(t + 1, yCenter + 0.4f, p);

                // No SFML, o Y cresce para baixo, então a altura é a subtração do bottom pelo top
                float width = topRight.x - bottomLeft.x;
                float height = bottomLeft.y - topRight.y;

                sf::RectangleShape rect(sf::Vector2f(width, height));
                rect.setPosition(bottomLeft.x, topRight.y); // Posição no SFML é o Top-Left
                rect.setFillColor(blockColor);
                
                // Bordas leves para separar blocos consecutivos da mesma cor
                rect.setOutlineThickness(1.0f);
                rect.setOutlineColor(sf::Color(20, 20, 25));

                target.draw(rect);
            }
        }
    }
}

// Suas funções de grid e eixos permanecem iguais, apenas com ajustes nos tipos
void UIController::drawGrid(sf::RenderTarget& target, const view::PlotArea& p, int xTicks, int yTicks) {
    sf::VertexArray grid(sf::Lines);
    if(xTicks <= 0) xTicks = 1;
    if(yTicks <= 0) yTicks = 1;

    for (int i = 0; i <= xTicks; ++i) {
        float t = static_cast<float>(i) / xTicks;
        float x = p.rect.left + t * p.rect.width;
        grid.append(sf::Vertex({x, p.rect.top}, sf::Color(60, 60, 70)));
        grid.append(sf::Vertex({x, p.rect.top + p.rect.height}, sf::Color(60, 60, 70)));
    }
    for (int j = 0; j <= yTicks; ++j) {
        float t = static_cast<float>(j) / yTicks;
        float y = p.rect.top + t * p.rect.height;
        grid.append(sf::Vertex({p.rect.left, y}, sf::Color(60, 60, 70)));
        grid.append(sf::Vertex({p.rect.left + p.rect.width, y}, sf::Color(60, 60, 70)));
    }
    target.draw(grid);
}

void UIController::drawAxes(sf::RenderTarget& target, const view::PlotArea& p) {
    sf::VertexArray axes(sf::Lines, 4);

    float x0 = p.rect.left; // Agora os eixos começam no canto inferior esquerdo
    float y0 = p.rect.top + p.rect.height;

    axes[0] = sf::Vertex({p.rect.left, y0}, sf::Color::White);
    axes[1] = sf::Vertex({p.rect.left + p.rect.width, y0}, sf::Color::White);
    axes[2] = sf::Vertex({x0, p.rect.top}, sf::Color::White);
    axes[3] = sf::Vertex({x0, p.rect.top + p.rect.height}, sf::Color::White);

    target.draw(axes);

    sf::RectangleShape border({p.rect.width, p.rect.height});
    border.setPosition({p.rect.left, p.rect.top});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(1.0f);
    border.setOutlineColor(sf::Color(120, 120, 130));
    target.draw(border);
}

// Auxiliar para as cores do arquivo config
sf::Color UIController::parseHexColor(const std::string& hexStr) {
    if (hexStr.length() >= 7 && hexStr[0] == '#') {
        int r = std::stoi(hexStr.substr(1, 2), nullptr, 16);
        int g = std::stoi(hexStr.substr(3, 2), nullptr, 16);
        int b = std::stoi(hexStr.substr(5, 2), nullptr, 16);
        return sf::Color(r, g, b);
    }
    return sf::Color::White; // Fallback
}

}
#pragma once

#include <SFML/Graphics.hpp>
#include <map>
#include <string>
#include <vector>

#include "core/OperatingSystem.hpp"
#include "core/Task.hpp"

namespace view {

// Area de plot reutilizavel (coordenadas mundo + retangulo na tela).
struct PlotArea {
    sf::FloatRect rect;
    float xmin;
    float xmax;
    float ymin;
    float ymax;
};

// Modo de execucao da UI.
enum class UIMode {
    STEP,      // Manual: usuario controla cada tick
    AUTO_PLAY  // Auto-avanco a cada N frames
};

// Controlador grafico do simulador.
// Responsabilidades:
//   - Renderizar o Gantt do historico (req 2.1, 2.2, 2.5).
//   - Dirigir executeOneTick quando o usuario pede passo (req 1.5.a).
//   - Permitir edicao manual de estado das tarefas (req 1.5.2, 3.4).
//   - Exportar imagem do Gantt final (req 2.3).
//   - Mostrar legenda, info por tarefa, marcador de sorteio (req 4.3 item 4).
class UIController {
public:
    UIController(sim::OperatingSystem* os,
                 const std::vector<sim::Task>& initialTasks);

    void execute();

private:
    // ---- Recursos SFML ----
    sf::RenderWindow window;
    sf::Font font;

    // ---- Areas da tela ----
    PlotArea gantt;        // Area do grafico de Gantt
    sf::FloatRect topBar;  // Barra superior (info + algoritmo + tick)
    sf::FloatRect sideBar; // Painel lateral (legenda + info)
    sf::FloatRect botBar;  // Rodape (hotkeys)

    // ---- Modelo ----
    sim::OperatingSystem* os;
    std::map<int, sf::Color> taskColors;
    std::map<int, sim::Task> initialTasksById;  // Para mostrar "ingresso" mesmo apos consumo
    std::map<int, int> taskRows;
    int maxTaskId;

    // ---- Estado da UI ----
    int currentTimeIndex;     // Posicao no historico (0 = tick 0)
    UIMode mode;
    bool isPlaying;           // Auto-play ligado?
    bool showHelp;            // Overlay F1 visivel?
    int  framesPerStep;       // No auto-play, 1 step a cada N frames
    int  frameCounter;        // Contador do auto-play
    int  selectedTaskId;      // Tarefa selecionada para edicao (-1 = nenhuma)
    std::string statusMessage;
    int statusMessageFramesLeft;

    // ---- Loop ----
    void processEvents();
    void update();
    void render();

    // ---- Acoes ----
    void stepForward();          // Avanca 1 tick (executeOneTick se no fim)
    void stepBackward();         // Volta 1 tick no historico
    void runToEnd();             // Modo completo (req 1.5.b sob demanda)
    void togglePlay();
    void goToStart();
    void goToEnd();
    void resetSimulation();      // Volta o cursor pro tick 0 (mantem historico)
    void exportPng();            // Salva gantt.png
    void cycleSelectedTaskState();  // Edicao manual
    void selectTaskByDigit(int digit);

    // ---- Geometria / utilitarios ----
    sf::Vector2f mapToScreen(float x, float y, const PlotArea& p) const;
    void updateViewWindow();     // Faz scroll horizontal pra acompanhar cursor

    bool atLiveTick() const;     // currentTimeIndex == ultimo do historico?
    int rowForTask(int taskId) const;
    const sim::GlobalState* currentSnapshot() const;
    void setStatus(const std::string& msg);

    // ---- Desenho ----
    void drawTopBar(sf::RenderTarget& t);
    void drawBotBar(sf::RenderTarget& t);
    void drawSideBar(sf::RenderTarget& t);
    void drawCpuStatus(sf::RenderTarget& t, float& y, const sim::GlobalState* snap);
    void drawHelpOverlay(sf::RenderTarget& t);
    void drawGantt(sf::RenderTarget& t, const PlotArea& p, bool drawCursor);
    void drawGrid(sf::RenderTarget& t, const PlotArea& p, int xTicks, int yTicks);
    void drawAxes(sf::RenderTarget& t, const PlotArea& p);
    void drawLabels(sf::RenderTarget& t, const PlotArea& p);
    void drawLegendInsideGantt(sf::RenderTarget& t, const PlotArea& p);

    // Decorators do Gantt
    void drawBlock(sf::RenderTarget& t, const PlotArea& p,
                   int tick, int taskId, int cpuId,
                   const sf::Color& color, bool isLottery);
    void drawSuspendedBlock(sf::RenderTarget& t, const PlotArea& p,
                            int tick, int taskId, sim::SuspendReason reason);
    void drawReadySlot(sf::RenderTarget& t, const PlotArea& p,
                       int tick, int taskId);
    void drawArrivalIcon(sf::RenderTarget& t, const PlotArea& p,
                         int tick, int taskId);
    void drawTerminationIcon(sf::RenderTarget& t, const PlotArea& p,
                             int tick, int taskId);
    // Marcadores das acoes de mutex/IO no bloco correspondente (Projeto B).
    void drawActionMarker(sf::RenderTarget& t, const PlotArea& p,
                          int tick, int taskId, const sim::ActionEvent& ev);

    // PNG export = renderiza para uma RenderTexture sem janela visivel.
    void renderGanttToTarget(sf::RenderTarget& t, const PlotArea& p, int firstTick, int lastTick);
};

}  // namespace view

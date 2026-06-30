#include "view/UIController.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace view {

// --------------------------------------------------------------------------
// Constantes de layout
// --------------------------------------------------------------------------
static const int   WINDOW_W = 1400;
static const int   WINDOW_H = 760;
static const float TOP_H    = 50.0f;
static const float BOT_H    = 90.0f;
static const float SIDE_W   = 320.0f;
static const float MARGIN   = 20.0f;

static const float TICK_WIDTH        = 34.0f;
static const int   AUTO_FRAMES_STEP  = 8;    // 60fps / 8 ≈ 7,5 ticks/s
static const int   STATUS_MSG_FRAMES = 180;  // ~3s de mensagem

// Cores do tema (escuro com acentos).
static const sf::Color BG_COLOR        (28, 30, 38);
static const sf::Color PANEL_COLOR     (40, 44, 56);
static const sf::Color GRID_COLOR      (55, 60, 75);
static const sf::Color AXIS_COLOR      (200, 200, 210);
static const sf::Color TEXT_COLOR      (220, 220, 230);
static const sf::Color TEXT_DIM        (150, 152, 165);
static const sf::Color CURSOR_COLOR    (255, 215, 0);
static const sf::Color SUSPENDED_COLOR (15, 15, 18);     // Preto p/ suspensa (req 2.1)
static const sf::Color READY_BORDER    (120, 130, 150);

// --------------------------------------------------------------------------
// Construtor
// --------------------------------------------------------------------------
UIController::UIController(sim::OperatingSystem* osPtr,
                           const std::vector<sim::Task>& initialTasks)
    : window(sf::VideoMode(WINDOW_W, WINDOW_H),
             "Simulador SO - Escalonamento de Tarefas"),
      os(osPtr),
      maxTaskId(0),
      currentTimeIndex(0),
      mode(UIMode::STEP),
      isPlaying(false),
      framesPerStep(AUTO_FRAMES_STEP),
      frameCounter(0),
      selectedTaskId(-1),
      statusMessageFramesLeft(0)
{
    window.setFramerateLimit(60);

    if (!font.loadFromFile("../assets/fonts/Roboto-Regular.ttf")) {
        // Fallback para outro caminho comum (quando rodando do build/).
        if (!font.loadFromFile("assets/fonts/Roboto-Regular.ttf")) {
            std::cerr << "[UI] Aviso: nao foi possivel carregar a fonte.\n";
        }
    }

    for (const auto& t : initialTasks) {
        taskColors[t.id] = t.color;
        initialTasksById[t.id] = t;
        if (t.id > maxTaskId) maxTaskId = t.id;
    }

    // Define as areas da tela. Gantt fica entre a top bar e a bot bar,
    // com a sideBar a direita. SIDE_W controla a largura do painel lateral.
    topBar  = { 0.0f,         0.0f,                          (float)WINDOW_W, TOP_H };
    botBar  = { 0.0f,         (float)WINDOW_H - BOT_H,        (float)WINDOW_W, BOT_H };
    sideBar = { (float)WINDOW_W - SIDE_W - MARGIN, TOP_H + MARGIN,
                SIDE_W, (float)WINDOW_H - TOP_H - BOT_H - 2*MARGIN };

    float ganttX = MARGIN + 60.f;   // 60 para sobrar espaco para labels do eixo Y
    float ganttY = TOP_H + MARGIN;
    float ganttW = sideBar.left - ganttX - MARGIN;
    float ganttH = (float)WINDOW_H - TOP_H - BOT_H - 2*MARGIN;

    gantt = {
        { ganttX, ganttY, ganttW, ganttH },
        0.0f, 10.0f,
        0.0f, (float)(maxTaskId + 1)
    };

    setStatus("Pronto. F1 = ajuda.");
}

// --------------------------------------------------------------------------
// Loop principal
// --------------------------------------------------------------------------
void UIController::execute() {
    // Posiciona o cursor no ultimo tick simulado (zero se nada foi rodado ainda).
    const auto& history = os->getSnapshotsHistory();
    if (!history.empty()) {
        currentTimeIndex = static_cast<int>(history.size()) - 1;
    }

    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

// --------------------------------------------------------------------------
// Tratamento de input
// --------------------------------------------------------------------------
void UIController::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        if (event.type != sf::Event::KeyPressed) continue;

        switch (event.key.code) {
            case sf::Keyboard::Escape: window.close();             break;
            case sf::Keyboard::Right:  stepForward();               break;
            case sf::Keyboard::Left:   stepBackward();              break;
            case sf::Keyboard::Space:  togglePlay();                break;
            case sf::Keyboard::A:      runToEnd();                  break;
            case sf::Keyboard::Home:   goToStart();                 break;
            case sf::Keyboard::End:    goToEnd();                   break;
            case sf::Keyboard::P:      exportPng();                 break;
            case sf::Keyboard::R:      resetSimulation();           break;
            case sf::Keyboard::S:
                // Suspende a tarefa selecionada (apenas no live).
                if (selectedTaskId >= 0 && atLiveTick()) {
                    if (os->setTaskState(selectedTaskId, sim::TaskState::SUSPENDED))
                        setStatus("T" + std::to_string(selectedTaskId) + " -> SUSPENDED");
                } else {
                    setStatus("Edicao so vale no tick atual (END).");
                }
                break;
            case sf::Keyboard::D:
                // Re-disponibiliza (acorda) a tarefa selecionada.
                if (selectedTaskId >= 0 && atLiveTick()) {
                    if (os->setTaskState(selectedTaskId, sim::TaskState::READY))
                        setStatus("T" + std::to_string(selectedTaskId) + " -> READY");
                } else {
                    setStatus("Edicao so vale no tick atual (END).");
                }
                break;
            case sf::Keyboard::T:
                // Forca o termino. Util para "matar" tarefas no debug.
                if (selectedTaskId >= 0 && atLiveTick()) {
                    if (os->setTaskState(selectedTaskId, sim::TaskState::TERMINATED))
                        setStatus("T" + std::to_string(selectedTaskId) + " -> TERMINATED");
                }
                break;
            default: break;
        }

        // Selecao de tarefa pelas teclas 1..9.
        if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
            int digit = (event.key.code - sf::Keyboard::Num1) + 1;
            selectTaskByDigit(digit);
        }
    }
}

void UIController::update() {
    if (statusMessageFramesLeft > 0) statusMessageFramesLeft--;

    if (isPlaying) {
        frameCounter++;
        if (frameCounter >= framesPerStep) {
            frameCounter = 0;
            stepForward();
            // Para automaticamente quando termina.
            if (os->isFinished() && atLiveTick()) {
                isPlaying = false;
                setStatus("Simulacao terminou.");
            }
        }
    }
}

// --------------------------------------------------------------------------
// Acoes
// --------------------------------------------------------------------------
void UIController::stepForward() {
    int last = static_cast<int>(os->getSnapshotsHistory().size()) - 1;
    if (currentTimeIndex < last) {
        // Ja temos historico — so navega.
        currentTimeIndex++;
    } else {
        // No live: pede mais 1 tick ao SO. Se ja terminou, nada acontece.
        if (os->executeOneTick()) {
            currentTimeIndex = static_cast<int>(os->getSnapshotsHistory().size()) - 1;
        } else {
            setStatus("Simulacao ja terminou.");
        }
    }
}

void UIController::stepBackward() {
    if (currentTimeIndex > 0) currentTimeIndex--;
}

void UIController::runToEnd() {
    // Roda em batch ate o fim — implementacao real do modo "completo" (req 1.5.b).
    while (!os->isFinished()) os->executeOneTick();
    currentTimeIndex = static_cast<int>(os->getSnapshotsHistory().size()) - 1;
    isPlaying = false;
    setStatus("Avancado ate o fim.");
}

void UIController::togglePlay() {
    isPlaying = !isPlaying;
    frameCounter = 0;
    setStatus(isPlaying ? "Auto-play ON" : "Auto-play OFF");
}

void UIController::goToStart() {
    currentTimeIndex = 0;
    setStatus("Cursor no inicio.");
}

void UIController::goToEnd() {
    int last = static_cast<int>(os->getSnapshotsHistory().size()) - 1;
    if (last < 0) last = 0;
    currentTimeIndex = last;
}

void UIController::resetSimulation() {
    currentTimeIndex = 0;
    isPlaying = false;
    setStatus("Cursor resetado (historico mantido).");
}

// --------------------------------------------------------------------------
// Selecao / edicao
// --------------------------------------------------------------------------
void UIController::selectTaskByDigit(int digit) {
    if (initialTasksById.count(digit)) {
        selectedTaskId = digit;
        setStatus("Selecionada T" + std::to_string(digit));
    }
}

bool UIController::atLiveTick() const {
    int last = static_cast<int>(os->getSnapshotsHistory().size()) - 1;
    return currentTimeIndex == last;
}

const sim::GlobalState* UIController::currentSnapshot() const {
    const auto& h = os->getSnapshotsHistory();
    if (h.empty()) return nullptr;
    int idx = currentTimeIndex;
    if (idx < 0) idx = 0;
    if (idx >= (int)h.size()) idx = (int)h.size() - 1;
    return &h[idx];
}

void UIController::setStatus(const std::string& msg) {
    statusMessage = msg;
    statusMessageFramesLeft = STATUS_MSG_FRAMES;
}

// --------------------------------------------------------------------------
// PNG export
// --------------------------------------------------------------------------
// Renderiza o Gantt COMPLETO (do tick 0 ao ultimo) numa textura off-screen
// e salva como gantt.png no diretorio corrente. Atende req 2.3.
void UIController::exportPng() {
    const auto& history = os->getSnapshotsHistory();
    if (history.empty()) {
        setStatus("Nada para exportar (historico vazio).");
        return;
    }

    int totalTicks = static_cast<int>(history.size());
    // Largura proporcional aos ticks para nao ficar comprimido.
    int pxPerTick = 24;
    int imgW = std::max(800, 80 + totalTicks * pxPerTick + 20);
    int imgH = 80 + (maxTaskId + 1) * 50;

    sf::RenderTexture texture;
    if (!texture.create(imgW, imgH)) {
        setStatus("Falha ao criar textura para PNG.");
        return;
    }

    PlotArea exportPlot = {
        { 60.f, 40.f, (float)imgW - 80.f, (float)imgH - 80.f },
        0.0f, (float)totalTicks,
        0.0f, (float)(maxTaskId + 1)
    };

    texture.clear(sf::Color::White);
    // No PNG usamos cores claras / fundo branco para impressao.
    // Mas para nao duplicar a logica de desenho, reutilizamos o mesmo path
    // e aceitamos o tema escuro tambem no PNG. Fica mais consistente
    // visualmente com o que o usuario viu na tela.
    texture.clear(BG_COLOR);

    drawGrid(texture, exportPlot, totalTicks, maxTaskId + 1);
    drawAxes(texture, exportPlot);
    drawLabels(texture, exportPlot);
    renderGanttToTarget(texture, exportPlot, 0, totalTicks - 1);
    drawLegendInsideGantt(texture, exportPlot);

    texture.display();

    sf::Image img = texture.getTexture().copyToImage();
    const std::string fname = "gantt.png";
    if (img.saveToFile(fname)) {
        setStatus("Salvo: " + fname);
    } else {
        setStatus("Falha ao salvar PNG.");
    }
}

// --------------------------------------------------------------------------
// Render principal
// --------------------------------------------------------------------------
void UIController::render() {
    window.clear(BG_COLOR);

    updateViewWindow();

    drawTopBar(window);
    drawGrid(window, gantt, (int)(gantt.xmax - gantt.xmin), maxTaskId + 1);
    drawAxes(window, gantt);
    drawLabels(window, gantt);
    drawGantt(window, gantt, /*drawCursor=*/true);
    drawSideBar(window);
    drawBotBar(window);

    window.display();
}

void UIController::updateViewWindow() {
    int visibleTicks = (int)(gantt.rect.width / TICK_WIDTH);
    if (visibleTicks < 1) visibleTicks = 1;

    int viewStart = (int)gantt.xmin;
    int viewEnd = viewStart + visibleTicks - 1;

    if (currentTimeIndex > viewEnd) {
        viewEnd = currentTimeIndex;
        viewStart = viewEnd - visibleTicks + 1;
    }
    if (currentTimeIndex < viewStart) {
        viewStart = currentTimeIndex;
        viewEnd = viewStart + visibleTicks - 1;
    }
    if (viewStart < 0) {
        viewStart = 0;
        viewEnd = viewStart + visibleTicks - 1;
    }

    gantt.xmin = (float)viewStart;
    gantt.xmax = (float)(viewEnd + 1);
}

sf::Vector2f UIController::mapToScreen(float x, float y, const PlotArea& p) const {
    float nx = (x - p.xmin) / (p.xmax - p.xmin);
    float ny = (y - p.ymin) / (p.ymax - p.ymin);
    return {
        p.rect.left + nx * p.rect.width,
        p.rect.top + (1.0f - ny) * p.rect.height
    };
}

// --------------------------------------------------------------------------
// Top bar — info do sistema
// --------------------------------------------------------------------------
void UIController::drawTopBar(sf::RenderTarget& target) {
    sf::RectangleShape bg({topBar.width, topBar.height});
    bg.setPosition(topBar.left, topBar.top);
    bg.setFillColor(PANEL_COLOR);
    target.draw(bg);

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(16);
    txt.setFillColor(TEXT_COLOR);

    int total = (int)os->getSnapshotsHistory().size();
    std::ostringstream oss;
    oss << "Algoritmo: " << os->getSchedulerName()
        << "    Quantum: " << os->getQuantum()
        << "    CPUs: " << os->getCpus().size()
        << "    Tick: " << currentTimeIndex << " / " << (total > 0 ? total - 1 : 0)
        << "    Modo: " << (isPlaying ? "AUTO" : "STEP")
        << (os->isFinished() && atLiveTick() ? "  [FIM]" : "");
    txt.setString(oss.str());
    txt.setPosition(topBar.left + 16.f, topBar.top + 16.f);
    target.draw(txt);
}

// --------------------------------------------------------------------------
// Bot bar — hotkeys + mensagem de status
// --------------------------------------------------------------------------
void UIController::drawBotBar(sf::RenderTarget& target) {
    sf::RectangleShape bg({botBar.width, botBar.height});
    bg.setPosition(botBar.left, botBar.top);
    bg.setFillColor(PANEL_COLOR);
    target.draw(bg);

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(13);
    txt.setFillColor(TEXT_DIM);

    const char* lines[] = {
        "[->] Passo a frente   [<-] Passo atras   [Espaco] Play/Pause   [A] Avancar ate fim   [Home/End] Inicio/Fim",
        "[1..9] Selecionar tarefa   [S] Suspender   [D] Liberar (READY)   [T] Terminar   [P] Exportar PNG   [R] Reset cursor   [Esc] Sair"
    };

    txt.setString(lines[0]);
    txt.setPosition(botBar.left + 16.f, botBar.top + 12.f);
    target.draw(txt);

    txt.setString(lines[1]);
    txt.setPosition(botBar.left + 16.f, botBar.top + 32.f);
    target.draw(txt);

    if (statusMessageFramesLeft > 0) {
        txt.setFillColor(CURSOR_COLOR);
        txt.setString(statusMessage);
        txt.setPosition(botBar.left + 16.f, botBar.top + 60.f);
        target.draw(txt);
    }
}

// --------------------------------------------------------------------------
// Side bar — legenda + painel da tarefa selecionada
// --------------------------------------------------------------------------
void UIController::drawSideBar(sf::RenderTarget& target) {
    sf::RectangleShape bg({sideBar.width, sideBar.height});
    bg.setPosition(sideBar.left, sideBar.top);
    bg.setFillColor(PANEL_COLOR);
    bg.setOutlineThickness(1.f);
    bg.setOutlineColor(sf::Color(60, 65, 80));
    target.draw(bg);

    sf::Text txt;
    txt.setFont(font);
    txt.setFillColor(TEXT_COLOR);

    // Titulo
    txt.setCharacterSize(15);
    txt.setStyle(sf::Text::Bold);
    txt.setString("Legenda / Tarefas");
    txt.setPosition(sideBar.left + 12.f, sideBar.top + 10.f);
    target.draw(txt);
    txt.setStyle(sf::Text::Regular);

    const sim::GlobalState* snap = currentSnapshot();

    float y = sideBar.top + 40.f;
    for (auto& kv : initialTasksById) {
        int id = kv.first;
        const sim::Task& initialT = kv.second;

        // Quadrado de cor
        sf::RectangleShape swatch({18.f, 18.f});
        swatch.setPosition(sideBar.left + 12.f, y);
        swatch.setFillColor(initialT.color);
        target.draw(swatch);

        // Borda se for a selecionada
        if (id == selectedTaskId) {
            sf::RectangleShape sel({sideBar.width - 16.f, 22.f});
            sel.setPosition(sideBar.left + 8.f, y - 2.f);
            sel.setFillColor(sf::Color::Transparent);
            sel.setOutlineThickness(2.f);
            sel.setOutlineColor(CURSOR_COLOR);
            target.draw(sel);
        }

        // Estado atual no snapshot corrente
        std::string stateStr = "NEW";
        int remaining = initialT.totalDuration;
        int cpu = -1;
        if (snap) {
            for (const auto& t : snap->tasks) {
                if (t.id == id) {
                    stateStr = sim::toString(t.state);
                    remaining = t.remainingTime;
                    cpu = t.cpuAssigned;
                    break;
                }
            }
        }

        txt.setCharacterSize(13);
        std::ostringstream oss;
        oss << "T" << id << "  prio=" << initialT.staticPriority
            << "  dur=" << initialT.totalDuration
            << "  ing=" << initialT.arrivalTime;
        txt.setString(oss.str());
        txt.setPosition(sideBar.left + 38.f, y - 2.f);
        target.draw(txt);

        std::ostringstream oss2;
        oss2 << "    state=" << stateStr << "  rest=" << remaining;
        if (cpu >= 0) oss2 << "  CPU" << cpu;
        txt.setString(oss2.str());
        txt.setFillColor(TEXT_DIM);
        txt.setPosition(sideBar.left + 38.f, y + 13.f);
        target.draw(txt);
        txt.setFillColor(TEXT_COLOR);

        y += 44.f;
    }

    // Painel de info da tarefa selecionada (metricas)
    if (selectedTaskId >= 0) {
        sf::RectangleShape sep({sideBar.width - 24.f, 1.f});
        sep.setPosition(sideBar.left + 12.f, y + 4.f);
        sep.setFillColor(sf::Color(70, 75, 90));
        target.draw(sep);

        // Procura a tarefa no SO (estado atual, nao snapshot — mostra metricas vivas)
        const sim::Task* live = nullptr;
        for (const auto& t : os->getTasks()) {
            if (t.id == selectedTaskId) { live = &t; break; }
        }

        if (live) {
            txt.setCharacterSize(13);
            std::ostringstream oss;
            oss << "T" << live->id << " (metricas)\n"
                << "  start = " << live->startTime << "\n"
                << "  finish = " << live->finishTime << "\n"
                << "  turnaround = " << live->turnaround() << "\n"
                << "  espera = " << live->waitingTime << "\n"
                << "  suspenso = " << live->suspendedTime << "\n"
                << "  preempcoes = " << live->preemptions;
            txt.setString(oss.str());
            txt.setPosition(sideBar.left + 12.f, y + 12.f);
            target.draw(txt);
        }
    }
}

// --------------------------------------------------------------------------
// Gantt
// --------------------------------------------------------------------------
void UIController::drawGantt(sf::RenderTarget& target, const PlotArea& p, bool drawCursor) {
    const auto& history = os->getSnapshotsHistory();
    if (history.empty()) return;

    int startT = (int)p.xmin;
    if (startT < 0) startT = 0;
    int endT = (int)p.xmax - 1;
    if (endT >= (int)history.size()) endT = (int)history.size() - 1;

    renderGanttToTarget(target, p, startT, endT);

    if (drawCursor && currentTimeIndex >= startT && currentTimeIndex <= endT) {
        sf::Vector2f cursorTL = mapToScreen((float)currentTimeIndex, p.ymax, p);
        sf::Vector2f cursorBR = mapToScreen((float)(currentTimeIndex + 1), p.ymin, p);

        sf::RectangleShape cursor;
        cursor.setPosition(cursorTL.x, cursorTL.y);
        cursor.setSize(sf::Vector2f(cursorBR.x - cursorTL.x, cursorBR.y - cursorTL.y));
        cursor.setFillColor(sf::Color::Transparent);
        cursor.setOutlineThickness(2.f);
        cursor.setOutlineColor(CURSOR_COLOR);
        target.draw(cursor);
    }
}

// Renderiza todos os elementos do Gantt (blocos, READY, SUSPENDED,
// marcadores de chegada/termino, sorteio) para um intervalo de ticks.
// Compartilhado entre a janela ao vivo e o exportador PNG.
void UIController::renderGanttToTarget(sf::RenderTarget& target, const PlotArea& p,
                                       int firstTick, int lastTick)
{
    const auto& history = os->getSnapshotsHistory();

    // Para localizar chegadas e terminos, vamos comparar estado em t e em t-1.
    auto stateOf = [&](int tick, int taskId) -> sim::TaskState {
        if (tick < 0 || tick >= (int)history.size()) return sim::TaskState::NEW;
        for (const auto& t : history[tick].tasks) if (t.id == taskId) return t.state;
        return sim::TaskState::NEW;
    };

    for (int t = firstTick; t <= lastTick; ++t) {
        const sim::GlobalState& state = history[t];

        // Mapa rapido id -> task no snapshot
        for (const auto& task : state.tasks) {
            int id = task.id;

            // Bloco baseado no estado
            switch (task.state) {
                case sim::TaskState::RUNNING: {
                    sf::Color color = sf::Color::White;
                    auto it = taskColors.find(id);
                    if (it != taskColors.end()) color = it->second;
                    drawBlock(target, p, t, id, task.cpuAssigned, color, task.wonByLottery);
                    break;
                }
                case sim::TaskState::READY:
                    drawReadySlot(target, p, t, id);
                    break;
                case sim::TaskState::SUSPENDED:
                    drawSuspendedBlock(target, p, t, id);
                    break;
                default: break;  // NEW (ainda nao chegou) e TERMINATED nao desenham bloco
            }

            // Icone de chegada: a tarefa deixou de ser NEW exatamente neste tick.
            sim::TaskState prev = stateOf(t - 1, id);
            if (prev == sim::TaskState::NEW && task.state != sim::TaskState::NEW) {
                drawArrivalIcon(target, p, t, id);
            }

            // Icone de termino: a tarefa entrou em TERMINATED neste tick.
            if (prev != sim::TaskState::TERMINATED && task.state == sim::TaskState::TERMINATED) {
                drawTerminationIcon(target, p, t, id);
            }
        }
    }
}

// Bloco RUNNING (com cor) + label "C{n}" + marcador de sorteio se aplicavel.
void UIController::drawBlock(sf::RenderTarget& target, const PlotArea& p,
                             int tick, int taskId, int cpuId,
                             const sf::Color& color, bool isLottery)
{
    float y = (float)taskId;
    sf::Vector2f bl = mapToScreen((float)tick,     y - 0.4f, p);
    sf::Vector2f tr = mapToScreen((float)(tick+1), y + 0.4f, p);

    float w = tr.x - bl.x;
    float h = bl.y - tr.y;

    sf::RectangleShape rect({w, h});
    rect.setPosition(bl.x, tr.y);
    rect.setFillColor(color);
    rect.setOutlineThickness(1.f);
    rect.setOutlineColor(sf::Color(15, 18, 25));
    target.draw(rect);

    // Label da CPU. So desenha se houver espaco minimo.
    if (cpuId >= 0 && w > 18.f) {
        sf::Text label;
        label.setFont(font);
        label.setCharacterSize(11);
        // Cor de texto que contrasta minimamente com o fundo (preto/branco
        // dependendo da luminosidade).
        int lum = (color.r * 299 + color.g * 587 + color.b * 114) / 1000;
        label.setFillColor(lum > 128 ? sf::Color(15, 15, 20) : sf::Color::White);
        label.setString("C" + std::to_string(cpuId));
        auto b = label.getLocalBounds();
        label.setPosition(bl.x + (w - b.width) / 2.f, tr.y + 2.f);
        target.draw(label);
    }

    // Marcador de sorteio (req 4.3 item 4): estrela amarela no canto.
    if (isLottery) {
        sf::CircleShape star(5.f, 5);  // 5 lados = aproxima uma estrela
        star.setFillColor(CURSOR_COLOR);
        star.setOutlineThickness(1.f);
        star.setOutlineColor(sf::Color::Black);
        star.setPosition(bl.x + 2.f, tr.y + 2.f);
        target.draw(star);
    }
}

// Bloco SUSPENDED — preto, conforme req 2.1.
void UIController::drawSuspendedBlock(sf::RenderTarget& target, const PlotArea& p,
                                      int tick, int taskId)
{
    float y = (float)taskId;
    sf::Vector2f bl = mapToScreen((float)tick,     y - 0.4f, p);
    sf::Vector2f tr = mapToScreen((float)(tick+1), y + 0.4f, p);
    float w = tr.x - bl.x;
    float h = bl.y - tr.y;

    sf::RectangleShape rect({w, h});
    rect.setPosition(bl.x, tr.y);
    rect.setFillColor(SUSPENDED_COLOR);
    rect.setOutlineThickness(1.f);
    rect.setOutlineColor(sf::Color(80, 80, 80));
    target.draw(rect);
}

// READY: ausencia de cor (req 2.1), mas mostramos um retangulo so com borda
// pra ainda haver uma "pista" visual na linha do tempo da tarefa.
void UIController::drawReadySlot(sf::RenderTarget& target, const PlotArea& p,
                                 int tick, int taskId)
{
    float y = (float)taskId;
    sf::Vector2f bl = mapToScreen((float)tick,     y - 0.4f, p);
    sf::Vector2f tr = mapToScreen((float)(tick+1), y + 0.4f, p);
    float w = tr.x - bl.x;
    float h = bl.y - tr.y;

    sf::RectangleShape rect({w - 2.f, h - 2.f});
    rect.setPosition(bl.x + 1.f, tr.y + 1.f);
    rect.setFillColor(sf::Color::Transparent);
    rect.setOutlineThickness(1.f);
    rect.setOutlineColor(READY_BORDER);
    target.draw(rect);
}

// Triangulo verde apontando pra cima na borda esquerda do bloco do tick.
void UIController::drawArrivalIcon(sf::RenderTarget& target, const PlotArea& p,
                                   int tick, int taskId)
{
    float y = (float)taskId;
    sf::Vector2f base = mapToScreen((float)tick, y - 0.4f, p);

    sf::ConvexShape tri;
    tri.setPointCount(3);
    tri.setPoint(0, {0.f, 0.f});
    tri.setPoint(1, {10.f, 0.f});
    tri.setPoint(2, {5.f, -10.f});
    tri.setFillColor(sf::Color(40, 220, 80));
    tri.setOutlineThickness(1.f);
    tri.setOutlineColor(sf::Color::Black);
    tri.setPosition(base.x - 5.f, base.y);
    target.draw(tri);
}

// "X" vermelho no canto direito da ultima celula em que a tarefa rodou.
void UIController::drawTerminationIcon(sf::RenderTarget& target, const PlotArea& p,
                                       int tick, int taskId)
{
    float y = (float)taskId;
    sf::Vector2f anchor = mapToScreen((float)tick, y + 0.4f, p);

    sf::Color red(230, 60, 60);
    auto line = [&](float x1, float y1, float x2, float y2) {
        sf::VertexArray v(sf::Lines, 2);
        v[0] = sf::Vertex({x1, y1}, red);
        v[1] = sf::Vertex({x2, y2}, red);
        target.draw(v);
    };
    line(anchor.x - 6.f, anchor.y - 6.f, anchor.x + 6.f, anchor.y + 6.f);
    line(anchor.x - 6.f, anchor.y + 6.f, anchor.x + 6.f, anchor.y - 6.f);
}

// --------------------------------------------------------------------------
// Grid + eixos + labels
// --------------------------------------------------------------------------
void UIController::drawGrid(sf::RenderTarget& target, const PlotArea& p,
                            int xTicks, int yTicks)
{
    if (xTicks <= 0) xTicks = 1;
    if (yTicks <= 0) yTicks = 1;

    sf::VertexArray grid(sf::Lines);
    for (int i = 0; i <= xTicks; ++i) {
        float t = (float)i / xTicks;
        float x = p.rect.left + t * p.rect.width;
        grid.append(sf::Vertex({x, p.rect.top}, GRID_COLOR));
        grid.append(sf::Vertex({x, p.rect.top + p.rect.height}, GRID_COLOR));
    }
    for (int j = 0; j <= yTicks; ++j) {
        float t = (float)j / yTicks;
        float y = p.rect.top + t * p.rect.height;
        grid.append(sf::Vertex({p.rect.left, y}, GRID_COLOR));
        grid.append(sf::Vertex({p.rect.left + p.rect.width, y}, GRID_COLOR));
    }
    target.draw(grid);
}

void UIController::drawAxes(sf::RenderTarget& target, const PlotArea& p) {
    sf::RectangleShape border({p.rect.width, p.rect.height});
    border.setPosition({p.rect.left, p.rect.top});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(1.f);
    border.setOutlineColor(AXIS_COLOR);
    target.draw(border);
}

void UIController::drawLabels(sf::RenderTarget& target, const PlotArea& p) {
    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(12);
    txt.setFillColor(TEXT_DIM);

    int viewStart = (int)p.xmin;
    int viewEnd = (int)p.xmax;
    for (int t = viewStart; t <= viewEnd; ++t) {
        txt.setString(std::to_string(t));
        sf::Vector2f pos = mapToScreen((float)t, 0.0f, p);
        auto b = txt.getLocalBounds();
        txt.setPosition(pos.x - b.width / 2.f, pos.y + 6.f);
        target.draw(txt);
    }

    for (auto& kv : taskColors) {
        int id = kv.first;
        txt.setString("T" + std::to_string(id));
        sf::Vector2f pos = mapToScreen(p.xmin, (float)id, p);
        auto b = txt.getLocalBounds();
        txt.setPosition(pos.x - b.width - 8.f, pos.y - b.height);
        target.draw(txt);
    }

    txt.setCharacterSize(13);
    txt.setFillColor(TEXT_COLOR);
    txt.setString("Tempo (ticks)");
    txt.setPosition(p.rect.left + p.rect.width / 2.f - 40.f,
                    p.rect.top + p.rect.height + 22.f);
    target.draw(txt);
}

// Quando o PNG e exportado, uma legenda compacta vai junto, no canto superior.
void UIController::drawLegendInsideGantt(sf::RenderTarget& target, const PlotArea& p) {
    float x = p.rect.left + 8.f;
    float y = p.rect.top + 8.f;

    sf::Text txt;
    txt.setFont(font);
    txt.setCharacterSize(11);
    txt.setFillColor(TEXT_COLOR);

    auto entry = [&](sf::Color sw, const std::string& label) {
        sf::RectangleShape r({12.f, 12.f});
        r.setPosition(x, y);
        r.setFillColor(sw);
        r.setOutlineThickness(1.f);
        r.setOutlineColor(sf::Color::Black);
        target.draw(r);
        txt.setString(label);
        txt.setPosition(x + 16.f, y - 3.f);
        target.draw(txt);
        x += 16.f + txt.getLocalBounds().width + 14.f;
    };

    entry(sf::Color(120, 160, 220), "RUNNING");
    entry(sf::Color::Transparent,   "READY (so borda)");
    entry(SUSPENDED_COLOR,          "SUSPENDED");
    entry(CURSOR_COLOR,             "Sorteio");
}

}  // namespace view

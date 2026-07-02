#include "view/ConfigMenu.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

namespace {

// Tenta abrir o file picker nativo do sistema (zenity/kdialog).
// Retorna o caminho selecionado, ou string vazia se o usuario cancelou
// ou se nenhum utilitario esta instalado. O ConfigMenu usa isso para
// evitar o input de texto sempre que possivel.
std::string tryNativeFileDialog() {
    const char* candidates[] = {
        "command -v zenity  >/dev/null 2>&1 && "
        "zenity --file-selection "
               "--title=\"Selecionar arquivo de configuracao\" "
               "--file-filter=\"Config (*.txt) | *.txt\" "
               "--file-filter=\"Todos | *\" 2>/dev/null",
        "command -v kdialog >/dev/null 2>&1 && "
        "kdialog --getopenfilename \"$HOME\" "
                "\"*.txt|Config (*.txt)\" 2>/dev/null",
        nullptr
    };
    for (int i = 0; candidates[i] != nullptr; ++i) {
        FILE* p = popen(candidates[i], "r");
        if (!p) continue;
        std::string result;
        char buf[4096];
        while (std::fgets(buf, sizeof(buf), p) != nullptr) {
            result += buf;
        }
        int rc = pclose(p);
        while (!result.empty() &&
               (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        // rc == 0: usuario confirmou. rc != 0: cancelou ou utilitario
        // ausente -> tenta o proximo.
        if (rc == 0 && !result.empty()) {
            return result;
        }
    }
    return "";
}

}  // namespace

namespace view {

// --------------------------------------------------------------------------
// Constantes de layout
// --------------------------------------------------------------------------
static const int   WIN_W       = 720;
static const int   WIN_H       = 520;
static const float MARGIN      = 24.0f;
static const float TITLE_H     = 60.0f;
static const float FOOTER_H    = 90.0f;
static const float ROW_H       = 34.0f;
static const int   STATUS_TTL  = 200;   // frames

static const sf::Color BG_COLOR      ( 28,  30,  38);
static const sf::Color PANEL_COLOR   ( 40,  44,  56);
static const sf::Color ROW_HOVER     ( 60,  68,  90);
static const sf::Color ROW_SELECTED  ( 90, 130, 190);
static const sf::Color TEXT_COLOR    (220, 220, 230);
static const sf::Color TEXT_DIM      (150, 152, 165);
static const sf::Color ACCENT        (255, 215,   0);
static const sf::Color OVERLAY_BG    (  0,   0,   0, 200);
static const sf::Color ERROR_COLOR   (240, 100, 100);
static const sf::Color OK_COLOR      (120, 210, 140);

// --------------------------------------------------------------------------
// Construtor
// --------------------------------------------------------------------------
ConfigMenu::ConfigMenu(std::string _configDir)
    : window(sf::VideoMode(WIN_W, WIN_H), "Simulador SO - Selecao de Configuracao",
             sf::Style::Titlebar | sf::Style::Close),
      configDir(std::move(_configDir)),
      selectedIndex(0),
      scrollOffset(0),
      mode(Mode::LIST),
      statusFrames(0)
{
    window.setFramerateLimit(60);

    if (!font.loadFromFile("../assets/fonts/Roboto-Regular.ttf")) {
        if (!font.loadFromFile("assets/fonts/Roboto-Regular.ttf")) {
            std::cerr << "[Menu] Aviso: nao foi possivel carregar a fonte.\n";
        }
    }

    // Garante que a pasta config exista (pode nao existir se o usuario
    // apagou tudo ou rodou o binario de outro diretorio).
    std::error_code ec;
    fs::create_directories(configDir, ec);

    refreshFiles();
}

// --------------------------------------------------------------------------
// Loop principal
// --------------------------------------------------------------------------
std::string ConfigMenu::selectConfig() {
    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            handleEvent(ev);
        }
        if (statusFrames > 0) --statusFrames;
        render();
        // Se o usuario confirmou uma escolha, sai.
        if (!chosen.empty()) {
            window.close();
        }
    }
    return chosen;
}

// --------------------------------------------------------------------------
// Refresh de arquivos
// --------------------------------------------------------------------------
void ConfigMenu::refreshFiles() {
    files.clear();
    std::error_code ec;
    if (!fs::exists(configDir, ec)) return;

    for (const auto& entry : fs::directory_iterator(configDir, ec)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path();
        if (path.extension() != ".txt") continue;
        files.push_back(path.filename().string());
    }
    std::sort(files.begin(), files.end());

    if (files.empty()) {
        selectedIndex = 0;
    } else if (selectedIndex >= static_cast<int>(files.size())) {
        selectedIndex = static_cast<int>(files.size()) - 1;
    }
    if (selectedIndex < 0) selectedIndex = 0;
}

// --------------------------------------------------------------------------
// Eventos
// --------------------------------------------------------------------------
void ConfigMenu::handleEvent(const sf::Event& ev) {
    if (ev.type == sf::Event::Closed) {
        // Fecha janela sem selecionar -> chosen vazio, main aborta.
        window.close();
        return;
    }

    if (mode == Mode::INPUT_PATH) {
        if (ev.type == sf::Event::TextEntered) {
            char32_t c = ev.text.unicode;
            if (c == 8) {  // Backspace
                if (!inputBuffer.empty()) inputBuffer.pop_back();
            } else if (c == 13 || c == 10) {
                // Enter -> tratado abaixo em KeyPressed
            } else if (c == 27) {
                // Esc -> tratado abaixo
            } else if (c >= 32 && c < 127) {
                inputBuffer.push_back(static_cast<char>(c));
            }
            return;
        }
        if (ev.type == sf::Event::KeyPressed) {
            if (ev.key.code == sf::Keyboard::Enter) {
                commitAttach();
            } else if (ev.key.code == sf::Keyboard::Escape) {
                cancelSubmode();
            } else if (ev.key.code == sf::Keyboard::V && ev.key.control) {
                // Ctrl+V: cola do clipboard
                sf::String s = sf::Clipboard::getString();
                inputBuffer += s.toAnsiString();
            }
        }
        return;
    }

    if (mode == Mode::CONFIRM_DELETE) {
        if (ev.type == sf::Event::KeyPressed) {
            if (ev.key.code == sf::Keyboard::Y || ev.key.code == sf::Keyboard::Enter) {
                confirmDelete();
            } else if (ev.key.code == sf::Keyboard::N ||
                       ev.key.code == sf::Keyboard::Escape) {
                cancelSubmode();
            }
        }
        return;
    }

    // ---- Modo LIST ----
    if (ev.type == sf::Event::KeyPressed) {
        switch (ev.key.code) {
            case sf::Keyboard::Up:
                if (selectedIndex > 0) --selectedIndex;
                break;
            case sf::Keyboard::Down:
                if (selectedIndex + 1 < static_cast<int>(files.size())) ++selectedIndex;
                break;
            case sf::Keyboard::Home:
                selectedIndex = 0;
                break;
            case sf::Keyboard::End:
                if (!files.empty()) selectedIndex = static_cast<int>(files.size()) - 1;
                break;
            case sf::Keyboard::Enter:
                confirmSelection();
                break;
            case sf::Keyboard::A:
                beginAttach();
                break;
            case sf::Keyboard::Delete:
            case sf::Keyboard::D:
                beginDelete();
                break;
            case sf::Keyboard::F5:
                refreshFiles();
                setStatus("Lista atualizada.");
                break;
            case sf::Keyboard::Escape:
                window.close();
                break;
            default: break;
        }
    } else if (ev.type == sf::Event::MouseButtonPressed &&
               ev.mouseButton.button == sf::Mouse::Left) {
        // Clique num item da lista -> seleciona; duplo click seria ideal, mas
        // como nao ha wiring pra isso na SFML basic, deixamos so o clique.
        float y = static_cast<float>(ev.mouseButton.y);
        float listTop = TITLE_H + MARGIN;
        int idx = scrollOffset + static_cast<int>((y - listTop) / ROW_H);
        if (idx >= 0 && idx < static_cast<int>(files.size())) {
            if (idx == selectedIndex) {
                confirmSelection();
            } else {
                selectedIndex = idx;
            }
        }
    } else if (ev.type == sf::Event::MouseWheelScrolled) {
        scrollOffset -= static_cast<int>(ev.mouseWheelScroll.delta);
        if (scrollOffset < 0) scrollOffset = 0;
    }
}

// --------------------------------------------------------------------------
// Acoes
// --------------------------------------------------------------------------
void ConfigMenu::confirmSelection() {
    if (files.empty()) {
        setStatus("Nenhum arquivo na lista. Pressione A para anexar.");
        return;
    }
    fs::path p = fs::path(configDir) / files[selectedIndex];
    chosen = p.string();
}

void ConfigMenu::beginAttach() {
    // Caminho preferido: file picker nativo (zenity/kdialog).
    // Como bloqueia o processo, escondemos a janela pra nao parecer
    // travada e restauramos depois.
    std::string native = tryNativeFileDialog();
    if (!native.empty()) {
        inputBuffer = native;
        commitAttach();
        return;
    }

    // Fallback: modo texto para quem nao tem zenity/kdialog instalado.
    mode = Mode::INPUT_PATH;
    inputBuffer.clear();
    setStatus("Sem zenity/kdialog. Digite ou cole o caminho (Ctrl+V para colar).");
}

void ConfigMenu::commitAttach() {
    // Remove aspas envolventes (comum quando se cola do file manager).
    std::string src = inputBuffer;
    while (!src.empty() && (src.front() == '"' || src.front() == '\'' ||
                            src.front() == ' ')) {
        src.erase(src.begin());
    }
    while (!src.empty() && (src.back() == '"' || src.back() == '\'' ||
                            src.back() == ' ')) {
        src.pop_back();
    }

    if (src.empty()) {
        setStatus("Caminho vazio.");
        return;
    }

    std::error_code ec;
    fs::path srcPath(src);
    if (!fs::exists(srcPath, ec) || !fs::is_regular_file(srcPath, ec)) {
        setStatus("Arquivo nao encontrado: " + src);
        return;
    }

    fs::path dst = fs::path(configDir) / srcPath.filename();
    fs::copy_file(srcPath, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        setStatus("Erro ao copiar: " + ec.message());
        return;
    }

    inputBuffer.clear();
    mode = Mode::LIST;
    refreshFiles();

    // Seleciona o arquivo recem-anexado.
    auto it = std::find(files.begin(), files.end(), srcPath.filename().string());
    if (it != files.end()) {
        selectedIndex = static_cast<int>(std::distance(files.begin(), it));
    }
    setStatus("Anexado: " + srcPath.filename().string());
}

void ConfigMenu::beginDelete() {
    if (files.empty()) {
        setStatus("Nenhum arquivo para remover.");
        return;
    }
    mode = Mode::CONFIRM_DELETE;
}

void ConfigMenu::confirmDelete() {
    if (files.empty()) {
        mode = Mode::LIST;
        return;
    }
    fs::path p = fs::path(configDir) / files[selectedIndex];
    std::error_code ec;
    if (!fs::remove(p, ec)) {
        setStatus("Erro ao remover: " + ec.message());
    } else {
        setStatus("Removido: " + files[selectedIndex]);
    }
    mode = Mode::LIST;
    refreshFiles();
}

void ConfigMenu::cancelSubmode() {
    mode = Mode::LIST;
    inputBuffer.clear();
    setStatus("Cancelado.");
}

void ConfigMenu::setStatus(const std::string& msg) {
    statusMsg = msg;
    statusFrames = STATUS_TTL;
}

// --------------------------------------------------------------------------
// Renderizacao
// --------------------------------------------------------------------------
void ConfigMenu::render() {
    window.clear(BG_COLOR);
    drawTitle();
    drawList();
    drawFooter();
    if (mode == Mode::INPUT_PATH)    drawInputOverlay();
    if (mode == Mode::CONFIRM_DELETE) drawConfirmOverlay();
    window.display();
}

void ConfigMenu::drawTitle() {
    sf::RectangleShape bar({static_cast<float>(WIN_W), TITLE_H});
    bar.setFillColor(PANEL_COLOR);
    window.draw(bar);

    sf::Text t("Escolha uma configuracao", font, 22);
    t.setFillColor(TEXT_COLOR);
    t.setPosition(MARGIN, 16.0f);
    window.draw(t);

    sf::Text sub("Pasta: " + configDir, font, 12);
    sub.setFillColor(TEXT_DIM);
    sub.setPosition(MARGIN, 40.0f);
    window.draw(sub);
}

void ConfigMenu::drawList() {
    float x = MARGIN;
    float y = TITLE_H + MARGIN;
    float w = WIN_W - 2 * MARGIN;
    float h = WIN_H - TITLE_H - FOOTER_H - 2 * MARGIN;

    // Fundo do painel de lista
    sf::RectangleShape bg({w, h});
    bg.setPosition(x, y);
    bg.setFillColor(PANEL_COLOR);
    window.draw(bg);

    if (files.empty()) {
        sf::Text t("Nenhum arquivo .txt em " + configDir + ".\n"
                   "Pressione [A] para anexar um.",
                   font, 14);
        t.setFillColor(TEXT_DIM);
        t.setPosition(x + 12.0f, y + 12.0f);
        window.draw(t);
        return;
    }

    int visibleRows = static_cast<int>(h / ROW_H);
    // Garante que o item selecionado apareca na tela
    if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    if (selectedIndex >= scrollOffset + visibleRows)
        scrollOffset = selectedIndex - visibleRows + 1;
    if (scrollOffset < 0) scrollOffset = 0;

    int last = std::min(scrollOffset + visibleRows, static_cast<int>(files.size()));

    for (int i = scrollOffset; i < last; ++i) {
        float ry = y + (i - scrollOffset) * ROW_H;
        if (i == selectedIndex) {
            sf::RectangleShape hi({w, ROW_H});
            hi.setPosition(x, ry);
            hi.setFillColor(ROW_SELECTED);
            window.draw(hi);
        }
        sf::Text name(files[i], font, 15);
        name.setFillColor(TEXT_COLOR);
        name.setPosition(x + 12.0f, ry + 7.0f);
        window.draw(name);
    }

    // Indicador de scroll
    if (static_cast<int>(files.size()) > visibleRows) {
        sf::Text sc(std::to_string(selectedIndex + 1) + "/" +
                    std::to_string(files.size()),
                    font, 11);
        sc.setFillColor(TEXT_DIM);
        sc.setPosition(x + w - 60.0f, y + h - 20.0f);
        window.draw(sc);
    }
}

void ConfigMenu::drawFooter() {
    float y = WIN_H - FOOTER_H;
    sf::RectangleShape bar({static_cast<float>(WIN_W), FOOTER_H});
    bar.setPosition(0.0f, y);
    bar.setFillColor(PANEL_COLOR);
    window.draw(bar);

    sf::Text hotkeys(
        "[Enter/Click] Executar   [A] Anexar   [Del] Remover   "
        "[F5] Atualizar   [Esc] Sair",
        font, 12);
    hotkeys.setFillColor(TEXT_COLOR);
    hotkeys.setPosition(MARGIN, y + 12.0f);
    window.draw(hotkeys);

    sf::Text nav("Setas: navegar   Home/End: primeiro/ultimo",
                 font, 12);
    nav.setFillColor(TEXT_DIM);
    nav.setPosition(MARGIN, y + 32.0f);
    window.draw(nav);

    if (statusFrames > 0 && !statusMsg.empty()) {
        sf::Color c = TEXT_COLOR;
        if (statusMsg.rfind("Erro", 0) == 0)          c = ERROR_COLOR;
        else if (statusMsg.rfind("Anexado", 0) == 0)  c = OK_COLOR;
        else if (statusMsg.rfind("Removido", 0) == 0) c = OK_COLOR;
        sf::Text st(statusMsg, font, 12);
        st.setFillColor(c);
        st.setPosition(MARGIN, y + 58.0f);
        window.draw(st);
    }
}

void ConfigMenu::drawInputOverlay() {
    sf::RectangleShape ov({static_cast<float>(WIN_W), static_cast<float>(WIN_H)});
    ov.setFillColor(OVERLAY_BG);
    window.draw(ov);

    float w = WIN_W - 4 * MARGIN;
    float h = 160.0f;
    float x = 2 * MARGIN;
    float y = (WIN_H - h) / 2.0f;

    sf::RectangleShape box({w, h});
    box.setPosition(x, y);
    box.setFillColor(PANEL_COLOR);
    box.setOutlineColor(ACCENT);
    box.setOutlineThickness(2.0f);
    window.draw(box);

    sf::Text title("Anexar arquivo de configuracao", font, 16);
    title.setFillColor(TEXT_COLOR);
    title.setPosition(x + 12.0f, y + 12.0f);
    window.draw(title);

    sf::Text hint("Cole ou digite o caminho absoluto. Ctrl+V para colar.",
                  font, 12);
    hint.setFillColor(TEXT_DIM);
    hint.setPosition(x + 12.0f, y + 36.0f);
    window.draw(hint);

    // Caixa de input
    sf::RectangleShape ib({w - 24.0f, 32.0f});
    ib.setPosition(x + 12.0f, y + 62.0f);
    ib.setFillColor(BG_COLOR);
    ib.setOutlineColor(TEXT_DIM);
    ib.setOutlineThickness(1.0f);
    window.draw(ib);

    // Texto do input (com cursor pisca-alegre simplificado: sempre um "_" no fim)
    std::string display = inputBuffer + "_";
    sf::Text txt(display, font, 14);
    txt.setFillColor(TEXT_COLOR);
    txt.setPosition(x + 18.0f, y + 68.0f);
    window.draw(txt);

    sf::Text foot("[Enter] Copiar para " + configDir + "/   [Esc] Cancelar",
                  font, 12);
    foot.setFillColor(TEXT_DIM);
    foot.setPosition(x + 12.0f, y + h - 24.0f);
    window.draw(foot);
}

void ConfigMenu::drawConfirmOverlay() {
    sf::RectangleShape ov({static_cast<float>(WIN_W), static_cast<float>(WIN_H)});
    ov.setFillColor(OVERLAY_BG);
    window.draw(ov);

    float w = 480.0f;
    float h = 140.0f;
    float x = (WIN_W - w) / 2.0f;
    float y = (WIN_H - h) / 2.0f;

    sf::RectangleShape box({w, h});
    box.setPosition(x, y);
    box.setFillColor(PANEL_COLOR);
    box.setOutlineColor(ERROR_COLOR);
    box.setOutlineThickness(2.0f);
    window.draw(box);

    sf::Text title("Remover arquivo?", font, 16);
    title.setFillColor(TEXT_COLOR);
    title.setPosition(x + 16.0f, y + 14.0f);
    window.draw(title);

    std::string fname = files.empty() ? "" : files[selectedIndex];
    sf::Text name(fname, font, 14);
    name.setFillColor(ACCENT);
    name.setPosition(x + 16.0f, y + 46.0f);
    window.draw(name);

    sf::Text foot("[Y/Enter] Sim   [N/Esc] Nao", font, 12);
    foot.setFillColor(TEXT_DIM);
    foot.setPosition(x + 16.0f, y + h - 24.0f);
    window.draw(foot);
}

}  // namespace view

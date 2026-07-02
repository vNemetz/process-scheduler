#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

namespace view {

// Tela inicial de selecao de arquivo de configuracao.
//
// Lista os arquivos .txt existentes na pasta config/, permite anexar
// novos arquivos (copiando para essa pasta) e escolher qual usar.
//
// Uso:
//   ConfigMenu menu("config");
//   std::string path = menu.selectConfig();  // "" se cancelado
class ConfigMenu {
public:
    explicit ConfigMenu(std::string configDir);

    // Abre a janela e bloqueia ate o usuario escolher (Enter) ou cancelar (Esc).
    // Retorna o caminho absoluto/relativo pro arquivo escolhido, ou "" se
    // o usuario fechou a janela sem escolher.
    std::string selectConfig();

private:
    enum class Mode {
        LIST,          // Navegacao normal na lista
        INPUT_PATH,    // Digitando caminho de arquivo pra anexar
        CONFIRM_DELETE // Confirmando exclusao do item selecionado
    };

    sf::RenderWindow window;
    sf::Font font;

    std::string configDir;
    std::vector<std::string> files;   // Nome do arquivo (sem diretorio)
    int selectedIndex;
    int scrollOffset;

    Mode mode;
    std::string inputBuffer;
    std::string statusMsg;
    int statusFrames;

    std::string chosen;   // resultado final ("" se cancelou)

    // Ciclo de execucao
    void handleEvent(const sf::Event& ev);
    void render();

    // Acoes
    void refreshFiles();
    void confirmSelection();     // Enter: seleciona arquivo atual
    void beginAttach();          // A: entra em modo input
    void commitAttach();         // Enter no input: copia arquivo
    void beginDelete();          // Del: pede confirmacao
    void confirmDelete();        // Y: remove arquivo
    void cancelSubmode();        // Esc no submodo: volta pra lista

    void setStatus(const std::string& msg);

    // Desenho
    void drawTitle();
    void drawList();
    void drawFooter();
    void drawInputOverlay();
    void drawConfirmOverlay();
};

}  // namespace view

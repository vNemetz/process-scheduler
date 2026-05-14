#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <map>

#include "core/OperatingSystem.hpp"
#include "core/Task.hpp"

namespace view {

    struct PlotArea {
        sf::FloatRect rect;
        float xmin;
        float xmax;
        float ymin;
        float ymax;
    };

    class UIController {
    public:
        UIController(const std::vector<sim::GlobalState>& history, 
                     const std::vector<sim::Task>& initialTasks);

        void execute();

    private:
        sf::RenderWindow window;
        PlotArea plot;

        sf::Font font;

        // Simulation data
        std::vector<sim::GlobalState> historyData;
        std::map<int, sf::Color> taskColors;
        int currentTimeIndex; 

        // Internal methods
        void processEvents();
        void render();

        // Recalcula plot.xmin e plot.xmax para manter o cursor de tempo
        // (currentTimeIndex) sempre visivel, com tamanho fixo por tick.
        void updateViewWindow();
        
        // Drawing methods
        sf::Vector2f mapToScreen(float x, float y, const view::PlotArea& p);
        void drawGrid(sf::RenderTarget& target, const view::PlotArea& p, int xTicks, int yTicks);
        void drawAxes(sf::RenderTarget& target, const view::PlotArea& p);
        void drawGantt(sf::RenderTarget& target, const view::PlotArea& p);

        void drawLabels(sf::RenderTarget& target, const view::PlotArea& p);

        // Helper to convert HEX colors to SFML colors (rgb)
        sf::Color parseHexColor(const std::string& hexStr);
    };

}
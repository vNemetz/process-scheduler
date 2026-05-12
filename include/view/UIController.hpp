#pragma once 
#include <SFML/Graphics.hpp>
#include <vector>

namespace view{

  struct PlotArea{
    sf::FloatRect rect;
    float xmin, xmax, ymin, ymax;
  };

  class UIController{
  public:
    UIController();
    void execute();
  private:
    void processEvents();
    void render();
    void buildDemoData();

    sf::Vector2f mapToScreen(float x, float y, const view::PlotArea& p);
    void drawGrid(sf::RenderTarget& target, const view::PlotArea& p, int xTicks, int yTicks);
    void drawAxes(sf::RenderTarget& target, const view::PlotArea& p);
    void drawLine(sf::RenderTarget& target, const view::PlotArea& p,
            const std::vector<sf::Vector2f>& data);

    sf::RenderWindow window;
    PlotArea plot;
    std::vector<sf::Vector2f> data;
  };

}
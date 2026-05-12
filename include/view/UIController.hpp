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

    sf::RenderWindow window;
    PlotArea plot;
    std::vector<sf::Vector2f> data;
  };

}
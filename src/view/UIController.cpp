#include <view/UIController.hpp>

#include <cmath>

namespace view{
namespace{
sf::Vector2f mapToScreen(float x, float y, const view::PlotArea& p) {
    float nx = (x - p.xmin) / (p.xmax - p.xmin);
    float ny = (y - p.ymin) / (p.ymax - p.ymin);
    return {
        p.rect.left + nx * p.rect.width,
        p.rect.top + (1.0f - ny) * p.rect.height
    };
}

void drawGrid(sf::RenderTarget& target, const view::PlotArea& p, int xTicks, int yTicks) {
    sf::VertexArray grid(sf::Lines);

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

void drawAxes(sf::RenderTarget& target, const view::PlotArea& p) {
    sf::VertexArray axes(sf::Lines, 4);

    float x0 = (0.0f >= p.xmin && 0.0f <= p.xmax)
        ? mapToScreen(0.0f, 0.0f, p).x
        : p.rect.left;
    float y0 = (0.0f >= p.ymin && 0.0f <= p.ymax)
        ? mapToScreen(0.0f, 0.0f, p).y
        : p.rect.top + p.rect.height;

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

void drawLine(sf::RenderTarget& target, const view::PlotArea& p,
              const std::vector<sf::Vector2f>& data) {
    if (data.empty()) {
        return;
    }
    sf::VertexArray line(sf::LineStrip, data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        auto pt = mapToScreen(data[i].x, data[i].y, p);
        line[i] = sf::Vertex(pt, sf::Color(100, 200, 250));
    }
    target.draw(line);
}
}

UIController::UIController()
    : window(sf::VideoMode(1200, 600), "Simulador SO - UI"),
      plot{{80.f, 60.f, 1000.f, 460.f}, -10.f, 10.f, -2.f, 2.f} {
    window.setFramerateLimit(60);
    buildDemoData();
}

void UIController::execute() {
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
        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::Escape) {
            window.close();
        }
    }
}

void UIController::render() {
    window.clear(sf::Color(30, 30, 35));

    drawGrid(window, plot, 10, 8);
    drawAxes(window, plot);
    drawLine(window, plot, data);

    window.display();
}

void UIController::buildDemoData() {
    data.clear();
    data.reserve(201);
    for (int i = -100; i <= 100; ++i) {
        float x = static_cast<float>(i) / 10.0f;
        data.push_back({x, std::sin(x)});
    }
}


}

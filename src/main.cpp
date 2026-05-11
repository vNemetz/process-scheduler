#include <SFML/Graphics.hpp>

int main() {
    // Cria uma janela (Requisito 3 do projeto)
    sf::RenderWindow window(sf::VideoMode(400, 400), "Teste SFML - Simulador");
    
    // Um simples circulo para testar a renderizacao
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Cyan);
    shape.setPosition(100.f, 100.f);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}
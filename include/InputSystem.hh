#include<SFML/Graphics.hpp>

class InputSystem
{
public:
  InputSystem(){}
  ~InputSystem(){}

  static sf::Vector2f Axis()
  {
    sf::Vector2f axis{};

    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
    {
      axis.x = 1;
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
    {
      axis.x = -1;
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
    {
      axis.y = -1;
    }
    if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
    {
      axis.y = 1;
    }

    return axis;
  }
};

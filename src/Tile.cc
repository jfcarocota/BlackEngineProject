#include "Tile.hh"
#include <iostream>
#include <memory>

Tile::Tile(const std::string& textureUrl, float scale, int width, int height, int column, int row,
float posX, float posY, sf::RenderWindow*& window)
{
  try {
    this->window = window;
    this->scale = scale;
    this->width = width;
    this->height = height;
    this->column = column;
    this->row = row;
    this->posX = posX;
    this->posY = posY;

    texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(textureUrl)) {
      std::cerr << "Failed to load tile texture: " << textureUrl << std::endl;
    }
    sprite = std::make_unique<sf::Sprite>(*texture, sf::IntRect({column * width, row * height}, {width, height}));
    sprite->setPosition(sf::Vector2f(posX, posY));
    sprite->setColor(sf::Color::White);
    sprite->setScale(sf::Vector2f(scale, scale));
  } catch (const std::exception& e) {
    std::cerr << "Exception in Tile constructor: " << e.what() << std::endl;
    texture.reset();
    sprite.reset();
  }
}

Tile::~Tile()
{
  // No need to manually delete smart pointers
}

void Tile::Draw()
{
  window->draw(*sprite);
}
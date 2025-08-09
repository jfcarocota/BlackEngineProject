#include "Tile.hh"
#include <iostream>

Tile::Tile(std::string textureUrl, float scale, int width, int height, int column, int row,
float posX, float posY, sf::RenderWindow*& window)
{
  this->window = window;
  this->scale = scale;
  this->width = width;
  this->height = height;
  this->column = column;
  this->row = row;
  this->posX = posX;
  this->posY = posY;

  texture = new sf::Texture();
  if (!texture->loadFromFile(textureUrl)) {
    std::cerr << "Failed to load tile texture: " << textureUrl << std::endl;
  }
  sprite = new sf::Sprite(*texture, sf::IntRect({column * width, row * height}, {width, height}));
  sprite->setPosition(sf::Vector2f(posX, posY));
  sprite->setColor(sf::Color::White);
  sprite->setScale(sf::Vector2f(scale, scale));
}

Tile::~Tile()
{
  if (texture) {
    delete texture;
  }
  if (sprite) {
    delete sprite;
  }
}

void Tile::Draw()
{
  window->draw(*sprite);
}
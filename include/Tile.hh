#pragma once

#include<SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <gsl/assert>
#include <gsl/narrow>

class Tile
{
private:
  float scale{};
  int width{};
  int height{};
  int column{};
  int row{};
  float posX{};
  float posY{};
  std::unique_ptr<sf::Sprite> sprite;
  std::unique_ptr<sf::Texture> texture;
  sf::RenderWindow* window;
public:
  Tile(const std::string& textureUrl, float scale, int width, int height, int column, int row, 
  float posX, float posY, sf::RenderWindow*& window);
  ~Tile();

  void Draw();
};
#pragma once
#include "Tile.hh"
#include<fstream>
#include <memory>

class TileGroup
{
private:
  sf::RenderWindow* window;
  std::unique_ptr<std::vector<std::unique_ptr<Tile>>> tiles;
  int COLS{}, ROWS{};
  std::unique_ptr<std::ifstream> reader;
  const char* filePath{};
  float scale;
  float tileWidth{}, tileHeight{};
  const char* textureUrl{};
public:
  TileGroup(sf::RenderWindow* window, int COLS, int ROWS, const char* filePath, 
  float scale, float tileWidth, float tileHeight, const char* textureUrl);
  ~TileGroup();

  void GenerateMap();
  void Draw();
};
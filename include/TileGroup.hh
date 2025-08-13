#pragma once
#include "Tile.hh"
#include <memory>
#include <string>

class TileGroup
{
private:
  sf::RenderWindow* window;
  // Multiple layers of tiles (drawn in order)
  std::unique_ptr<std::vector<std::vector<std::unique_ptr<Tile>>>> layerTiles;
  int COLS{}, ROWS{};
  std::string filePathStr{};
  float scale;
  float tileWidth{}, tileHeight{};
  std::string textureUrlStr{};
public:
  TileGroup(sf::RenderWindow* window, int COLS, int ROWS, const char* filePath, 
  float scale, float tileWidth, float tileHeight, const char* textureUrl);
  ~TileGroup();

  void GenerateMap();
  void Draw();
};
#include "TileGroup.hh"
#include <iostream>
#include <memory>

TileGroup::TileGroup(sf::RenderWindow* window, int COLS, int ROWS, const char* filePath, 
float scale, float tileWidth, float tileHeight, const char* textureUrl)
{
  this->textureUrl = textureUrl;
  this->tileWidth = tileWidth;
  this->tileHeight = tileHeight;
  this->scale = scale;
  this->COLS = COLS;
  this->ROWS = ROWS;
  this->filePath = filePath;
  reader = std::make_unique<std::ifstream>();
  this->window = window;
  tiles = std::make_unique<std::vector<std::unique_ptr<Tile>>>();

  GenerateMap();
}

TileGroup::~TileGroup()
{
  // Smart pointers automatically clean up
}

void TileGroup::GenerateMap()
{
  reader->open(filePath);
  
  if (!reader->is_open()) {
    std::cerr << "Failed to open map file: " << filePath << std::endl;
    return;
  }
  
  for(int y{}; y < ROWS; y++)
  {
    for(int x{}; x < COLS; x++)
    {
      float posX{scale * tileWidth * x};
      float posY{scale * tileHeight * y};

      int coord{};
      if (!(*reader >> coord)) {
        std::cerr << "Failed to read coordinate at position (" << x << ", " << y << ") from map file: " << filePath << std::endl;
        break;
      }
      int col{coord};
      if (!(*reader >> coord)) {
        std::cerr << "Failed to read coordinate at position (" << x << ", " << y << ") from map file: " << filePath << std::endl;
        break;
      }
      int row{coord};
      tiles->push_back(std::make_unique<Tile>(textureUrl, scale, tileWidth, tileHeight, col, row, posX, posY, window));
    }
  }
  reader->close();
}

void TileGroup::Draw()
{
  for(auto& tile : *tiles)
  {
    tile->Draw();
  }
}
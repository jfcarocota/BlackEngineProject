#include "TileGroup.hh"
#include <iostream>
#include <memory>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include <json/json.h>

TileGroup::TileGroup(sf::RenderWindow* window, int COLS, int ROWS, const char* filePath, 
float scale, float tileWidth, float tileHeight, const char* textureUrl)
{
  this->textureUrlStr = textureUrl ? std::string(textureUrl) : std::string{};
  this->tileWidth = tileWidth;
  this->tileHeight = tileHeight;
  this->scale = scale;
  this->COLS = COLS;
  this->ROWS = ROWS;
  this->filePathStr = filePath ? std::string(filePath) : std::string{};
  this->window = window;
  layerTiles = std::make_unique<std::vector<std::vector<std::unique_ptr<Tile>>>>();

  GenerateMap();
}

TileGroup::~TileGroup()
{
  // Smart pointers automatically clean up
}
void TileGroup::GenerateMap()
{
  // JSON-only map loading (supports single-layer and layered variants)
  try {
    if (filePathStr.empty()) {
      std::cerr << "TileGroup: empty map path" << std::endl;
      return;
    }

    std::cout << "TileGroup: loading JSON map -> " << filePathStr << std::endl;
    // Parse with jsoncpp for robustness
    std::ifstream in(filePathStr, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
      std::cerr << "Failed to open JSON map file: " << filePathStr << std::endl;
      return;
    }
    Json::CharReaderBuilder rbuilder;
    rbuilder["collectComments"] = false;
    Json::Value root;
    std::string errs;
    if (!Json::parseFromStream(rbuilder, in, &root, &errs)) {
      std::cerr << "JSON parse error: " << errs << std::endl;
      return;
    }
    in.close();

    int totalPlaced = 0;
    layerTiles->clear();

    auto readGrid = [&](const Json::Value& gridVal) -> std::vector<std::vector<std::pair<int,int>>> {
      std::vector<std::vector<std::pair<int,int>>> grid;
      if (!gridVal.isArray()) return grid;
      grid.reserve(gridVal.size());
      for (const auto& rowVal : gridVal) {
        std::vector<std::pair<int,int>> row;
        if (!rowVal.isArray()) continue;
        row.reserve(rowVal.size());
        for (const auto& cell : rowVal) {
          if (cell.isArray() && cell.size() == 2) {
            int c = cell[0].asInt();
            int r = cell[1].asInt();
            row.emplace_back(c, r);
          } else {
            row.emplace_back(0, 0);
          }
        }
        grid.push_back(std::move(row));
      }
      return grid;
    };

    if (root.isMember("layers") && root["layers"].isArray()) {
      const auto& arr = root["layers"];
      int layerIndex = 0;
      for (const auto& L : arr) {
        std::string tilesetPath = L.isMember("tileset") ? L["tileset"].asString() : this->textureUrlStr;
        int lTileW = L.isMember("tileW") ? L["tileW"].asInt() : static_cast<int>(this->tileWidth);
        int lTileH = L.isMember("tileH") ? L["tileH"].asInt() : static_cast<int>(this->tileHeight);
        auto grid = readGrid(L["grid"]);
        if (grid.empty() || grid[0].empty()) { ++layerIndex; continue; }
        if (layerIndex == 0) { ROWS = static_cast<int>(grid.size()); COLS = static_cast<int>(grid[0].size()); }
        float tw = static_cast<float>(lTileW);
        float th = static_cast<float>(lTileH);
        std::vector<std::unique_ptr<Tile>> layerVec;
        layerVec.reserve(static_cast<size_t>(ROWS * COLS));
        for (int y = 0; y < static_cast<int>(grid.size()); ++y) {
          for (int x = 0; x < static_cast<int>(grid[y].size()); ++x) {
            auto [col, row] = grid[y][x];
            if (col == 0 && row == 0) continue;
            float posX{scale * tw * x};
            float posY{scale * th * y};
            layerVec.push_back(std::make_unique<Tile>(tilesetPath, scale, (int)tw, (int)th, col, row, posX, posY, window));
            ++totalPlaced;
          }
        }
        layerTiles->push_back(std::move(layerVec));
        ++layerIndex;
      }
      if (layerTiles->empty()) { std::cerr << "No valid layers parsed; aborting" << std::endl; return; }
      std::cout << "TileGroup: JSON layered loaded. Layers=" << layerTiles->size()
                << ", Size=" << COLS << "x" << ROWS << ", tiles=" << totalPlaced << std::endl;
    } else {
      // Single-layer JSON
      if (root.isMember("tileset")) this->textureUrlStr = root["tileset"].asString();
      if (root.isMember("tileW")) this->tileWidth = static_cast<float>(root["tileW"].asInt());
      if (root.isMember("tileH")) this->tileHeight = static_cast<float>(root["tileH"].asInt());
      auto grid = readGrid(root["grid"]);
      if (grid.empty() || grid[0].empty()) { std::cerr << "JSON map 'grid' is empty or malformed: " << filePathStr << std::endl; return; }
      ROWS = static_cast<int>(grid.size());
      COLS = static_cast<int>(grid[0].size());
      std::vector<std::unique_ptr<Tile>> singleLayer;
      singleLayer.reserve(static_cast<size_t>(ROWS * COLS));
      for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < static_cast<int>(grid[y].size()); ++x) {
          auto [col, row] = grid[y][x];
          if (col == 0 && row == 0) continue;
          float posX{scale * tileWidth * x};
          float posY{scale * tileHeight * y};
          singleLayer.push_back(std::make_unique<Tile>(this->textureUrlStr, scale, (int)tileWidth, (int)tileHeight, col, row, posX, posY, window));
          ++totalPlaced;
        }
      }
      layerTiles->push_back(std::move(singleLayer));
      std::cout << "TileGroup: JSON loaded. Size=" << COLS << "x" << ROWS << ", tiles=" << totalPlaced << std::endl;
    }
  } catch (const std::exception& ex) {
    std::cerr << "Exception while generating map: " << ex.what() << std::endl;
  }
}

void TileGroup::Draw()
{
  // Draw layers in order; per-layer tiles are already positioned
  if (!layerTiles) return;
  for (auto& layer : *layerTiles) {
    for (auto& tile : layer) {
      tile->Draw();
    }
  }
}
#include "TileGroup.hh"
#include <iostream>
#include <memory>
#include <sstream>
#include <filesystem>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

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
  // Decide format by file extension; support legacy .grid and new .json
  try {
    // Portable extension extraction without relying on std::filesystem
    std::string ext;
    const auto dot = filePathStr.find_last_of('.');
    if (dot != std::string::npos) {
      ext = filePathStr.substr(dot);
      // Normalize to lowercase
      for (auto& ch : ext) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    if (ext == ".json") {
      // New JSON format: { "tileset": "assets/tiles.png", "grid": [[[c,r], ...], ...] }
      // Read whole file into a string
      std::ifstream in(filePathStr, std::ios::in | std::ios::binary);
      if (!in.is_open()) {
        std::cerr << "Failed to open JSON map file: " << filePathStr << std::endl;
        return;
      }
      std::ostringstream ss; ss << in.rdbuf();
      const std::string json = ss.str();
      in.close();

      // Extract value of a string field like "tileset":"..."
      auto extractStringField = [](const std::string& src, const std::string& key) -> std::string {
        const std::string quoted = '"' + key + '"';
        size_t pos = src.find(quoted);
        if (pos == std::string::npos) return {};
        pos = src.find(':', pos);
        if (pos == std::string::npos) return {};
        pos = src.find('"', pos);
        if (pos == std::string::npos) return {};
        ++pos; // move past opening quote
        std::string out; out.reserve(128);
        bool escape = false;
        for (; pos < src.size(); ++pos) {
          char ch = src[pos];
          if (escape) { out.push_back(ch); escape = false; continue; }
          if (ch == '\\') { escape = true; continue; }
          if (ch == '"') break; // end
          out.push_back(ch);
        }
        return out;
      };

      // Extract the grid array starting from the first '[' after "grid":
      auto extractGridStart = [](const std::string& src) -> size_t {
        const std::string keyQuoted = "\"grid\"";
        size_t pos = src.find(keyQuoted);
        if (pos == std::string::npos) return std::string::npos;
        pos = src.find(':', pos);
        if (pos == std::string::npos) return std::string::npos;
        // find first '[' after colon
        pos = src.find('[', pos);
        return pos;
      };

      // Parse [[[c,r], ...], ...] to vector<vector<pair<int,int>>>
      auto parseGrid = [](const std::string& src, size_t startIdx)
          -> std::vector<std::vector<std::pair<int,int>>> {
        std::vector<std::vector<std::pair<int,int>>> grid;
        std::vector<std::pair<int,int>> currentRow;
        int depth = 0;
        std::string numBuf;
        std::vector<int> pairBuf; pairBuf.reserve(2);
        auto flushNumber = [&]() {
          if (!numBuf.empty()) {
            // convert
            try { pairBuf.push_back(std::stoi(numBuf)); } catch (...) {}
            numBuf.clear();
          }
        };
        for (size_t i = startIdx; i < src.size(); ++i) {
          char ch = src[i];
          if (ch == '[') { ++depth; continue; }
          if (ch == ']') {
            flushNumber();
            if (depth == 3) {
              // end of a pair [c,r]
              if (pairBuf.size() == 2) {
                currentRow.emplace_back(pairBuf[0], pairBuf[1]);
              }
              pairBuf.clear();
            } else if (depth == 2) {
              // end of a row [[...], [...]]
              // nothing to do here; pairs are appended as they close
            } else if (depth == 1) {
              // end of outer grid
              grid.push_back(std::move(currentRow));
              currentRow = {};
              break;
            }
            --depth;
            if (depth == 1 && ch == ']') {
              // End of a row
              if (!currentRow.empty()) {
                grid.push_back(std::move(currentRow));
                currentRow = {};
              }
            }
            continue;
          }
          if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+') {
            numBuf.push_back(ch);
          } else if (ch == ',' || std::isspace(static_cast<unsigned char>(ch))) {
            flushNumber();
          }
        }
        // Remove possible empty row pushed at the end
        while (!grid.empty() && grid.back().empty()) grid.pop_back();
        return grid;
      };

      const std::string parsedTileset = extractStringField(json, "tileset");
      if (!parsedTileset.empty()) {
        this->textureUrlStr = parsedTileset;
      }

      const size_t gridStart = extractGridStart(json);
      if (gridStart == std::string::npos) {
        std::cerr << "JSON map missing 'grid' field: " << filePathStr << std::endl;
        return;
      }
      auto grid = parseGrid(json, gridStart);
      if (grid.empty() || grid[0].empty()) {
        std::cerr << "JSON map 'grid' is empty or malformed: " << filePathStr << std::endl;
        return;
      }

      // Update ROWS and COLS based on parsed grid
      this->ROWS = static_cast<int>(grid.size());
      this->COLS = static_cast<int>(grid[0].size());

      for (int y = 0; y < this->ROWS; ++y) {
        if (static_cast<int>(grid[y].size()) != this->COLS) {
          std::cerr << "Row " << y << " has a different column count in JSON map: " << filePathStr << std::endl;
        }
        for (int x = 0; x < static_cast<int>(grid[y].size()); ++x) {
          const auto [col, row] = grid[y][x];
          float posX{scale * tileWidth * x};
          float posY{scale * tileHeight * y};
          tiles->push_back(std::make_unique<Tile>(this->textureUrlStr, scale, (int)tileWidth, (int)tileHeight, col, row, posX, posY, window));
        }
      }
    } else {
      // Legacy .grid format (two ints per tile: col row)
      reader->open(filePathStr);
      if (!reader->is_open()) {
        std::cerr << "Failed to open map file: " << filePathStr << std::endl;
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
            std::cerr << "Failed to read coordinate at position (" << x << ", " << y << ") from map file: " << filePathStr << std::endl;
            break;
          }
          int col{coord};
          if (!(*reader >> coord)) {
            std::cerr << "Failed to read coordinate at position (" << x << ", " << y << ") from map file: " << filePathStr << std::endl;
            break;
          }
          int row{coord};
          tiles->push_back(std::make_unique<Tile>(textureUrlStr, scale, (int)tileWidth, (int)tileHeight, col, row, posX, posY, window));
        }
      }
      reader->close();
    }
  } catch (const std::exception& ex) {
    std::cerr << "Exception while generating map: " << ex.what() << std::endl;
  }
}

void TileGroup::Draw()
{
  for(auto& tile : *tiles)
  {
    tile->Draw();
  }
}
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
  layerTiles = std::make_unique<std::vector<std::vector<std::unique_ptr<Tile>>>>();

  GenerateMap();
}

TileGroup::~TileGroup()
{
  // Smart pointers automatically clean up
}

void TileGroup::GenerateMap()
{
  // Decide format by file extension; support legacy .grid and new .json (with optional layers)
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
      std::cout << "TileGroup: loading JSON map -> " << filePathStr << std::endl;
      // JSON formats supported:
      // 1) Single-layer: { "tileset": "assets/tiles.png", "tileW":16, "tileH":16, "grid": [[[c,r],...], ...] }
      // 2) Multi-layer:  { "layers": [ {"name":"Base","tileset":"...","tileW":16,"tileH":16,"grid":[...]} , ... ] }
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

      // Extract int field like "tileW": 16
      auto extractIntField = [](const std::string& src, const std::string& key) -> int {
        const std::string quoted = '"' + key + '"';
        size_t pos = src.find(quoted);
        if (pos == std::string::npos) return 0;
        pos = src.find(':', pos);
        if (pos == std::string::npos) return 0;
        ++pos;
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t')) ++pos;
        std::string num;
        while (pos < src.size() && (std::isdigit(static_cast<unsigned char>(src[pos])) || src[pos] == '-')) num += src[pos++];
        try { return std::stoi(num); } catch (...) { return 0; }
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

      // Detect layered format
      auto hasKey = [&](const std::string& key) -> bool {
        const std::string quoted = '"' + key + '"';
        return json.find(quoted) != std::string::npos;
      };

      bool isLayered = hasKey("layers");
      int totalPlaced = 0;
      layerTiles->clear();

      if (isLayered) {
        // Very simple layers parser: iterate by occurrences of objects inside layers array
        // Find layers array start
        size_t layersPos = json.find("\"layers\"");
        if (layersPos == std::string::npos) {
          std::cerr << "JSON declared layers but none found: " << filePathStr << std::endl;
          return;
        }
        size_t arrStart = json.find('[', json.find(':', layersPos));
        if (arrStart == std::string::npos) { std::cerr << "Invalid layers array" << std::endl; return; }

        // Iterate through layer objects by scanning for '{' ... '}' at top-level within array
        size_t i = arrStart + 1;
        int layerIndex = 0;
        while (i < json.size()) {
          // Find next object start
          size_t objStart = json.find('{', i);
          if (objStart == std::string::npos) break;
          // Find matching '}' for this object with simple brace counting
          int depth = 0; size_t objEnd = objStart;
          for (; objEnd < json.size(); ++objEnd) {
            char ch = json[objEnd];
            if (ch == '{') ++depth; else if (ch == '}') { --depth; if (depth == 0) { ++objEnd; break; } }
          }
          if (objEnd <= objStart) break;
          std::string layerObj = json.substr(objStart, objEnd - objStart);
          i = objEnd;

          // Parse fields from this layer object
          std::string lTileset = extractStringField(layerObj, "tileset");
          int lTileW = extractIntField(layerObj, "tileW");
          int lTileH = extractIntField(layerObj, "tileH");
          size_t gridStart = extractGridStart(layerObj);
          if (gridStart == std::string::npos) { std::cerr << "Layer " << layerIndex << " missing grid" << std::endl; continue; }
          auto grid = parseGrid(layerObj, gridStart);
          if (grid.empty()) { std::cerr << "Layer " << layerIndex << " grid empty" << std::endl; continue; }

          // On first layer, set COLS/ROWS from grid
          if (layerIndex == 0) {
            ROWS = static_cast<int>(grid.size());
            COLS = static_cast<int>(grid[0].size());
          }
          // Determine tileset path and tile size for this layer (fallback to root defaults)
          std::string tilesetPath = !lTileset.empty() ? lTileset : this->textureUrlStr;
          float tw = (lTileW > 0 ? static_cast<float>(lTileW) : this->tileWidth);
          float th = (lTileH > 0 ? static_cast<float>(lTileH) : this->tileHeight);

          std::vector<std::unique_ptr<Tile>> layerVec;
          layerVec.reserve(static_cast<size_t>(ROWS * COLS));
          for (int y = 0; y < static_cast<int>(grid.size()); ++y) {
            for (int x = 0; x < static_cast<int>(grid[y].size()); ++x) {
              const auto [col, row] = grid[y][x];
              float posX{scale * tw * x};
              float posY{scale * th * y};
              layerVec.push_back(std::make_unique<Tile>(tilesetPath, scale, (int)tw, (int)th, col, row, posX, posY, window));
              ++totalPlaced;
            }
          }
          layerTiles->push_back(std::move(layerVec));
          ++layerIndex;

          // Stop when layers array likely ended
          size_t nextComma = json.find(',', objEnd);
          size_t arrEnd = json.find(']', arrStart);
          if (arrEnd != std::string::npos && (nextComma == std::string::npos || arrEnd < nextComma)) break;
        }
        if (layerTiles->empty()) {
          std::cerr << "No valid layers parsed; aborting" << std::endl;
          return;
        }
        std::cout << "TileGroup: JSON layered loaded. Layers=" << layerTiles->size()
                  << ", Size=" << COLS << "x" << ROWS << ", tiles=" << totalPlaced << std::endl;
      } else {
        // Single-layer JSON (back-compat)
        const std::string parsedTileset = extractStringField(json, "tileset");
        if (!parsedTileset.empty()) { this->textureUrlStr = parsedTileset; }
        int jsonTileW = extractIntField(json, "tileW");
        int jsonTileH = extractIntField(json, "tileH");
        if (jsonTileW > 0) this->tileWidth = static_cast<float>(jsonTileW);
        if (jsonTileH > 0) this->tileHeight = static_cast<float>(jsonTileH);

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

        std::vector<std::unique_ptr<Tile>> singleLayer;
        singleLayer.reserve(static_cast<size_t>(ROWS * COLS));
        for (int y = 0; y < this->ROWS; ++y) {
          for (int x = 0; x < static_cast<int>(grid[y].size()); ++x) {
            const auto [col, row] = grid[y][x];
            float posX{scale * tileWidth * x};
            float posY{scale * tileHeight * y};
            singleLayer.push_back(std::make_unique<Tile>(this->textureUrlStr, scale, (int)tileWidth, (int)tileHeight, col, row, posX, posY, window));
            ++totalPlaced;
          }
        }
        layerTiles->push_back(std::move(singleLayer));
        std::cout << "TileGroup: JSON loaded. Size=" << this->COLS << "x" << this->ROWS << ", tiles=" << totalPlaced << std::endl;
      }
    } else {
    std::cout << "TileGroup: loading GRID map -> " << filePathStr << std::endl;
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
          // Legacy grid is treated as a single layer
          if (layerTiles->empty()) layerTiles->push_back({});
          (*layerTiles)[0].push_back(std::make_unique<Tile>(textureUrlStr, scale, (int)tileWidth, (int)tileHeight, col, row, posX, posY, window));
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
  // Draw layers in order; per-layer tiles are already positioned
  if (!layerTiles) return;
  for (auto& layer : *layerTiles) {
    for (auto& tile : layer) {
      tile->Draw();
    }
  }
}
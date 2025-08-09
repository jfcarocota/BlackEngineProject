#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <limits.h>
#ifdef __APPLE__
  #include <mach-o/dyld.h>
#endif

#include "Constants.hh"

struct TileCR {
  int col{0};
  int row{0};
};

// Resolve asset paths when running from different working directories or a macOS .app bundle
static std::string getExecutableDir() {
  std::string dir;
#ifdef __APPLE__
  char buf[PATH_MAX];
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0) {
    std::error_code ec;
    auto p = std::filesystem::weakly_canonical(std::filesystem::path(buf), ec);
    if (!ec) dir = p.parent_path().string();
  }
#else
  // On non-Apple systems you could add platform-specific resolution if needed
#endif
  return dir;
}

static std::string findAssetPath(const std::string& relative) {
  // 1) As provided (relative to CWD or absolute)
  std::filesystem::path p(relative);
  if (std::filesystem::exists(p)) return p.string();

  // 2) Try common fallbacks relative to CWD
  std::vector<std::string> relPrefixes = {"./", "../", "../../", "../../../"};
  for (const auto& pref : relPrefixes) {
    std::filesystem::path cand = std::filesystem::path(pref) / relative;
    if (std::filesystem::exists(cand)) return cand.string();
  }

  // 3) Try relative to the executable location
  std::string exeDir = getExecutableDir();
  if (!exeDir.empty()) {
    std::filesystem::path cand = std::filesystem::path(exeDir) / relative;
    if (std::filesystem::exists(cand)) return cand.string();

#ifdef __APPLE__
    // If inside a .app bundle: exeDir = <App>.app/Contents/MacOS
    auto macosDir = std::filesystem::path(exeDir);
    if (macosDir.filename() == "MacOS" && macosDir.parent_path().filename() == "Contents") {
      std::filesystem::path resources = macosDir.parent_path() / "Resources" / relative;
      if (std::filesystem::exists(resources)) return resources.string();
      // Also support apps where assets are placed directly under Resources without the leading folder
      // e.g., relative = "assets/tiles.png" might also exist as Resources/assets/tiles.png (already checked)
    }
#endif
  }

  // As a last resort, return the original
  return relative;
}

// User-writable directory for saved maps
static std::filesystem::path getUserMapsDir() {
  std::filesystem::path home;
#ifdef _WIN32
  const char* appData = std::getenv("APPDATA");
  if (appData) home = appData;
  else home = std::filesystem::path(".");
  return home / "BlackEngineProject" / "Maps";
#else
  const char* h = std::getenv("HOME");
  if (h) home = h; else home = ".";
#ifdef __APPLE__
  return home / "Library" / "Application Support" / "BlackEngineProject" / "Maps";
#else
  return home / ".local" / "share" / "BlackEngineProject" / "Maps";
#endif
#endif
}

static std::string timestampName() {
  std::time_t t = std::time(nullptr);
  std::tm tm{};
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  char buf[32];
  std::strftime(buf, sizeof(buf), "map_%Y%m%d_%H%M%S.grid", &tm);
  return std::string(buf);
}

struct Tileset {
  sf::Texture texture;
  sf::Image image; // optional for future use
  std::string path;
  int tileSize{static_cast<int>(GameConstants::TILE_SIZE)};
  int cols{0};
  int rows{0};
  bool loaded{false};

  bool loadFromFile(const std::string& p, int tile) {
    path = p;
    tileSize = tile;
    loaded = false;
    if (!texture.loadFromFile(path)) {
      std::cerr << "Failed to load tileset: " << path << std::endl;
      return false;
    }
    image = texture.copyToImage();
    cols = texture.getSize().x / tileSize;
    rows = texture.getSize().y / tileSize;
    if (cols <= 0 || rows <= 0) {
      std::cerr << "Tileset size smaller than tile size." << std::endl;
      return false;
    }
    loaded = true;
    return true;
  }
};

int main() {
  // Layout constants
  const int gridCols = GameConstants::MAP_WIDTH;
  const int gridRows = GameConstants::MAP_HEIGHT;
  const int tilePx = static_cast<int>(GameConstants::TILE_SIZE);
  const float tileScale = GameConstants::TILE_SCALE;

  const int paletteWidth = 280; // left panel
  const int margin = 12;
  const int gridPxW = static_cast<int>(gridCols * tilePx * tileScale);
  const int gridPxH = static_cast<int>(gridRows * tilePx * tileScale);
  const int winW = paletteWidth + margin + gridPxW + margin;
  const int winH = std::max(gridPxH + margin * 2, 720);

  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(winW, winH)), "Tile Map Editor");
  window.setFramerateLimit(60);

  // Load font for UI (SFML 3: openFromFile only)
  sf::Font font;
  if (!font.openFromFile(findAssetPath(ASSETS_FONT_ARCADECLASSIC))) {
    std::cerr << "Failed to load font: " << ASSETS_FONT_ARCADECLASSIC << std::endl;
  }

  // Tileset
  Tileset tileset;
  std::string tilesetPath = findAssetPath(ASSETS_TILES); // default
  if (!tileset.loadFromFile(tilesetPath, tilePx)) {
    // Defer info message setup; will be initialized below
  }

  // Grid data (col,row for each cell)
  std::vector<std::vector<TileCR>> grid(gridRows, std::vector<TileCR>(gridCols, TileCR{0, 0}));

  // Selected tile in palette (col,row)
  TileCR selected{0, 0};

  // Text input state for loading tileset
  bool enteringPath = false;
  std::string pathBuffer = tilesetPath;
  std::string infoMessage;
  sf::Clock infoClock;

  auto gridOrigin = sf::Vector2f(static_cast<float>(paletteWidth + margin), static_cast<float>(margin));

  auto showInfo = [&](const std::string& msg) {
    infoMessage = msg;
    infoClock.restart();
  };

  // If initial tileset load failed, show a hint on screen (useful when launched from Finder)
  if (!tileset.loaded) {
    showInfo(std::string("Failed to load default tileset. Press L to select path or ensure assets are in the app Resources folder. Tried: ") + tilesetPath);
  }

  auto saveGrid = [&]() {
    auto mapsDir = getUserMapsDir();
    std::error_code ec;
    std::filesystem::create_directories(mapsDir, ec);
    if (ec) {
      showInfo(std::string("Failed to create maps dir: ") + mapsDir.string());
      return;
    }

    std::string fileName = timestampName();
    std::filesystem::path outPath = mapsDir / fileName;

    std::ofstream out(outPath);
    if (!out.is_open()) {
      showInfo("Failed to save: " + outPath.string());
      return;
    }

    // Write pairs: col row per tile, rows by lines
    for (int y = 0; y < gridRows; ++y) {
      for (int x = 0; x < gridCols; ++x) {
        out << grid[y][x].col << ' ' << grid[y][x].row;
        if (x != gridCols - 1) out << ' ';
      }
      out << '\n';
    }
    out.close();
    showInfo(std::string("Saved -> ") + outPath.string());
  };

  auto loadGridFromFile = [&](const std::string& filePath) {
    std::string resolved = findAssetPath(filePath);
    std::ifstream in(resolved);
    if (!in.is_open()) {
      showInfo("Failed to open: " + resolved);
      return;
    }
    for (int y = 0; y < gridRows; ++y) {
      for (int x = 0; x < gridCols; ++x) {
        int c, r;
        if (!(in >> c >> r)) {
          showInfo("Invalid grid file format");
          return;
        }
        grid[y][x] = TileCR{c, r};
      }
    }
    showInfo("Loaded grid from: " + resolved);
  };

  // Load default map if available (works both from CLI and .app bundle)
  {
    std::string defaultMap = findAssetPath(ASSETS_MAPS);
    if (!defaultMap.empty() && std::filesystem::exists(defaultMap)) {
      loadGridFromFile(defaultMap);
    }
  }

  // Simple helpers for drawing
  sf::RectangleShape paletteBG(sf::Vector2f(static_cast<float>(paletteWidth), static_cast<float>(winH)));
  paletteBG.setFillColor(sf::Color(30, 30, 40));

  sf::RectangleShape gridBG(sf::Vector2f(static_cast<float>(gridPxW), static_cast<float>(gridPxH)));
  gridBG.setFillColor(sf::Color(10, 15, 20));
  gridBG.setPosition(gridOrigin);
  gridBG.setOutlineThickness(1);
  gridBG.setOutlineColor(sf::Color(60, 60, 70));

  // Palette draw origin and layout (shared by draw and hit-test)
  const float thumbScale = 2.0f;
  const int cell = static_cast<int>(tilePx * thumbScale);
  const int padding = 6;
  const int x0 = 8;
  const int y0 = 92;

  while (window.isOpen()) {
    // SFML 3 event polling
    while (auto ev = window.pollEvent()) {
      const sf::Event& event = *ev;
      if (event.is<sf::Event::Closed>()) window.close();

      if (event.is<sf::Event::KeyPressed>()) {
        const auto* e = event.getIf<sf::Event::KeyPressed>();
        if (e) {
          if (!enteringPath) {
            if (e->code == sf::Keyboard::Key::Escape) window.close();
            if (e->code == sf::Keyboard::Key::S) saveGrid();
            if (e->code == sf::Keyboard::Key::N) {
              for (auto& row : grid) for (auto& cell : row) cell = TileCR{0, 0};
              showInfo("New empty grid");
            }
            if (e->code == sf::Keyboard::Key::L) {
              enteringPath = true;
              pathBuffer = tilesetPath;
              showInfo("Type tileset path and press Enter");
            }
            if (e->code == sf::Keyboard::Key::O) {
              loadGridFromFile(ASSETS_MAPS);
            }
          } else {
            if (e->code == sf::Keyboard::Key::Enter) {
              if (tileset.loadFromFile(pathBuffer, tilePx)) {
                tilesetPath = pathBuffer;
                showInfo("Loaded tileset: " + tilesetPath);
              } else {
                showInfo("Failed tileset: " + pathBuffer);
              }
              enteringPath = false;
            }
            if (e->code == sf::Keyboard::Key::Escape) {
              enteringPath = false;
            }
          }
        }
      }

      if (enteringPath && event.is<sf::Event::TextEntered>()) {
        const auto* e = event.getIf<sf::Event::TextEntered>();
        if (e) {
          if (e->unicode == 8) { // backspace
            if (!pathBuffer.empty()) pathBuffer.pop_back();
          } else if (e->unicode >= 32 && e->unicode < 127) {
            pathBuffer += static_cast<char>(e->unicode);
          }
        }
      }

      if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* e = event.getIf<sf::Event::MouseButtonPressed>();
        if (!e) continue;
        sf::Vector2i mp = sf::Mouse::getPosition(window);

        // Click in palette area to select tile
        if (mp.x >= 0 && mp.x < paletteWidth) {
          if (!tileset.loaded) continue;
          const int colsPerRow = std::max(1, (paletteWidth - padding - x0) / (cell + padding));

          int localX = mp.x - x0;
          int localY = mp.y - y0;
          if (localX >= 0 && localY >= 0) {
            int tx = localX / (cell + padding);
            int ty = localY / (cell + padding);
            if (tx >= 0 && ty >= 0 && tx < colsPerRow) {
              int index = ty * colsPerRow + tx;
              int selCol = index % tileset.cols;
              int selRow = index / tileset.cols;
              if (selRow < tileset.rows) {
                selected = TileCR{selCol, selRow};
              }
            }
          }
        }

        // Click in grid to paint / erase
        sf::FloatRect gridRect({gridOrigin.x, gridOrigin.y}, {static_cast<float>(gridPxW), static_cast<float>(gridPxH)});
        if (gridRect.contains(sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y)))) {
          int gx = static_cast<int>((mp.x - gridOrigin.x) / (tilePx * tileScale));
          int gy = static_cast<int>((mp.y - gridOrigin.y) / (tilePx * tileScale));
          if (gx >= 0 && gx < gridCols && gy >= 0 && gy < gridRows) {
            if (e->button == sf::Mouse::Button::Left) {
              grid[gy][gx] = selected;
            } else if (e->button == sf::Mouse::Button::Right) {
              grid[gy][gx] = TileCR{0, 0};
            }
          }
        }
      }
    }

    window.clear(sf::Color(18, 18, 24));

    // Left palette background
    paletteBG.setPosition(sf::Vector2f(0.f, 0.f));
    window.draw(paletteBG);

    // Palette title
    sf::Text title(font);
    title.setString("Tileset");
    title.setCharacterSize(22);
    title.setFillColor(sf::Color(230, 230, 240));
    title.setPosition(sf::Vector2f(16.f, 10.f));
    window.draw(title);

    // Tileset path / load help
    {
      sf::Text help(font);
      help.setCharacterSize(14);
      help.setFillColor(sf::Color(180, 180, 200));
      help.setPosition(sf::Vector2f(16.f, 36.f));
      help.setString("L: load tileset  •  S: save  •  N: new  •  O: load level1");
      window.draw(help);

      if (enteringPath) {
        sf::RectangleShape box(sf::Vector2f(static_cast<float>(paletteWidth - 24), 26.f));
        box.setFillColor(sf::Color(50, 50, 60));
        box.setOutlineThickness(1);
        box.setOutlineColor(sf::Color(90, 90, 110));
        box.setPosition(sf::Vector2f(12.f, 62.f));
        window.draw(box);

        sf::Text path(font);
        path.setCharacterSize(16);
        path.setFillColor(sf::Color::White);
        path.setPosition(sf::Vector2f(18.f, 64.f));
        path.setString(pathBuffer);
        window.draw(path);
      } else {
        sf::Text path(font);
        path.setCharacterSize(12);
        path.setFillColor(sf::Color(160, 160, 180));
        path.setPosition(sf::Vector2f(16.f, 62.f));
        path.setString(std::string("Tileset: ") + tilesetPath);
        window.draw(path);

        // Show where files are saved
        sf::Text saveHint(font);
        saveHint.setCharacterSize(12);
        saveHint.setFillColor(sf::Color(150, 200, 160));
        saveHint.setPosition(sf::Vector2f(16.f, 78.f));
        saveHint.setString(std::string("Saves in: ") + getUserMapsDir().string());
        window.draw(saveHint);
      }
    }

    // Draw palette thumbnails
    if (tileset.loaded) {
      const int colsPerRow = std::max(1, (paletteWidth - padding - x0) / (cell + padding));
      for (int r = 0; r < tileset.rows; ++r) {
        for (int c = 0; c < tileset.cols; ++c) {
          int i = r * tileset.cols + c;
          int tx = (i % colsPerRow);
          int ty = (i / colsPerRow);
          int px = x0 + tx * (cell + padding);
          int py = y0 + ty * (cell + padding);

          sf::Sprite spr(tileset.texture, sf::IntRect({c * tileset.tileSize, r * tileset.tileSize}, {tileset.tileSize, tileset.tileSize}));
          spr.setPosition(sf::Vector2f(static_cast<float>(px), static_cast<float>(py)));
          spr.setScale(sf::Vector2f(thumbScale, thumbScale));
          window.draw(spr);

          // Highlight selected
          if (selected.col == c && selected.row == r) {
            sf::RectangleShape sel(sf::Vector2f(static_cast<float>(cell), static_cast<float>(cell)));
            sel.setPosition(sf::Vector2f(static_cast<float>(px), static_cast<float>(py)));
            sel.setFillColor(sf::Color::Transparent);
            sel.setOutlineThickness(2);
            sel.setOutlineColor(sf::Color(255, 215, 0));
            window.draw(sel);
          }
        }
      }
    }

    // Grid panel
    window.draw(gridBG);

    // Grid lines
    for (int x = 0; x <= gridCols; ++x) {
      sf::RectangleShape line(sf::Vector2f(1.f, static_cast<float>(gridPxH)));
      line.setFillColor(sf::Color(45, 45, 55));
      line.setPosition(sf::Vector2f(gridOrigin.x + x * tilePx * tileScale, gridOrigin.y));
      window.draw(line);
    }
    for (int y = 0; y <= gridRows; ++y) {
      sf::RectangleShape line(sf::Vector2f(static_cast<float>(gridPxW), 1.f));
      line.setFillColor(sf::Color(45, 45, 55));
      line.setPosition(sf::Vector2f(gridOrigin.x, gridOrigin.y + y * tilePx * tileScale));
      window.draw(line);
    }

    // Draw placed tiles
    if (tileset.loaded) {
      for (int y = 0; y < gridRows; ++y) {
        for (int x = 0; x < gridCols; ++x) {
          const TileCR& t = grid[y][x];
          sf::Sprite spr(tileset.texture, sf::IntRect({t.col * tilePx, t.row * tilePx}, {tilePx, tilePx}));
          spr.setScale(sf::Vector2f(tileScale, tileScale));
          spr.setPosition(sf::Vector2f(gridOrigin.x + x * tilePx * tileScale, gridOrigin.y + y * tilePx * tileScale));
          window.draw(spr);
        }
      }
    }

    // Hover highlight on grid
    sf::Vector2i mp = sf::Mouse::getPosition(window);
    sf::FloatRect gridRect({gridOrigin.x, gridOrigin.y}, {static_cast<float>(gridPxW), static_cast<float>(gridPxH)});
    if (gridRect.contains(sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y)))) {
      int gx = static_cast<int>((mp.x - gridOrigin.x) / (tilePx * tileScale));
      int gy = static_cast<int>((mp.y - gridOrigin.y) / (tilePx * tileScale));
      sf::RectangleShape hover(sf::Vector2f(tilePx * tileScale, tilePx * tileScale));
      hover.setPosition(sf::Vector2f(gridOrigin.x + gx * tilePx * tileScale, gridOrigin.y + gy * tilePx * tileScale));
      hover.setFillColor(sf::Color(255, 255, 255, 20));
      hover.setOutlineThickness(1);
      hover.setOutlineColor(sf::Color(255, 255, 255, 60));
      window.draw(hover);
    }

    // Info message (auto-fade after 3s)
    if (!infoMessage.empty()) {
      float s = infoClock.getElapsedTime().asSeconds();
      if (s < 3.0f) {
        sf::Text msg(font);
        msg.setCharacterSize(16);
        msg.setFillColor(sf::Color(240, 240, 250));
        msg.setPosition(sf::Vector2f(16.f, static_cast<float>(winH - 32)));
        msg.setString(infoMessage);
        window.draw(msg);
      }
    }

    window.display();
  }

  return 0;
}

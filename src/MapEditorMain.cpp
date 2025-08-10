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
#include <cstdio>
#include <algorithm>
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

// macOS-only: show a folder picker using AppleScript (osascript)
#ifdef __APPLE__
static std::optional<std::string> macChooseFolder() {
  const char* cmd = "osascript -e 'POSIX path of (choose folder with prompt \"Select save folder\")' 2>/dev/null";
  std::string result;
  FILE* pipe = popen(cmd, "r");
  if (!pipe) return std::nullopt;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    result += buffer;
  }
  pclose(pipe);
  // trim whitespace/newlines
  auto isSpace = [](unsigned char c){ return std::isspace(c); };
  while (!result.empty() && isSpace(result.back())) result.pop_back();
  while (!result.empty() && isSpace(result.front())) result.erase(result.begin());
  if (result.empty()) return std::nullopt;
  return result;
}
#endif

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
  int winW = paletteWidth + margin + gridPxW + margin;
  int winH = std::max(gridPxH + margin * 2, 720);

  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(winW, winH)), "Tile Map Editor");
  window.setFramerateLimit(60);

  // Create views to isolate palette UI and prevent overlap into grid
  sf::View defaultView = window.getView();
  sf::View paletteView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(paletteWidth), static_cast<float>(winH)}));
  paletteView.setViewport(sf::FloatRect({0.f, 0.f}, {static_cast<float>(paletteWidth) / static_cast<float>(winW), 1.f}));

  // Load font for UI (SFML 3: openFromFile only)
  sf::Font font;
  if (!font.openFromFile(findAssetPath(ASSETS_FONT_ARCADECLASSIC))) {
    std::cerr << "Failed to load font: " << ASSETS_FONT_ARCADECLASSIC << std::endl;
  }

  // Helper text fitting utilities to avoid overlapping UI into grid
  auto ellipsizeEnd = [&](const std::string& text, unsigned int charSize, float maxWidth) {
    sf::Text measure(font);
    measure.setCharacterSize(charSize);
    measure.setString(text);
    float w = measure.getLocalBounds().size.x;
    if (w <= maxWidth) return text;
    const std::string dots = "...";
    measure.setString(dots);
    float dotsW = measure.getLocalBounds().size.x;
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
      out.push_back(c);
      measure.setString(out);
      if (measure.getLocalBounds().size.x + dotsW > maxWidth) {
        out.pop_back();
        break;
      }
    }
    return out + dots;
  };
  auto ellipsizeStart = [&](const std::string& text, unsigned int charSize, float maxWidth) {
    sf::Text measure(font);
    measure.setCharacterSize(charSize);
    measure.setString(text);
    float w = measure.getLocalBounds().size.x;
    if (w <= maxWidth) return text;
    const std::string dots = "...";
    measure.setString(dots);
    float dotsW = measure.getLocalBounds().size.x;
    std::string tail;
    tail.reserve(text.size());
    for (auto it = text.rbegin(); it != text.rend(); ++it) {
      tail.push_back(*it);
      std::string rev = tail;
      std::reverse(rev.begin(), rev.end());
      measure.setString(rev);
      if (measure.getLocalBounds().size.x + dotsW > maxWidth) {
        tail.pop_back();
        break;
      }
    }
    std::string tailForward = tail;
    std::reverse(tailForward.begin(), tailForward.end());
    return dots + tailForward;
  };

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
  // Save directory input state
  bool enteringSaveDir = false;
  std::string saveDirPath = getUserMapsDir().string();
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
    std::filesystem::path mapsDir = saveDirPath.empty() ? getUserMapsDir() : std::filesystem::path(saveDirPath);
    std::error_code ec;
    if (!std::filesystem::exists(mapsDir)) {
      std::filesystem::create_directories(mapsDir, ec);
      if (ec) {
        showInfo(std::string("Failed to create maps dir: ") + mapsDir.string());
        return;
      }
    } else if (!std::filesystem::is_directory(mapsDir)) {
      showInfo(std::string("Not a directory: ") + mapsDir.string());
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
  // Shift tileset thumbnails down to make space for save controls
  const int y0 = 180;
  // Save controls layout
  const int saveLabelY = 92;
  const int saveInputY = 112;
  const int saveButtonsY = 144;

  // --- Palette vertical scrolling state & helpers ---
  float paletteScrollY = 0.f; // in palette coordinate space
  auto computePaletteContentHeight = [&]() -> float {
    // Base content up to start of thumbnails
    float content = static_cast<float>(y0);
    if (tileset.loaded) {
      int colsPerRow = std::max(1, (paletteWidth - padding - x0) / (cell + padding));
      int total = tileset.rows * tileset.cols;
      int rowsNeeded = (total + colsPerRow - 1) / colsPerRow;
      content = std::max(content, static_cast<float>(y0 + rowsNeeded * (cell + padding) + padding));
    }
    // Ensure at least window height so we don't get negative maxScroll
    return std::max(content, static_cast<float>(winH));
  };
  auto clampAndApplyPaletteScroll = [&]() {
    float contentH = computePaletteContentHeight();
    float maxScroll = std::max(0.f, contentH - static_cast<float>(winH));
    if (paletteScrollY < 0.f) paletteScrollY = 0.f;
    if (paletteScrollY > maxScroll) paletteScrollY = maxScroll;
    paletteView.setSize(sf::Vector2f(static_cast<float>(paletteWidth), static_cast<float>(winH)));
    paletteView.setCenter(sf::Vector2f(static_cast<float>(paletteWidth) / 2.f,
                                       static_cast<float>(winH) / 2.f + paletteScrollY));
  };
  // Helper to get scrollbar track and thumb rects in palette-view coordinates
  auto getScrollbarRects = [&]() -> std::optional<std::pair<sf::FloatRect, sf::FloatRect>> {
    float contentH = computePaletteContentHeight();
    if (contentH <= static_cast<float>(winH)) return std::nullopt; // no scrollbar
    float trackW = 8.f;                       // widened for easier interaction
    float trackX = static_cast<float>(paletteWidth) - (trackW + 2.f); // 2px margin
    float trackH = static_cast<float>(winH) - 8.f;
    float viewTop = paletteView.getCenter().y - paletteView.getSize().y / 2.f;
    float trackY = viewTop + 4.f;
    float thumbH = std::max(24.f, trackH * (static_cast<float>(winH) / contentH));
    float maxScroll = contentH - static_cast<float>(winH);
    float t = (maxScroll > 0.f) ? (paletteScrollY / maxScroll) : 0.f;
    float thumbY = trackY + t * (trackH - thumbH);
    sf::FloatRect track(sf::Vector2f(trackX, trackY), sf::Vector2f(trackW, trackH));
    sf::FloatRect thumb(sf::Vector2f(trackX, thumbY), sf::Vector2f(trackW, thumbH));
    return std::make_pair(track, thumb);
  };
  // Dragging state for scrollbar thumb
  bool draggingScrollThumb = false;
  float dragOffsetY = 0.f; // distance from mouse Y to thumb top when drag starts

  // Painting state for grid drag drawing/erasing
  bool paintingLeft = false;
  bool paintingRight = false;
  int lastPaintGX = -1;
  int lastPaintGY = -1;
  auto paintCell = [&](int gx, int gy, bool leftButton) {
    if (gx >= 0 && gx < gridCols && gy >= 0 && gy < gridRows) {
      if (leftButton) grid[gy][gx] = selected; else grid[gy][gx] = TileCR{0, 0};
    }
  };
  auto paintLine = [&](int x0, int y0, int x1, int y1, bool leftButton) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int steps = std::max(adx, ady);
    if (steps <= 0) { paintCell(x1, y1, leftButton); return; }
    // Step-inclusive from previous cell toward current to avoid gaps on fast mouse moves
    for (int i = 1; i <= steps; ++i) {
      int x = x0 + (dx * i + (dx >= 0 ? steps / 2 : -steps / 2)) / steps;
      int y = y0 + (dy * i + (dy >= 0 ? steps / 2 : -steps / 2)) / steps;
      paintCell(x, y, leftButton);
    }
  };

  // Initialize view center with scroll
  clampAndApplyPaletteScroll();

  while (window.isOpen()) {
    // SFML 3 event polling
    while (auto ev = window.pollEvent()) {
      const sf::Event& event = *ev;
      if (event.is<sf::Event::Closed>()) window.close();

      // Cancel dragging when window loses focus
      if (event.is<sf::Event::FocusLost>()) {
        draggingScrollThumb = false;
        // Cancel any in-progress painting to avoid stuck state
        paintingLeft = false;
        paintingRight = false;
        lastPaintGX = lastPaintGY = -1;
      }

      // Keep views and palette size in sync with window to avoid overlap on resize
      if (event.is<sf::Event::Resized>()) {
        auto sz = window.getSize();
        winW = static_cast<int>(sz.x);
        winH = static_cast<int>(sz.y);
        defaultView = sf::View(sf::FloatRect({0.f, 0.f}, {static_cast<float>(winW), static_cast<float>(winH)}));
        window.setView(defaultView);
        paletteView.setViewport(sf::FloatRect({0.f, 0.f}, {static_cast<float>(paletteWidth) / static_cast<float>(winW), 1.f}));
        paletteBG.setSize(sf::Vector2f(static_cast<float>(paletteWidth), static_cast<float>(winH)));
        clampAndApplyPaletteScroll();
      }

      // Mouse wheel scroll to move palette
      if (event.is<sf::Event::MouseWheelScrolled>()) {
        const auto* e = event.getIf<sf::Event::MouseWheelScrolled>();
        if (e) {
          // Only scroll when mouse is over the left palette area
          sf::Vector2i mp = sf::Mouse::getPosition(window);
          if (mp.x >= 0 && mp.x < paletteWidth) {
            // delta > 0 typically means scroll up
            float step = 60.f; // pixels per notch; trackpads provide fractional deltas
            paletteScrollY -= e->delta * step;
            clampAndApplyPaletteScroll();
          }
        }
      }

      // Handle dragging of the scrollbar thumb and grid paint-drag
      if (event.is<sf::Event::MouseMoved>()) {
        const auto* me = event.getIf<sf::Event::MouseMoved>();
        if (!me) continue;

        // Scrollbar thumb dragging (palette view coords)
        if (draggingScrollThumb) {
          sf::Vector2i mp(me->position.x, me->position.y);
          sf::Vector2f mpPaletteF = window.mapPixelToCoords(mp, paletteView);
          if (auto rects = getScrollbarRects()) {
            const auto& track = rects->first;
            const auto& thumb = rects->second;
            float trackY = track.position.y;
            float trackH = track.size.y;
            float thumbH = thumb.size.y;
            float contentH = computePaletteContentHeight();
            float maxScroll = contentH - static_cast<float>(winH);
            float newThumbTop = mpPaletteF.y - dragOffsetY;
            // clamp thumb within track
            float minThumbTop = trackY;
            float maxThumbTop = trackY + trackH - thumbH;
            if (newThumbTop < minThumbTop) newThumbTop = minThumbTop;
            if (newThumbTop > maxThumbTop) newThumbTop = maxThumbTop;
            float t = (trackH - thumbH) > 0.f ? (newThumbTop - trackY) / (trackH - thumbH) : 0.f;
            paletteScrollY = t * std::max(0.f, maxScroll);
            clampAndApplyPaletteScroll();
          }
        }

        // Grid painting while holding mouse button (default view coords)
        if (paintingLeft || paintingRight) {
          sf::Vector2i mp(me->position.x, me->position.y);
          sf::FloatRect gridRect({gridOrigin.x, gridOrigin.y}, {static_cast<float>(gridPxW), static_cast<float>(gridPxH)});
          if (gridRect.contains(sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y)))) {
            int gx = static_cast<int>((mp.x - gridOrigin.x) / (tilePx * tileScale));
            int gy = static_cast<int>((mp.y - gridOrigin.y) / (tilePx * tileScale));
            if (gx != lastPaintGX || gy != lastPaintGY) {
              bool leftBtn = paintingLeft;
              if (lastPaintGX >= 0 && lastPaintGY >= 0) paintLine(lastPaintGX, lastPaintGY, gx, gy, leftBtn);
              else paintCell(gx, gy, leftBtn);
              lastPaintGX = gx; lastPaintGY = gy;
            }
          }
        }
      }
      if (event.is<sf::Event::MouseButtonReleased>()) {
        const auto* e = event.getIf<sf::Event::MouseButtonReleased>();
        if (e && e->button == sf::Mouse::Button::Left) {
          draggingScrollThumb = false;
          paintingLeft = false;
          lastPaintGX = lastPaintGY = -1;
        }
        if (e && e->button == sf::Mouse::Button::Right) {
          paintingRight = false;
          lastPaintGX = lastPaintGY = -1;
        }
      }

      if (event.is<sf::Event::KeyPressed>()) {
        const auto* e = event.getIf<sf::Event::KeyPressed>();
        if (e) {
          if (!enteringPath && !enteringSaveDir) {
            if (e->code == sf::Keyboard::Key::Escape) window.close();
            if (e->code == sf::Keyboard::Key::S) saveGrid();
            if (e->code == sf::Keyboard::Key::N) {
              for (auto& row : grid) for (auto& cell : row) cell = TileCR{0, 0};
              showInfo("New empty grid");
            }
            if (e->code == sf::Keyboard::Key::L) {
              enteringPath = true;
              enteringSaveDir = false;
              pathBuffer = tilesetPath;
              showInfo("Type tileset path and press Enter");
            }
            if (e->code == sf::Keyboard::Key::O) {
              loadGridFromFile(ASSETS_MAPS);
            }
            // Optional keyboard scroll helpers
            if (e->code == sf::Keyboard::Key::PageDown) { paletteScrollY += 0.9f * static_cast<float>(winH); clampAndApplyPaletteScroll(); }
            if (e->code == sf::Keyboard::Key::PageUp)   { paletteScrollY -= 0.9f * static_cast<float>(winH); clampAndApplyPaletteScroll(); }
            if (e->code == sf::Keyboard::Key::Home)     { paletteScrollY = 0.f; clampAndApplyPaletteScroll(); }
            if (e->code == sf::Keyboard::Key::End)      { paletteScrollY = 1e9f; clampAndApplyPaletteScroll(); }
          } else {
            if (e->code == sf::Keyboard::Key::Enter) {
              if (enteringPath) {
                if (tileset.loadFromFile(pathBuffer, tilePx)) {
                  tilesetPath = pathBuffer;
                  showInfo("Loaded tileset: " + tilesetPath);
                } else {
                  showInfo("Failed tileset: " + pathBuffer);
                }
                enteringPath = false;
              } else if (enteringSaveDir) {
                enteringSaveDir = false;
                showInfo(std::string("Save folder set: ") + saveDirPath);
              }
            }
            if (e->code == sf::Keyboard::Key::Escape) {
              enteringPath = false;
              enteringSaveDir = false;
            }
          }
        }
      }

      if ((enteringPath || enteringSaveDir) && event.is<sf::Event::TextEntered>()) {
        const auto* e = event.getIf<sf::Event::TextEntered>();
        if (e) {
          if (e->unicode == 8) { // backspace
            if (enteringPath) { if (!pathBuffer.empty()) pathBuffer.pop_back(); }
            else if (enteringSaveDir) { if (!saveDirPath.empty()) saveDirPath.pop_back(); }
          } else if (e->unicode >= 32 && e->unicode < 127) {
            if (enteringPath) pathBuffer += static_cast<char>(e->unicode);
            else if (enteringSaveDir) saveDirPath += static_cast<char>(e->unicode);
          }
        }
      }

      if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* e = event.getIf<sf::Event::MouseButtonPressed>();
        if (!e) continue;
        // Use event-provided position to avoid HiDPI scaling issues
        sf::Vector2i mp(e->position.x, e->position.y);
        // Map to palette coordinate space for accurate hit testing while scrolled
        sf::Vector2f mpPaletteF = window.mapPixelToCoords(mp, paletteView);
        sf::Vector2i mpPalette(static_cast<int>(mpPaletteF.x), static_cast<int>(mpPaletteF.y));

        // Save controls hit-tests (take precedence over palette)
        if (mp.x >= 0 && mp.x < paletteWidth) {
          // Input box
          sf::IntRect inputRect({12, saveInputY}, {paletteWidth - 24, 26});
          sf::IntRect saveBtnRect({12, saveButtonsY}, {100, 28});
          sf::IntRect browseBtnRect({12 + 110, saveButtonsY}, {100, 28});
          if (inputRect.contains(mpPalette)) {
            enteringSaveDir = true;
            enteringPath = false;
            continue;
          }
          if (saveBtnRect.contains(mpPalette) && e->button == sf::Mouse::Button::Left) {
            saveGrid();
            continue;
          }
          if (browseBtnRect.contains(mpPalette) && e->button == sf::Mouse::Button::Left) {
#ifdef __APPLE__
            if (auto chosen = macChooseFolder()) {
              saveDirPath = *chosen;
              showInfo(std::string("Save folder set: ") + saveDirPath);
            } else {
              showInfo("Folder selection canceled");
            }
#else
            showInfo("Folder picker not supported on this platform");
#endif
            continue;
          }
        }

        // Scrollbar hit-tests (track and thumb) when clicking in palette area
        if (mp.x >= 0 && mp.x < paletteWidth && e->button == sf::Mouse::Button::Left) {
          if (auto rects = getScrollbarRects()) {
            const auto& track = rects->first;
            const auto& thumb = rects->second;
            // Enlarge hit area horizontally for easier grabbing
            sf::FloatRect hitTrack = track;
            hitTrack.position.x -= 4.f;
            hitTrack.size.x += 8.f;
            if (hitTrack.contains(mpPaletteF)) {
              if (thumb.contains(mpPaletteF)) {
                // Start dragging
                draggingScrollThumb = true;
                dragOffsetY = mpPaletteF.y - thumb.position.y; // preserve grab offset inside the thumb
                continue; // consume
              } else {
                // Jump to clicked position on track
                float trackY = track.position.y;
                float trackH = track.size.y;
                float thumbH = thumb.size.y;
                float contentH = computePaletteContentHeight();
                float maxScroll = contentH - static_cast<float>(winH);
                float t = (trackH - thumbH) > 0.f ? (mpPaletteF.y - trackY - thumbH * 0.5f) / (trackH - thumbH) : 0.f;
                if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;
                paletteScrollY = t * std::max(0.f, maxScroll);
                clampAndApplyPaletteScroll();
                continue; // consume
              }
            }
          }
        }

        // Click in palette area to select tile
        if (mp.x >= 0 && mp.x < paletteWidth) {
          if (!tileset.loaded) continue;
          const int colsPerRow = std::max(1, (paletteWidth - padding - x0) / (cell + padding));

          int localX = mpPalette.x - x0;
          int localY = mpPalette.y - y0;
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
              paintCell(gx, gy, true);
              paintingLeft = true;
              lastPaintGX = gx; lastPaintGY = gy;
            } else if (e->button == sf::Mouse::Button::Right) {
              paintCell(gx, gy, false);
              paintingRight = true;
              lastPaintGX = gx; lastPaintGY = gy;
            }
          }
        }
      }
    }

    window.clear(sf::Color(18, 18, 24));

    // Draw the left UI palette in its own clipped view
    window.setView(paletteView);

    // Update background height to cover full content when scrolled
    {
      float contentH = computePaletteContentHeight();
      paletteBG.setSize(sf::Vector2f(static_cast<float>(paletteWidth), std::max(static_cast<float>(winH), contentH)));
    }

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
      // Ensure help stays within palette
      help.setString(ellipsizeEnd("L: load tileset  •  S: save  •  N: new  •  O: load level1", 14u, static_cast<float>(paletteWidth - 32)));
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
        // Fit input text into the box (prefer tail since paths are right-important)
        path.setString(ellipsizeStart(pathBuffer, 16u, static_cast<float>((paletteWidth - 24) - 12)));
        window.draw(path);
      } else {
        sf::Text path(font);
        path.setCharacterSize(12);
        path.setFillColor(sf::Color(160, 160, 180));
        path.setPosition(sf::Vector2f(16.f, 62.f));
        // Fit tileset path within palette
        path.setString(ellipsizeStart(std::string("Tileset: ") + tilesetPath, 12u, static_cast<float>(paletteWidth - 32)));
        window.draw(path);

        // Save folder controls
        sf::Text saveLabel(font);
        saveLabel.setCharacterSize(14);
        saveLabel.setFillColor(sf::Color(180, 180, 200));
        saveLabel.setPosition(sf::Vector2f(16.f, static_cast<float>(saveLabelY)));
        saveLabel.setString("Save folder:");
        window.draw(saveLabel);

        sf::RectangleShape saveBox(sf::Vector2f(static_cast<float>(paletteWidth - 24), 26.f));
        saveBox.setFillColor(sf::Color(50, 50, 60));
        saveBox.setOutlineThickness(1);
        saveBox.setOutlineColor(sf::Color(90, 90, 110));
        saveBox.setPosition(sf::Vector2f(12.f, static_cast<float>(saveInputY)));
        window.draw(saveBox);

        sf::Text savePathText(font);
        savePathText.setCharacterSize(14);
        savePathText.setFillColor(sf::Color::White);
        savePathText.setPosition(sf::Vector2f(18.f, static_cast<float>(saveInputY + 2)));
        // Fit displayed save path into the input box
        savePathText.setString(ellipsizeStart(saveDirPath, 14u, static_cast<float>((paletteWidth - 24) - 12)));
        window.draw(savePathText);

        // Save button
        sf::RectangleShape saveBtn(sf::Vector2f(100.f, 28.f));
        saveBtn.setFillColor(sf::Color(70, 120, 90));
        saveBtn.setOutlineThickness(1);
        saveBtn.setOutlineColor(sf::Color(90, 140, 110));
        saveBtn.setPosition(sf::Vector2f(12.f, static_cast<float>(saveButtonsY)));
        window.draw(saveBtn);

        sf::Text saveBtnText(font);
        saveBtnText.setCharacterSize(16);
        saveBtnText.setFillColor(sf::Color(240, 255, 240));
        saveBtnText.setPosition(sf::Vector2f(12.f + 20.f, static_cast<float>(saveButtonsY) + 4.f));
        saveBtnText.setString("Save");
        window.draw(saveBtnText);

        // Browse button
        sf::RectangleShape browseBtn(sf::Vector2f(100.f, 28.f));
        browseBtn.setFillColor(sf::Color(70, 90, 120));
        browseBtn.setOutlineThickness(1);
        browseBtn.setOutlineColor(sf::Color(90, 110, 140));
        browseBtn.setPosition(sf::Vector2f(12.f + 110.f, static_cast<float>(saveButtonsY)));
        window.draw(browseBtn);

        sf::Text browseBtnText(font);
        browseBtnText.setCharacterSize(16);
        browseBtnText.setFillColor(sf::Color(235, 240, 255));
        browseBtnText.setPosition(sf::Vector2f(12.f + 110.f + 14.f, static_cast<float>(saveButtonsY) + 4.f));
        browseBtnText.setString("Browse");
        window.draw(browseBtnText);
      }
    }

    // Draw palette thumbnails (still clipped by palette view)
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

    // Simple vertical scrollbar (right edge of the palette)
    {
      float contentH = computePaletteContentHeight();
      if (contentH > static_cast<float>(winH)) {
        float trackW = 8.f;
        float trackX = static_cast<float>(paletteWidth) - (trackW + 2.f);
        float trackH = static_cast<float>(winH) - 8.f;
        float viewTop = paletteView.getCenter().y - paletteView.getSize().y / 2.f;
        // Thumb size proportional to visible fraction
        float thumbH = std::max(24.f, trackH * (static_cast<float>(winH) / contentH));
        float maxScroll = contentH - static_cast<float>(winH);
        float t = (maxScroll > 0.f) ? (paletteScrollY / maxScroll) : 0.f;
        float thumbY = (viewTop + 4.f) + t * (trackH - thumbH);

        // Track (subtle)
        sf::RectangleShape track(sf::Vector2f(trackW, trackH));
        track.setPosition(sf::Vector2f(trackX, viewTop + 4.f));
        track.setFillColor(sf::Color(40, 40, 50, 120));
        window.draw(track);
        // Thumb
        sf::RectangleShape thumb(sf::Vector2f(trackW, thumbH));
        thumb.setPosition(sf::Vector2f(trackX, thumbY));
        thumb.setFillColor(sf::Color(90, 110, 140, 200));
        window.draw(thumb);
      }
    }

    // Switch back to default (full-window) view for the grid
    window.setView(defaultView);

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

    // Draw info message inside the palette view so it never overlaps grid
    if (!infoMessage.empty()) {
      window.setView(paletteView);
      float s = infoClock.getElapsedTime().asSeconds();
      if (s < 3.0f) {
        sf::Text msg(font);
        msg.setCharacterSize(16);
        msg.setFillColor(sf::Color(240, 240, 250));
        // Pin to the visible bottom of the palette view
        float viewBottom = paletteView.getCenter().y + paletteView.getSize().y / 2.f;
        msg.setPosition(sf::Vector2f(16.f, viewBottom - 32.f));
        // Keep within left palette to avoid overlapping the grid
        msg.setString(ellipsizeEnd(infoMessage, 16u, static_cast<float>(paletteWidth - 32)));
        window.draw(msg);
      }
      // Restore default view for anything drawn afterwards (none here, but keep tidy)
      window.setView(defaultView);
    }

    window.display();
  }

  return 0;
}

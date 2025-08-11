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
#ifdef _WIN32
  #ifndef NOMINMAX
  #define NOMINMAX
  #endif
  #include <windows.h>
  #include <commdlg.h>
  #include <shlobj.h>
  #include <shobjidl.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "comdlg32.lib")
    #pragma comment(lib, "shell32.lib")
    #pragma comment(lib, "ole32.lib")
  #endif
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

// Try to locate a readable, widely available UI font on each platform
static std::string findDefaultUIFont() {
  std::vector<std::string> candidates;
#ifdef _WIN32
  candidates = {
    "C:/Windows/Fonts/segoeui.ttf",
    "C:/Windows/Fonts/arial.ttf",
    "C:/Windows/Fonts/tahoma.ttf",
    "C:/Windows/Fonts/verdana.ttf"
  };
#elif __APPLE__
  candidates = {
    "/System/Library/Fonts/SFNS.ttf",
    "/System/Library/Fonts/SFNS.ttf",
    "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
    "/System/Library/Fonts/Supplemental/Helvetica.ttc"
  };
#else
  candidates = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
  };
#endif
  for (const auto& p : candidates) {
    std::error_code ec;
    if (std::filesystem::exists(p, ec)) return p;
  }
  return {};
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
  int tileW{32};
  int tileH{32};
  int cols{0};
  int rows{0};
  bool loaded{false}; // true when texture and grid are valid

  bool loadTexture(const std::string& p) {
    path = p;
    loaded = false;
    if (!texture.loadFromFile(path)) {
      std::cerr << "Failed to load tileset: " << path << std::endl;
      return false;
    }
    image = texture.copyToImage();
    return true;
  }

  bool configureGrid(int w, int h, int c, int r) {
    if (texture.getSize().x == 0 || texture.getSize().y == 0) return false;
    tileW = std::max(1, w);
    tileH = std::max(1, h);
    if (c > 0 && r > 0) {
      cols = c;
      rows = r;
    } else {
      cols = static_cast<int>(texture.getSize().x) / tileW;
      rows = static_cast<int>(texture.getSize().y) / tileH;
    }
    loaded = cols > 0 && rows > 0;
    if (!loaded) {
      std::cerr << "Tileset grid invalid (cols/rows <= 0)." << std::endl;
    }
    return loaded;
  }
};

#ifdef _WIN32
// Try to use modern IFileDialog for picking files/folders; fallback to legacy APIs if needed
static std::optional<std::string> winPickImageFile() {
  // Preferred: IFileOpenDialog with filters
  IFileOpenDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
  if (SUCCEEDED(hr) && pDlg) {
    // Set filters
    COMDLG_FILTERSPEC filters[] = {
      { L"Image Files", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif" },
      { L"All Files", L"*.*" }
    };
    pDlg->SetFileTypes(static_cast<UINT>(std::size(filters)), filters);
    pDlg->SetFileTypeIndex(1);
    pDlg->SetOptions(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);
    hr = pDlg->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem* pItem = nullptr;
      if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem) {
        PWSTR psz = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &psz)) && psz) {
          std::wstring ws(psz);
          CoTaskMemFree(psz);
          pItem->Release();
          pDlg->Release();
          // convert to UTF-8
          int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
          std::string out(len > 1 ? len - 1 : 0, '\0');
          if (len > 1) WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), len - 1, nullptr, nullptr);
          return out;
        }
        if (pItem) pItem->Release();
      }
    }
    pDlg->Release();
  }
  // Fallback: GetOpenFileNameA
  const char* filter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All Files\0*.*\0\0";
  char file[MAX_PATH] = "";
  OPENFILENAMEA ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFilter = filter;
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
  if (GetOpenFileNameA(&ofn)) return std::string(ofn.lpstrFile);
  return std::nullopt;
}

static std::optional<std::string> winPickFolder(const wchar_t* title = L"Select save folder") {
  // Preferred: IFileOpenDialog with FOS_PICKFOLDERS
  IFileOpenDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
  if (SUCCEEDED(hr) && pDlg) {
    DWORD opts = 0;
    pDlg->GetOptions(&opts);
    pDlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
    if (title) pDlg->SetTitle(title);
    hr = pDlg->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem* pItem = nullptr;
      if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem) {
        PWSTR psz = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &psz)) && psz) {
          std::wstring ws(psz);
          CoTaskMemFree(psz);
          pItem->Release();
          pDlg->Release();
          int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
          std::string out(len > 1 ? len - 1 : 0, '\0');
          if (len > 1) WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), len - 1, nullptr, nullptr);
          return out;
        }
        if (pItem) pItem->Release();
      }
    }
    pDlg->Release();
  }
  // Fallback: SHBrowseForFolderA
  BROWSEINFOA bi{};
  bi.hwndOwner = nullptr;
  bi.pidlRoot = nullptr;
  std::string titleA;
  if (title) {
    int len = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr, nullptr);
    titleA.resize(len > 1 ? len - 1 : 0);
    if (len > 1) WideCharToMultiByte(CP_UTF8, 0, title, -1, titleA.data(), len - 1, nullptr, nullptr);
    bi.lpszTitle = titleA.c_str();
  }
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
  LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
  if (!pidl) return std::nullopt;
  char path[MAX_PATH] = "";
  BOOL ok = SHGetPathFromIDListA(pidl, path);
  LPMALLOC pMalloc = nullptr;
  if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc) {
    pMalloc->Free(pidl);
    pMalloc->Release();
  }
  if (ok && path[0] != '\0') return std::string(path);
  return std::nullopt;
}
#endif

int main() {
#ifdef _WIN32
  // Initialize COM for modern file/folder pickers
  HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  (void)comInit; // ignore failures; we have fallbacks
#endif
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
  // Helper to keep palette viewport within window bounds and avoid overlap
  auto setPaletteViewport = [&]() {
    float vw = static_cast<float>(winW);
    float fraction = vw > 0.f ? static_cast<float>(paletteWidth) / vw : 1.f;
    if (fraction < 0.f) fraction = 0.f;
    if (fraction > 1.f) fraction = 1.f;
    paletteView.setViewport(sf::FloatRect({0.f, 0.f}, {fraction, 1.f}));
  };
  setPaletteViewport();

  // Load a default system UI font to avoid missing glyphs/kerning issues
  sf::Font font;
  bool fontOK = false;
  {
    std::string sysFont = findDefaultUIFont();
    if (!sysFont.empty()) {
      fontOK = font.openFromFile(sysFont);
      if (!fontOK) std::cerr << "Failed to open system font: " << sysFont << std::endl;
    }
    if (!fontOK) {
      // Fallback to bundled font if system fonts not found
      std::string assetFont = findAssetPath(ASSETS_FONT_ARCADECLASSIC);
      fontOK = font.openFromFile(assetFont);
      if (!fontOK) std::cerr << "Failed to load fallback font: " << assetFont << std::endl;
    }
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
  // Default config buffers
  int defaultW = tilePx;
  int defaultH = tilePx;

  // Grid data (col,row for each cell)
  std::vector<std::vector<TileCR>> grid(gridRows, std::vector<TileCR>(gridCols, TileCR{0, 0}));

  // Selected tile in palette (col,row)
  TileCR selected{0, 0};

  // Text input state for loading tileset
  bool enteringPath = false;
  std::string pathBuffer = tilesetPath;

  // Inputs for tileset config (W, H)
  bool enteringTileW = false, enteringTileH = false, enteringRows = false, enteringCols = false;
  std::string tileWBuf = std::to_string(defaultW);
  std::string tileHBuf = std::to_string(defaultH);
  std::string rowsBuf = ""; // removed from UI; kept for minimal change
  std::string colsBuf = ""; // removed from UI; kept for minimal change

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

  // Helper to apply current buffers into tileset grid
  auto applyTilesetConfig = [&]() {
    if (tileset.texture.getSize().x == 0) { showInfo("Load a tileset path first (press L)"); return; }
    int w = defaultW, h = defaultH, r = 0, c = 0;
    try { if (!tileWBuf.empty()) w = std::max(1, std::stoi(tileWBuf)); } catch (...) {}
    try { if (!tileHBuf.empty()) h = std::max(1, std::stoi(tileHBuf)); } catch (...) {}
    // Rows/Cols no longer set by UI; let tileset compute automatically
    r = 0; c = 0;
    if (tileset.configureGrid(w, h, c, r)) {
      selected = TileCR{0, 0};
      showInfo("Tileset config applied");
    } else {
      showInfo("Invalid tileset configuration");
    }
  };

  // Save current grid to file in chosen folder
  auto saveGrid = [&]() {
    std::filesystem::path mapsDir = saveDirPath.empty() ? getUserMapsDir() : std::filesystem::path(saveDirPath);
    std::error_code ec;
    if (!std::filesystem::exists(mapsDir)) {
      std::filesystem::create_directories(mapsDir, ec);
      if (ec) { showInfo(std::string("Failed to create maps dir: ") + mapsDir.string()); return; }
    } else if (!std::filesystem::is_directory(mapsDir)) {
      showInfo(std::string("Not a directory: ") + mapsDir.string());
      return;
    }
    std::string fileName = timestampName();
    std::filesystem::path outPath = mapsDir / fileName;
    std::ofstream out(outPath);
    if (!out.is_open()) { showInfo("Failed to save: " + outPath.string()); return; }
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

  // Load grid from a .grid file
  auto loadGridFromFile = [&](const std::string& filePath) {
    std::string resolved = findAssetPath(filePath);
    std::ifstream in(resolved);
    if (!in.is_open()) { showInfo("Failed to open: " + resolved); return; }
    for (int y = 0; y < gridRows; ++y) {
      for (int x = 0; x < gridCols; ++x) {
        int c, r; if (!(in >> c >> r)) { showInfo("Invalid grid file format"); return; }
        grid[y][x] = TileCR{c, r};
      }
    }
    showInfo("Loaded grid from: " + resolved);
  };

  // Load default map if available
  {
    std::string defaultMap = findAssetPath(ASSETS_MAPS);
    if (!defaultMap.empty() && std::filesystem::exists(defaultMap)) {
      loadGridFromFile(defaultMap);
    }
  }

  // Palette draw origin and layout (shared by draw and hit-test)
  const float thumbScale = 2.0f;
  const int cell = static_cast<int>(tilePx * thumbScale);
  const int padding = 6;
  const int x0 = 8;
  // Layout for tileset config and save controls
  const int cfgLabelY = 128;
  const int cfgInputsY = 148;
  const int cfgButtonsY = 178;
  const int saveLabelY = 214;
  const int saveInputY = 234;
  const int saveButtonsY = 266;
  // Shift tileset thumbnails down to make space for config + save controls
  const int y0 = 302;

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

  // Simple helpers for drawing
  sf::RectangleShape paletteBG(sf::Vector2f(static_cast<float>(paletteWidth), static_cast<float>(winH)));
  paletteBG.setFillColor(sf::Color(30, 30, 40));

  sf::RectangleShape gridBG(sf::Vector2f(static_cast<float>(gridPxW), static_cast<float>(gridPxH)));
  gridBG.setFillColor(sf::Color(10, 15, 20));
  gridBG.setPosition(gridOrigin);
  gridBG.setOutlineThickness(1);
  gridBG.setOutlineColor(sf::Color(60, 60, 70));

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
        setPaletteViewport();
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
          if (!enteringPath && !enteringSaveDir && !enteringTileW && !enteringTileH && !enteringRows && !enteringCols) {
            if (e->code == sf::Keyboard::Key::Escape) window.close();
            if (e->code == sf::Keyboard::Key::S) saveGrid();
            if (e->code == sf::Keyboard::Key::N) {
              for (auto& row : grid) for (auto& cell : row) cell = TileCR{0, 0};
              showInfo("New empty grid");
            }
            if (e->code == sf::Keyboard::Key::L) {
              enteringPath = true;
              enteringSaveDir = false;
              enteringTileW = enteringTileH = enteringRows = enteringCols = false;
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
                if (tileset.loadTexture(pathBuffer)) {
                  tilesetPath = pathBuffer;
                  // Keep current buffers for grid config, or defaults if empty
                  applyTilesetConfig();
                  showInfo("Loaded tileset: " + tilesetPath);
                } else {
                  showInfo("Failed tileset: " + pathBuffer);
                }
                enteringPath = false;
              } else if (enteringSaveDir) {
                enteringSaveDir = false;
                showInfo(std::string("Save folder set: ") + saveDirPath);
              } else if (enteringTileW || enteringTileH) {
                enteringTileW = enteringTileH = enteringRows = enteringCols = false;
                applyTilesetConfig();
                clampAndApplyPaletteScroll();
              }
            }
            if (e->code == sf::Keyboard::Key::Escape) {
              enteringPath = false;
              enteringSaveDir = false;
              enteringTileW = enteringTileH = enteringRows = enteringCols = false;
            }
          }
        }
      }

      if ((enteringPath || enteringSaveDir || enteringTileW || enteringTileH || enteringRows || enteringCols) && event.is<sf::Event::TextEntered>()) {
        const auto* e = event.getIf<sf::Event::TextEntered>();
        if (e) {
          if (e->unicode == 8) { // backspace
            if (enteringPath) { if (!pathBuffer.empty()) pathBuffer.pop_back(); }
            else if (enteringSaveDir) { if (!saveDirPath.empty()) saveDirPath.pop_back(); }
            else if (enteringTileW) { if (!tileWBuf.empty()) tileWBuf.pop_back(); }
            else if (enteringTileH) { if (!tileHBuf.empty()) tileHBuf.pop_back(); }
            else if (enteringRows) { if (!rowsBuf.empty()) rowsBuf.pop_back(); }
            else if (enteringCols) { if (!colsBuf.empty()) colsBuf.pop_back(); }
          } else if (enteringPath) {
            if (e->unicode >= 32 && e->unicode < 127) pathBuffer += static_cast<char>(e->unicode);
          } else {
            // Numeric-only for config
            if (e->unicode >= '0' && e->unicode <= '9') {
              if (enteringTileW) tileWBuf += static_cast<char>(e->unicode);
              else if (enteringTileH) tileHBuf += static_cast<char>(e->unicode);
            }
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

  // Tileset path input and Load button hit-tests
        if (mp.x >= 0 && mp.x < paletteWidth) {
          int pathInputW = paletteWidth - 24 - 100 - 6; // input + gap + load button
          sf::IntRect pathRect({12, 62}, {pathInputW, 26});
          sf::IntRect loadRect({12 + pathInputW + 6, 62}, {100, 26});
          if (pathRect.contains(mpPalette)) {
            enteringPath = true;
            enteringSaveDir = enteringTileW = enteringTileH = enteringRows = enteringCols = false;
            continue;
          }
          if (loadRect.contains(mpPalette) && e->button == sf::Mouse::Button::Left) {
#ifdef _WIN32
            // Open dialog to pick image and load immediately
            if (auto sel = winPickImageFile()) {
              pathBuffer = *sel;
              if (tileset.loadTexture(pathBuffer)) {
                tilesetPath = pathBuffer;
                applyTilesetConfig();
                clampAndApplyPaletteScroll();
                showInfo(std::string("Loaded tileset: ") + tilesetPath);
              } else {
                showInfo(std::string("Failed tileset: ") + pathBuffer);
              }
            } else {
              showInfo("Load canceled");
            }
#else
            // Fallback: use typed path
            if (tileset.loadTexture(pathBuffer)) {
              tilesetPath = pathBuffer;
              applyTilesetConfig();
              clampAndApplyPaletteScroll();
              showInfo(std::string("Loaded tileset: ") + tilesetPath);
            } else {
              showInfo(std::string("Failed tileset: ") + pathBuffer);
            }
#endif
            enteringPath = false;
            continue;
          }
        }

    // Tileset config controls (take top precedence)
        if (mp.x >= 0 && mp.x < paletteWidth) {
          // Input rects
          sf::IntRect wRect({12, cfgInputsY}, {52, 26});
          sf::IntRect hRect({12 + 60, cfgInputsY}, {52, 26});
          // Move Apply below inputs to fit within palette width
          sf::IntRect applyRect({12, cfgButtonsY}, {100, 26});
          if (wRect.contains(mpPalette)) {
      enteringTileW = true; enteringTileH = enteringRows = enteringCols = enteringSaveDir = enteringPath = false; continue;
          }
          if (hRect.contains(mpPalette)) {
      enteringTileH = true; enteringTileW = enteringRows = enteringCols = enteringSaveDir = enteringPath = false; continue;
          }
          if (applyRect.contains(mpPalette) && e->button == sf::Mouse::Button::Left) {
            enteringTileW = enteringTileH = enteringRows = enteringCols = false;
            applyTilesetConfig();
            clampAndApplyPaletteScroll();
            continue;
          }
        }

        // Save controls hit-tests
        if (mp.x >= 0 && mp.x < paletteWidth) {
          sf::IntRect inputRect({12, saveInputY}, {paletteWidth - 24, 26});
          sf::IntRect saveBtnRect({12, saveButtonsY}, {100, 28});
          sf::IntRect browseBtnRect({12 + 110, saveButtonsY}, {100, 28});
          if (inputRect.contains(mpPalette)) {
            enteringSaveDir = true;
            enteringPath = enteringTileW = enteringTileH = enteringRows = enteringCols = false;
            continue;
          }
          if (saveBtnRect.contains(mpPalette) && e->button == sf::Mouse::Button::Left) { saveGrid(); continue; }
          if (browseBtnRect.contains(mpPalette) && e->button == sf::Mouse::Button::Left) {
#if defined(_WIN32)
            if (auto chosen = winPickFolder(L"Select folder to save maps")) { saveDirPath = *chosen; showInfo(std::string("Save folder set: ") + saveDirPath); }
            else { showInfo("Folder selection canceled"); }
#elif defined(__APPLE__)
            if (auto chosen = macChooseFolder()) { saveDirPath = *chosen; showInfo(std::string("Save folder set: ") + saveDirPath); }
            else { showInfo("Folder selection canceled"); }
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

  // Tileset path input
    {
      int pathInputW = paletteWidth - 24 - 100 - 6; // input + gap + Load button
      // Input box
      sf::RectangleShape box(sf::Vector2f(static_cast<float>(pathInputW), 26.f));
      box.setFillColor(sf::Color(50, 50, 60));
      box.setOutlineThickness(1);
      box.setOutlineColor(enteringPath ? sf::Color(120, 160, 220) : sf::Color(90, 90, 110));
      box.setPosition(sf::Vector2f(12.f, 62.f));
      window.draw(box);

      sf::Text path(font);
      path.setCharacterSize(16);
      path.setFillColor(sf::Color::White);
      path.setPosition(sf::Vector2f(18.f, 64.f));
      path.setString(ellipsizeStart(pathBuffer, 16u, static_cast<float>(pathInputW - 12)));
      window.draw(path);

      // Load button
      sf::RectangleShape loadBtn(sf::Vector2f(100.f, 26.f));
      loadBtn.setFillColor(sf::Color(85, 120, 160));
      loadBtn.setOutlineThickness(1);
      loadBtn.setOutlineColor(sf::Color(90, 110, 140));
      loadBtn.setPosition(sf::Vector2f(12.f + static_cast<float>(pathInputW) + 6.f, 62.f));
      window.draw(loadBtn);
      sf::Text loadTxt(font);
      loadTxt.setCharacterSize(16);
      loadTxt.setFillColor(sf::Color(235, 240, 255));
      loadTxt.setPosition(sf::Vector2f(12.f + static_cast<float>(pathInputW) + 6.f + 24.f, 66.f));
      loadTxt.setString("Load");
      window.draw(loadTxt);

  // (Top Browse button removed; Load already opens a file dialog)

  // Tileset config label
      sf::Text cfgLabel(font);
      cfgLabel.setCharacterSize(14);
      cfgLabel.setFillColor(sf::Color(180, 180, 200));
      cfgLabel.setPosition(sf::Vector2f(16.f, static_cast<float>(cfgLabelY)));
  cfgLabel.setString("Config (W,H):");
      window.draw(cfgLabel);

      // Inputs W, H, Rows, Cols and Apply button
      auto drawInput = [&](sf::Vector2f pos, const std::string& text, bool focused) {
        sf::RectangleShape ibox(sf::Vector2f(52.f, 26.f));
        ibox.setFillColor(sf::Color(50, 50, 60));
        ibox.setOutlineThickness(1);
        ibox.setOutlineColor(focused ? sf::Color(120, 160, 220) : sf::Color(90, 90, 110));
        ibox.setPosition(pos);
        window.draw(ibox);
        sf::Text t(font);
        t.setCharacterSize(16);
        t.setFillColor(sf::Color::White);
        t.setPosition(pos + sf::Vector2f(6.f, 4.f));
        t.setString(text.empty() ? "" : text);
        window.draw(t);
      };
  drawInput(sf::Vector2f(12.f, static_cast<float>(cfgInputsY)), tileWBuf, enteringTileW);
  drawInput(sf::Vector2f(72.f, static_cast<float>(cfgInputsY)), tileHBuf, enteringTileH);
      // Apply button (moved below inputs to avoid overlap and stay within palette)
      sf::RectangleShape applyBtn(sf::Vector2f(100.f, 26.f));
      applyBtn.setFillColor(sf::Color(85, 120, 160));
      applyBtn.setOutlineThickness(1);
      applyBtn.setOutlineColor(sf::Color(90, 110, 140));
      applyBtn.setPosition(sf::Vector2f(12.f, static_cast<float>(cfgButtonsY)));
      window.draw(applyBtn);
      sf::Text applyTxt(font);
      applyTxt.setCharacterSize(16);
      applyTxt.setFillColor(sf::Color(235, 240, 255));
      applyTxt.setPosition(sf::Vector2f(12.f + 12.f, static_cast<float>(cfgButtonsY) + 4.f));
      applyTxt.setString("Apply");
      window.draw(applyTxt);

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

          sf::Sprite spr(tileset.texture, sf::IntRect({c * tileset.tileW, r * tileset.tileH}, {tileset.tileW, tileset.tileH}));
          // Fit uniformly into cell and center
          float scale = std::min(static_cast<float>(cell) / static_cast<float>(tileset.tileW), static_cast<float>(cell) / static_cast<float>(tileset.tileH));
          float offX = (cell - tileset.tileW * scale) * 0.5f;
          float offY = (cell - tileset.tileH * scale) * 0.5f;
          spr.setPosition(sf::Vector2f(static_cast<float>(px) + offX, static_cast<float>(py) + offY));
          spr.setScale(sf::Vector2f(scale, scale));
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
          sf::Sprite spr(tileset.texture, sf::IntRect({t.col * tileset.tileW, t.row * tileset.tileH}, {tileset.tileW, tileset.tileH}));
          // Scale to fit grid cell size (may be non-uniform if tileW != tileH)
          float scaleX = (tilePx * tileScale) / static_cast<float>(tileset.tileW);
          float scaleY = (tilePx * tileScale) / static_cast<float>(tileset.tileH);
          spr.setScale(sf::Vector2f(scaleX, scaleY));
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

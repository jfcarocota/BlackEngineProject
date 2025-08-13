#include <limits.h>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <cwchar>
#include <fstream>
#include <functional>
#include <gsl/narrow>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

// Define to enable native Win32 file dialogs. Requires Windows SDK
// headers/libs. #define MAPEDITOR_ENABLE_WIN32_DIALOGS 1

// Fallback for compilers without __has_include
#ifndef __has_include
#define __has_include(x) 0
#endif

// Filesystem compatibility shim: prefer <filesystem>, fallback to
// <experimental/filesystem>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "No <filesystem> support available"
#endif

// Optional compatibility: prefer <optional>, fallback to experimental
#if __has_include(<optional>)
#include <optional>
#elif __has_include(<experimental/optional>)
#include <experimental/optional>
namespace std {
using experimental::nullopt;
using experimental::nullopt_t;
using experimental::optional;
}  // namespace std
#else
#error "No <optional> support available"
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <windows.h>
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

static inline bool operator==(const TileCR& a, const TileCR& b) noexcept {
  return a.col == b.col && a.row == b.row;
}
static inline bool operator!=(const TileCR& a, const TileCR& b) noexcept {
  return !(a == b);
}

// Resolve asset paths when running from different working directories or a
// macOS .app bundle
static std::string getExecutableDir() {
  std::string dir;
#ifdef __APPLE__
  char buf[PATH_MAX];
  uint32_t size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0) {
    std::error_code ec;
    auto p = fs::weakly_canonical(fs::path(buf), ec);
    if (!ec) dir = p.parent_path().string();
  }
#else
  // On non-Apple systems you could add platform-specific resolution if needed
#endif
  return dir;
}

static std::string findAssetPath(const std::string& relative) {
  // 1) As provided (relative to CWD or absolute)
  fs::path p(relative);
  if (fs::exists(p)) return p.string();

  // 2) Try common fallbacks relative to CWD
  std::vector<std::string> relPrefixes = {"./", "../", "../../", "../../../"};
  for (const auto& pref : relPrefixes) {
    fs::path cand = fs::path(pref) / relative;
    if (fs::exists(cand)) return cand.string();
  }

  // 3) Try relative to the executable location
  std::string exeDir = getExecutableDir();
  if (!exeDir.empty()) {
    fs::path cand = fs::path(exeDir) / relative;
    if (fs::exists(cand)) return cand.string();

#ifdef __APPLE__
    // If inside a .app bundle: exeDir = <App>.app/Contents/MacOS
    auto macosDir = fs::path(exeDir);
    if (macosDir.filename() == "MacOS" &&
        macosDir.parent_path().filename() == "Contents") {
      fs::path resources = macosDir.parent_path() / "Resources" / relative;
      if (fs::exists(resources)) return resources.string();
      // Also support apps where assets are placed directly under Resources
      // without the leading folder e.g., relative = "assets/tiles.png" might
      // also exist as Resources/assets/tiles.png (already checked)
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
  candidates = {"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf",
                "C:/Windows/Fonts/tahoma.ttf", "C:/Windows/Fonts/verdana.ttf"};
#elif __APPLE__
  candidates = {"/System/Library/Fonts/SFNS.ttf",
                "/System/Library/Fonts/SFNS.ttf",
                "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
                "/System/Library/Fonts/Supplemental/Helvetica.ttc"};
#else
  candidates = {
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
      "/usr/share/fonts/truetype/freefont/FreeSans.ttf"};
#endif
  for (const auto& p : candidates) {
    std::error_code ec;
    if (fs::exists(p, ec)) return p;
  }
  return {};
}

// User-writable directory for saved maps
static fs::path getUserMapsDir() {
  fs::path home;
#ifdef _WIN32
  const char* appData = std::getenv("APPDATA");
  if (appData)
    home = appData;
  else
    home = fs::path(".");
  return home / "BlackEngineProject" / "Maps";
#else
  const char* h = std::getenv("HOME");
  if (h)
    home = h;
  else
    home = ".";
#ifdef __APPLE__
  return home / "Library" / "Application Support" / "BlackEngineProject" /
         "Maps";
#else
  return home / ".local" / "share" / "BlackEngineProject" / "Maps";
#endif
#endif
}

// macOS-only: show a folder picker using AppleScript (osascript)
#ifdef __APPLE__
static std::optional<std::string> macChooseFolder() {
  const char* cmd =
      "osascript -e 'POSIX path of (choose folder with prompt \"Select save "
      "folder\")' 2>/dev/null";
  std::string result;
  FILE* pipe = popen(cmd, "r");
  if (!pipe) return std::nullopt;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    result += buffer;
  }
  pclose(pipe);
  // trim whitespace/newlines
  auto isSpace = [](unsigned char c) { return std::isspace(c); };
  while (!result.empty() && isSpace(result.back())) result.pop_back();
  while (!result.empty() && isSpace(result.front()))
    result.erase(result.begin());
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
  std::strftime(buf, sizeof(buf), "map_%Y%m%d_%H%M%S.json", &tm);
  return std::string(buf);
}

// timestampName now produces JSON names directly

struct Tileset {
  sf::Texture texture;
  sf::Image image;  // optional for future use
  std::string path;
  int tileW{32};
  int tileH{32};
  int cols{0};
  int rows{0};
  bool loaded{false};  // true when texture and grid are valid

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
      cols = gsl::narrow_cast<int>(texture.getSize().x) / tileW;
      rows = gsl::narrow_cast<int>(texture.getSize().y) / tileH;
    }
    loaded = cols > 0 && rows > 0;
    if (!loaded) {
      std::cerr << "Tileset grid invalid (cols/rows <= 0)." << std::endl;
    }
    return loaded;
  }
};

#ifndef BEP_CHUNK_SIZE
#define BEP_CHUNK_SIZE 64
#endif

// Sparse, chunked grid for effectively infinite maps
struct ChunkCoord {
  int cx{0};
  int cy{0};
};
struct ChunkCoordHash {
  std::size_t operator()(const ChunkCoord& k) const noexcept {
    // 32-bit pair to 64-bit key, then hash
    uint64_t ux = static_cast<uint32_t>(k.cx);
    uint64_t uy = static_cast<uint32_t>(k.cy);
    uint64_t key = (ux << 32) ^ uy;
    return std::hash<uint64_t>{}(key);
  }
};
inline bool operator==(const ChunkCoord& a, const ChunkCoord& b) {
  return a.cx == b.cx && a.cy == b.cy;
}

struct Chunk {
  // Stored as dense 2D array within the chunk for fast access
  std::vector<TileCR> cells;  // size = BEP_CHUNK_SIZE * BEP_CHUNK_SIZE
  Chunk() : cells(BEP_CHUNK_SIZE * BEP_CHUNK_SIZE, TileCR{0, 0}) {}
  inline TileCR get(int lx, int ly) const {
    return cells[ly * BEP_CHUNK_SIZE + lx];
  }
  inline void set(int lx, int ly, TileCR v) {
    cells[ly * BEP_CHUNK_SIZE + lx] = v;
  }
  bool empty() const {
    for (const auto& c : cells)
      if (c.col != 0 || c.row != 0) return false;
    return true;
  }
};

static inline ChunkCoord toChunk(int gx, int gy) {
  auto divFloor = [](int a, int b) {
    int q = a / b;
    int r = a % b;
    if ((r != 0) && ((r > 0) != (b > 0))) --q;
    return q;
  };
  int cx = divFloor(gx, BEP_CHUNK_SIZE);
  int cy = divFloor(gy, BEP_CHUNK_SIZE);
  return ChunkCoord{cx, cy};
}
static inline void toLocal(int gx, int gy, int& lx, int& ly) {
  int cx =
      std::floor(static_cast<float>(gx) / static_cast<float>(BEP_CHUNK_SIZE));
  int cy =
      std::floor(static_cast<float>(gy) / static_cast<float>(BEP_CHUNK_SIZE));
  lx = gx - cx * BEP_CHUNK_SIZE;
  ly = gy - cy * BEP_CHUNK_SIZE;
  if (lx < 0) lx += BEP_CHUNK_SIZE;
  if (ly < 0) ly += BEP_CHUNK_SIZE;
}

#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
// Try to use modern IFileDialog for picking files/folders; fallback to legacy
// APIs if needed
static std::optional<std::string> winPickImageFile() {
  // Preferred: IFileOpenDialog with filters
  IFileOpenDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
  if (SUCCEEDED(hr) && pDlg) {
    // Set filters
    COMDLG_FILTERSPEC filters[] = {
        {L"Image Files", L"*.png;*.jpg;*.jpeg;*.bmp;*.gif"},
        {L"All Files", L"*.*"}};
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
          int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0,
                                        nullptr, nullptr);
          std::string out(len > 1 ? len - 1 : 0, '\0');
          if (len > 1)
            WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), len - 1,
                                nullptr, nullptr);
          return out;
        }
        if (pItem) pItem->Release();
      }
    }
    pDlg->Release();
  }
  // Fallback: GetOpenFileNameA
  const char* filter =
      "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0All Files\0*.*\0\0";
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

static std::optional<std::string> winPickFolder(
    const wchar_t* title = L"Select save folder") {
  // Preferred: IFileOpenDialog with FOS_PICKFOLDERS
  IFileOpenDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
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
          int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0,
                                        nullptr, nullptr);
          std::string out(len > 1 ? len - 1 : 0, '\0');
          if (len > 1)
            WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, out.data(), len - 1,
                                nullptr, nullptr);
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
    int len = WideCharToMultiByte(CP_UTF8, 0, title, -1, nullptr, 0, nullptr,
                                  nullptr);
    titleA.resize(len > 1 ? len - 1 : 0);
    if (len > 1)
      WideCharToMultiByte(CP_UTF8, 0, title, -1, titleA.data(), len - 1,
                          nullptr, nullptr);
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
#endif  // _WIN32 && MAPEDITOR_ENABLE_WIN32_DIALOGS

#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
// Open a .json file using modern IFileOpenDialog; fallback to GetOpenFileName
static std::optional<std::string> winPickMapFile(
    const wchar_t* title = L"Open map file") {
  IFileOpenDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
  if (SUCCEEDED(hr) && pDlg) {
    COMDLG_FILTERSPEC filters[] = {{L"JSON Files (*.json)", L"*.json"},
                                   {L"All Files (*.*)", L"*.*"}};
    pDlg->SetFileTypes(2, filters);
    if (title) pDlg->SetTitle(title);
    hr = pDlg->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem* pItem = nullptr;
      if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem) {
        PWSTR pszFilePath = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) &&
            pszFilePath) {
          size_t len = wcslen(pszFilePath);
          std::string out;
          out.resize(WideCharToMultiByte(CP_UTF8, 0, pszFilePath, (int)len,
                                         nullptr, 0, nullptr, nullptr));
          WideCharToMultiByte(CP_UTF8, 0, pszFilePath, (int)len, out.data(),
                              (int)out.size(), nullptr, nullptr);
          CoTaskMemFree(pszFilePath);
          pItem->Release();
          pDlg->Release();
          return out;
        }
        if (pszFilePath) CoTaskMemFree(pszFilePath);
        pItem->Release();
      }
      pDlg->Release();
    } else {
      pDlg->Release();
    }
  }
  // Fallback common dialog
  char file[MAX_PATH] = "";
  OPENFILENAMEA ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files\0*.*\0";
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  if (GetOpenFileNameA(&ofn)) return std::string(file);
  return std::nullopt;
}

// Save As dialog for JSON using IFileSaveDialog; fallback to GetSaveFileName
static std::optional<std::string> winSaveMapAs(
    const wchar_t* title = L"Save map as") {
  IFileSaveDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
  if (SUCCEEDED(hr) && pDlg) {
    COMDLG_FILTERSPEC filters[] = {{L"JSON Files (*.json)", L"*.json"},
                                   {L"All Files (*.*)", L"*.*"}};
    pDlg->SetFileTypes(2, filters);
    pDlg->SetDefaultExtension(L"json");
    if (title) pDlg->SetTitle(title);
    hr = pDlg->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem* pItem = nullptr;
      if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem) {
        PWSTR pszFilePath = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) &&
            pszFilePath) {
          std::wstring ws(pszFilePath);
          CoTaskMemFree(pszFilePath);
          pItem->Release();
          pDlg->Release();
          std::string out;
          out.resize(WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(),
                                         nullptr, 0, nullptr, nullptr));
          WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(),
                              out.data(), (int)out.size(), nullptr, nullptr);
          // Sugerir .json si no hay extensión
          if (out.find('.') == std::string::npos) out += ".json";
          return out;
        }
        if (pszFilePath) CoTaskMemFree(pszFilePath);
        pItem->Release();
      }
      pDlg->Release();
    } else {
      pDlg->Release();
    }
  }
  char file[MAX_PATH] = "";
  OPENFILENAMEA ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFilter = "JSON Files (*.json)\0*.json\0All Files\0*.*\0";
  ofn.lpstrDefExt = "json";
  ofn.lpstrFile = file;
  ofn.nMaxFile = MAX_PATH;
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
  if (GetSaveFileNameA(&ofn)) {
    std::string out(file);
    if (out.find('.') == std::string::npos) out += ".json";
    return out;
  }
  return std::nullopt;
}

// Save As dialog for .json using IFileSaveDialog
static std::optional<std::string> winSaveJsonAs(
    const wchar_t* title = L"Save JSON as") {
  IFileSaveDialog* pDlg = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDlg));
  if (SUCCEEDED(hr) && pDlg) {
    COMDLG_FILTERSPEC filters[] = {{L"JSON Files (*.json)", L"*.json"},
                                   {L"All Files (*.*)", L"*.*"}};
    pDlg->SetFileTypes(2, filters);
    pDlg->SetDefaultExtension(L"json");
    if (title) pDlg->SetTitle(title);
    hr = pDlg->Show(nullptr);
    if (SUCCEEDED(hr)) {
      IShellItem* pItem = nullptr;
      if (SUCCEEDED(pDlg->GetResult(&pItem)) && pItem) {
        PWSTR pszFilePath = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) &&
            pszFilePath) {
          std::wstring ws(pszFilePath);
          CoTaskMemFree(pszFilePath);
          pItem->Release();
          pDlg->Release();
          std::string out;
          out.resize(WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(),
                                         nullptr, 0, nullptr, nullptr));
          WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(),
                              out.data(), (int)out.size(), nullptr, nullptr);
          if (out.size() < 5 || out.substr(out.size() - 5) != ".json")
            out += ".json";
          return out;
        }
        if (pszFilePath) CoTaskMemFree(pszFilePath);
        pItem->Release();
      }
      pDlg->Release();
    } else {
      pDlg->Release();
    }
  }
  return std::nullopt;
}
#endif  // _WIN32 && MAPEDITOR_ENABLE_WIN32_DIALOGS

#if defined(_WIN32) && !defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
// Stubs when dialogs are disabled; callers should be behind the same macro
static std::optional<std::string> winPickImageFile() { return std::nullopt; }
static std::optional<std::string> winPickFolder(const wchar_t* = L"") {
  return std::nullopt;
}
static std::optional<std::string> winPickGridFile(const wchar_t* = L"") {
  return std::nullopt;
}
static std::optional<std::string> winSaveGridAs(const wchar_t* = L"") {
  return std::nullopt;
}
static std::optional<std::string> winSaveJsonAs(const wchar_t* = L"") {
  return std::nullopt;
}
#endif

int main() {
#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
  // Initialize COM for modern file/folder pickers
  HRESULT comInit = CoInitializeEx(
      nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  (void)comInit;  // ignore failures; we have fallbacks
#endif
  // Layout constants
  const int gridCols = GameConstants::MAP_WIDTH;
  const int gridRows = GameConstants::MAP_HEIGHT;
  const int tilePx = gsl::narrow_cast<int>(GameConstants::TILE_SIZE);
  const float tileScale = GameConstants::TILE_SCALE;

  const int paletteWidth = 280;  // left panel
  const int margin = 12;
  const int gridPxW = gsl::narrow_cast<int>(gridCols * tilePx * tileScale);
  const int gridPxH = gsl::narrow_cast<int>(gridRows * tilePx * tileScale);
  int winW = paletteWidth + margin + gridPxW + margin;
  int winH = std::max(gridPxH + margin * 2, 720);

  sf::RenderWindow window(sf::VideoMode(sf::Vector2u(winW, winH)),
                          "Tile Map Editor");
  window.setFramerateLimit(60);

  // Create views to isolate palette UI and prevent overlap into grid
  sf::View defaultView = window.getView();
  sf::View paletteView(sf::FloatRect(
      {0.f, 0.f},
      {static_cast<float>(paletteWidth), static_cast<float>(winH)}));
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
      if (!fontOK)
        std::cerr << "Failed to open system font: " << sysFont << std::endl;
    }
    if (!fontOK) {
      // Fallback to bundled font if system fonts not found
      std::string assetFont = findAssetPath(ASSETS_FONT_ARCADECLASSIC);
      fontOK = font.openFromFile(assetFont);
      if (!fontOK)
        std::cerr << "Failed to load fallback font: " << assetFont << std::endl;
    }
  }

  // Helper text fitting utilities to avoid overlapping UI into grid
  auto ellipsizeEnd = [&](const std::string& text, unsigned int charSize,
                          float maxWidth) {
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
  auto ellipsizeStart = [&](const std::string& text, unsigned int charSize,
                            float maxWidth) {
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

  // --- Layers system ---
  struct Layer {
    Tileset tileset;
    std::string tilesetPath;
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks;
    std::string name;
    bool visible{true};
  };

  std::vector<Layer> layers;
  int activeLayer = 0;
  auto ensureInitialLayer = [&]() {
    if (layers.empty()) {
      layers.emplace_back();
      layers[0].tilesetPath = findAssetPath(ASSETS_TILES);  // default
      layers[0].name = "Layer 1";
      layers[0].visible = true;
    }
  };
  ensureInitialLayer();

  // Helper: compute Y position for layer selector just below the title text
  auto layerHeaderY = [&]() -> int {
    sf::Text t(font);
    t.setCharacterSize(22);
    t.setString(std::string("Tileset (Layer: ") +
                (layers.empty() ? std::string("-") : layers[activeLayer].name) +
                ")");
    auto b = t.getLocalBounds();
    float bottom = 10.f + (b.size.y - b.position.y);  // title is drawn at y=10
    return gsl::narrow_cast<int>(bottom + 6.f);       // small gap below title
  };

  // Helper to insert a new layer safely (place before macros to avoid macro
  // name collisions)
  struct InsertNewLayerAtHelper {
    std::vector<Layer>* layersPtr;
    int operator()(int idxInsert) {
      auto& layersRef = *layersPtr;
      if (idxInsert < 0) idxInsert = 0;
      if (idxInsert > gsl::narrow_cast<int>(layersRef.size()))
        idxInsert = gsl::narrow_cast<int>(layersRef.size());
      Layer newLayer;
      newLayer.name =
          std::string("Layer ") +
          std::to_string(gsl::narrow_cast<int>(layersRef.size() + 1));
      // Copiar tileset desde la capa activa si existe para compartir
      // imagen/config
      if (!layersRef.empty()) {
        int src = std::clamp(idxInsert - 1, 0,
                             gsl::narrow_cast<int>(layersRef.size() - 1));
        newLayer.tileset = layersRef[src].tileset;
        newLayer.tilesetPath = layersRef[src].tilesetPath;
      } else {
        newLayer.tilesetPath = findAssetPath(ASSETS_TILES);
      }
      newLayer.visible = true;
      layersRef.insert(layersRef.begin() + idxInsert, std::move(newLayer));
      return idxInsert;
    }
  } insertNewLayerAt{&layers};

  // Ensure a layer has a loaded tileset; if not, copy from first loaded layer
  auto ensureLayerTileset = [&](int layerIdx) {
    if (layerIdx < 0 || layerIdx >= gsl::narrow_cast<int>(layers.size()))
      return;
    if (layers[layerIdx].tileset.loaded) return;
    for (const auto& L : layers) {
      if (L.tileset.loaded) {
        layers[layerIdx].tileset = L.tileset;
        layers[layerIdx].tilesetPath = L.tilesetPath;
        // Recompute grid based on copied dims
        int tw = layers[layerIdx].tileset.tileW;
        int th = layers[layerIdx].tileset.tileH;
        layers[layerIdx].tileset.configureGrid(tw, th,
                                               layers[layerIdx].tileset.cols,
                                               layers[layerIdx].tileset.rows);
        break;
      }
    }
  };

  // Propagate a tileset to all layers so they share the same image/config
  auto propagateTilesetToAllLayers = [&](const Tileset& ts,
                                         const std::string& path) {
    for (size_t i = 0; i < layers.size(); ++i) {
      layers[i].tileset = ts;
      layers[i].tilesetPath = path;
      int tw = ts.tileW, th = ts.tileH;
      layers[i].tileset.configureGrid(tw, th, ts.cols, ts.rows);
    }
  };

// Convenience macros to keep existing code paths working with the active layer
#define CUR_LAYER (layers[activeLayer])
#define tileset (CUR_LAYER.tileset)
#define tilesetPath (CUR_LAYER.tilesetPath)
#define chunks (CUR_LAYER.chunks)
  // Default config buffers
  int defaultW = tilePx;
  int defaultH = tilePx;

  // Grid data (col,row for each cell)
  // Sparse grid by chunks instead of fixed dense grid (per-layer via macro
  // 'chunks')
  auto getChunkPtr = [&](const ChunkCoord& cc) -> Chunk* {
    auto it = chunks.find(cc);
    if (it == chunks.end()) return nullptr;
    return &it->second;
  };
  auto getOrCreateChunk = [&](const ChunkCoord& cc) -> Chunk& {
    auto it = chunks.find(cc);
    if (it == chunks.end()) it = chunks.emplace(cc, Chunk{}).first;
    return it->second;
  };
  auto getTileAt = [&](int gx, int gy) -> TileCR {
    ChunkCoord cc = toChunk(gx, gy);
    int lx, ly;
    toLocal(gx, gy, lx, ly);
    if (Chunk* ch = getChunkPtr(cc)) return ch->get(lx, ly);
    return TileCR{0, 0};
  };
  auto setTileAt = [&](int gx, int gy, TileCR v) {
    ChunkCoord cc = toChunk(gx, gy);
    int lx, ly;
    toLocal(gx, gy, lx, ly);
    if (v.col == 0 && v.row == 0) {
      auto it = chunks.find(cc);
      if (it != chunks.end()) {
        it->second.set(lx, ly, v);
        if (it->second.empty()) chunks.erase(it);
      }
    } else {
      auto& ch = getOrCreateChunk(cc);
      ch.set(lx, ly, v);
    }
  };

  // Selected tile in palette (col,row)
  TileCR selected{0, 0};

  // ---- Undo/Redo system ----
  struct TileChange {
    int layer;
    int gx;
    int gy;
    TileCR before;
    TileCR after;
  };
  struct Action {
    std::vector<TileChange> changes;
  };
  std::vector<Action> undoStack;
  std::vector<Action> redoStack;
  const size_t MAX_HISTORY = 200;
  // Stroke recording state
  bool strokeActive = false;
  int strokeLayer = 0;
  std::unordered_map<uint64_t, TileChange> strokeChanges;
  auto keyFor = [](int gx, int gy) -> uint64_t {
    uint64_t ux = static_cast<uint32_t>(gx);
    uint64_t uy = static_cast<uint32_t>(gy);
    return (ux << 32) ^ uy;
  };
  // Record a change for the current stroke (capture first-before and
  // last-after)
  auto recordStrokeChange = [&](int gx, int gy, const TileCR& after) {
    if (!strokeActive) return;
    uint64_t k = keyFor(gx, gy);
    auto it = strokeChanges.find(k);
    if (it == strokeChanges.end()) {
      TileCR before = getTileAt(gx, gy);
      strokeChanges.emplace(k, TileChange{strokeLayer, gx, gy, before, after});
    } else {
      it->second.after = after;
    }
  };
  auto finalizeStroke = [&]() {
    if (!strokeActive) return;
    if (!strokeChanges.empty()) {
      Action act;
      act.changes.reserve(strokeChanges.size());
      for (auto& kv : strokeChanges) act.changes.push_back(kv.second);
      // push to undo, clear redo
      undoStack.push_back(std::move(act));
      if (undoStack.size() > MAX_HISTORY) undoStack.erase(undoStack.begin());
      redoStack.clear();
    }
    strokeChanges.clear();
    strokeActive = false;
  };
  auto applyAction = [&](const Action& act, bool forward) {
    int saved = activeLayer;
    for (const auto& ch : act.changes) {
      activeLayer = ch.layer;
      setTileAt(ch.gx, ch.gy, forward ? ch.after : ch.before);
    }
    activeLayer = saved;
  };
  // doUndo/doRedo are defined later where showInfo is available
  std::function<void()> doUndo;
  std::function<void()> doRedo;

  // Text input state for loading tileset
  bool enteringPath = false;
  std::string pathBuffer = tilesetPath;

  // Inputs for tileset config (W, H)
  bool enteringTileW = false, enteringTileH = false, enteringRows = false,
       enteringCols = false;
  std::string tileWBuf = std::to_string(defaultW);
  std::string tileHBuf = std::to_string(defaultH);
  std::string rowsBuf = "";  // removed from UI; kept for minimal change
  std::string colsBuf = "";  // removed from UI; kept for minimal change

  // Save directory input state
  bool enteringSaveDir = false;
  std::string saveDirPath = getUserMapsDir().string();
  // UI state: layer dropdown selector
  bool layerDropdownOpen = false;
  std::string infoMessage;
  sf::Clock infoClock;

  // Grid camera state (origin can move via panning; zoom multiplies base tile
  // scale)
  sf::Vector2f gridOrigin = sf::Vector2f(
      static_cast<float>(paletteWidth + margin), static_cast<float>(margin));
  float gridZoom = 1.0f;     // zoom factor for grid (1.0 = default)
  bool panningGrid = false;  // middle-mouse panning
  sf::Vector2i lastPanMouse{0, 0};

  auto showInfo = [&](const std::string& msg) {
    infoMessage = msg;
    infoClock.restart();
  };

  // Define undo/redo now that showInfo exists
  doUndo = [&]() {
    if (undoStack.empty()) {
      showInfo("Nothing to undo");
      return;
    }
    Action act = std::move(undoStack.back());
    undoStack.pop_back();
    applyAction(act, /*forward=*/false);
    redoStack.push_back(std::move(act));
    showInfo("Undo");
  };
  doRedo = [&]() {
    if (redoStack.empty()) {
      showInfo("Nothing to redo");
      return;
    }
    Action act = std::move(redoStack.back());
    redoStack.pop_back();
    applyAction(act, /*forward=*/true);
    undoStack.push_back(std::move(act));
    showInfo("Redo");
  };

  // Layer selection helpers (after showInfo is defined)
  auto selectPrevLayer = [&]() {
    if (layers.empty()) return;
    activeLayer = (activeLayer - 1 + gsl::narrow_cast<int>(layers.size())) %
                  gsl::narrow_cast<int>(layers.size());
    Expects(activeLayer >= 0 &&
            activeLayer < gsl::narrow_cast<int>(layers.size()));
    ensureLayerTileset(activeLayer);
    showInfo(std::string("Active: ") + layers[activeLayer].name);
  };
  auto selectNextLayer = [&]() {
    if (layers.empty()) return;
    activeLayer = (activeLayer + 1) % gsl::narrow_cast<int>(layers.size());
    Expects(activeLayer >= 0 &&
            activeLayer < gsl::narrow_cast<int>(layers.size()));
    ensureLayerTileset(activeLayer);
    showInfo(std::string("Active: ") + layers[activeLayer].name);
  };

  // Helper to apply current buffers into tileset grid
  auto applyTilesetConfig = [&]() {
    if (tileset.texture.getSize().x == 0) {
      showInfo("Load a tileset path first (press L)");
      return;
    }
    int w = defaultW, h = defaultH, r = 0, c = 0;
    try {
      if (!tileWBuf.empty()) w = std::max(1, std::stoi(tileWBuf));
    } catch (...) {
    }
    try {
      if (!tileHBuf.empty()) h = std::max(1, std::stoi(tileHBuf));
    } catch (...) {
    }
    // Rows/Cols no longer set by UI; let tileset compute automatically
    r = 0;
    c = 0;
    if (tileset.configureGrid(w, h, c, r)) {
      selected = TileCR{0, 0};
      showInfo("Tileset config applied");
    } else {
      showInfo("Invalid tileset configuration");
    }
  };

  // Save helpers
  auto saveJsonToPath = [&](const fs::path& outPath) {
    auto jsonEscape = [](const std::string& s) {
      std::string out;
      out.reserve(s.size() + 8);
      for (char ch : s) {
        switch (ch) {
          case '\\':
            out += "\\\\";
            break;
          case '"':
            out += "\\\"";
            break;
          case '\n':
            out += "\\n";
            break;
          case '\r':
            out += "\\r";
            break;
          case '\t':
            out += "\\t";
            break;
          default:
            out.push_back(ch);
            break;
        }
      }
      return out;
    };
    std::ofstream out(outPath);
    if (!out.is_open()) {
      showInfo("Failed to save: " + outPath.string());
      return;
    }
    // Helper to get tile from arbitrary layer by temporarily switching
    // activeLayer
    auto getTileFromLayer = [&](int layerIdx, int gx, int gy) -> TileCR {
      int prev = activeLayer;
      activeLayer = layerIdx;
      TileCR t = getTileAt(gx, gy);
      activeLayer = prev;
      return t;
    };
    auto tilesetPathFromLayer = [&](int layerIdx) -> std::string {
      int prev = activeLayer;
      activeLayer = layerIdx;
      std::string p = tilesetPath;
      activeLayer = prev;
      return p;
    };
    auto tileDimFromLayer = [&](int layerIdx) -> std::pair<int, int> {
      int prev = activeLayer;
      activeLayer = layerIdx;
      auto pr = std::make_pair(tileset.tileW, tileset.tileH);
      activeLayer = prev;
      return pr;
    };

    out << "{\n  \"layers\": [\n";
    for (size_t li = 0; li < layers.size(); ++li) {
      // tileset path relative to assets/
      std::string tilesetOut = tilesetPathFromLayer(gsl::narrow_cast<int>(li));
      auto posAssets = tilesetOut.find("assets/");
      if (posAssets != std::string::npos)
        tilesetOut = tilesetOut.substr(posAssets);
      auto dims = tileDimFromLayer(gsl::narrow_cast<int>(li));
      out << "    {\n      \"name\": \""
          << jsonEscape(layers[li].name.empty()
                            ? (std::string("Layer ") + std::to_string(li + 1))
                            : layers[li].name)
          << "\",\n";
      out << "      \"tileset\": \"" << jsonEscape(tilesetOut) << "\",\n";
      out << "      \"tileW\": " << dims.first << ", \"tileH\": " << dims.second
          << ",\n";
      out << "      \"grid\": [\n";
      for (int y = 0; y < gridRows; ++y) {
        out << "        [";
        for (int x = 0; x < gridCols; ++x) {
          TileCR t = getTileFromLayer(gsl::narrow_cast<int>(li), x, y);
          out << "[" << t.col << "," << t.row << "]";
          if (x + 1 < gridCols) out << ",";
        }
        out << "]";
        if (y + 1 < gridRows) out << ",";
        out << "\n";
      }
      out << "      ]\n    }";
      if (li + 1 < layers.size()) out << ",";
      out << "\n";
    }
    out << "  ]\n}\n";
    out.close();
    showInfo(std::string("Saved JSON -> ") + outPath.string());
  };

  auto saveGridToPath = [&](const fs::path& outPath) {
    std::ofstream out(outPath);
    if (!out.is_open()) {
      showInfo("Failed to save: " + outPath.string());
      return;
    }
    out << "# BEP_GRID_SPARSE v1\n";
    for (const auto& kv : chunks) {
      const ChunkCoord& cc = kv.first;
      const Chunk& ch = kv.second;
      for (int ly = 0; ly < BEP_CHUNK_SIZE; ++ly) {
        for (int lx = 0; lx < BEP_CHUNK_SIZE; ++lx) {
          TileCR t = ch.get(lx, ly);
          if (t.col != 0 || t.row != 0) {
            int gx = cc.cx * BEP_CHUNK_SIZE + lx;
            int gy = cc.cy * BEP_CHUNK_SIZE + ly;
            out << gx << ' ' << gy << ' ' << t.col << ' ' << t.row << '\n';
          }
        }
      }
    }
    out.close();
    showInfo(std::string("Saved -> ") + outPath.string());
  };

  // Removed legacy grid save helper

  // Save current grid to JSON in chosen folder (fixed size grid + tileset path)
  auto saveJson = [&]() {
    fs::path mapsDir =
        saveDirPath.empty() ? getUserMapsDir() : fs::path(saveDirPath);
    std::error_code ec;
    if (!fs::exists(mapsDir)) {
      fs::create_directories(mapsDir, ec);
      if (ec) {
        showInfo(std::string("Failed to create maps dir: ") + mapsDir.string());
        return;
      }
    } else if (!fs::is_directory(mapsDir)) {
      showInfo(std::string("Not a directory: ") + mapsDir.string());
      return;
    }
    std::string fileName = timestampName();
    fs::path outPath = mapsDir / fileName;
    saveJsonToPath(outPath);
  };

  // Legacy .grid support removed; JSON-only
  auto loadGridFromFile = [&](const std::string& filePath) {
    showInfo("Grid format no longer supported. Use JSON maps.");
  };

  // Load from JSON map: supports single-layer schema {tileset, tileW?, tileH?,
  // grid} and multi-layer schema {layers: [{name?, tileset, tileW?, tileH?,
  // grid}, ...]}
  auto loadJsonFromFile = [&](const std::string& filePath) {
    std::string resolved = findAssetPath(filePath);
    std::ifstream in(resolved, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
      showInfo("Failed to open: " + resolved);
      return;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();
    std::string json = ss.str();

    auto extractIntField = [](const std::string& src,
                              const std::string& key) -> int {
      const std::string quoted = '"' + key + '"';
      size_t pos = src.find(quoted);
      if (pos == std::string::npos) return 0;
      pos = src.find(':', pos);
      if (pos == std::string::npos) return 0;
      ++pos;
      while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t')) ++pos;
      std::string num;
      while (pos < src.size() &&
             (std::isdigit(static_cast<unsigned char>(src[pos])) ||
              src[pos] == '-'))
        num += src[pos++];
      try {
        return std::stoi(num);
      } catch (...) {
        return 0;
      }
    };
    auto extractStringField = [](const std::string& src,
                                 const std::string& key) -> std::string {
      const std::string quoted = '"' + key + '"';
      size_t pos = src.find(quoted);
      if (pos == std::string::npos) return {};
      pos = src.find(':', pos);
      if (pos == std::string::npos) return {};
      pos = src.find('"', pos);
      if (pos == std::string::npos) return {};
      ++pos;
      std::string out;
      bool escape = false;
      for (; pos < src.size(); ++pos) {
        char ch = src[pos];
        if (escape) {
          out.push_back(ch);
          escape = false;
          continue;
        }
        if (ch == '\\') {
          escape = true;
          continue;
        }
        if (ch == '"') break;
        out.push_back(ch);
      }
      return out;
    };
    auto extractGridStart = [](const std::string& src) -> size_t {
      size_t pos = src.find("\"grid\"");
      if (pos == std::string::npos) return std::string::npos;
      pos = src.find(':', pos);
      if (pos == std::string::npos) return std::string::npos;
      pos = src.find('[', pos);
      return pos;
    };
    auto parseGrid = [](const std::string& src,
                        size_t startIdx) -> std::vector<std::vector<TileCR>> {
      std::vector<std::vector<TileCR>> grid;
      std::vector<TileCR> currentRow;
      int depth = 0;
      std::string num;
      std::vector<int> pair;
      auto flush = [&]() {
        if (!num.empty()) {
          try {
            pair.push_back(std::stoi(num));
          } catch (...) {
          }
          num.clear();
        }
      };
      for (size_t i = startIdx; i < src.size(); ++i) {
        char ch = src[i];
        if (ch == '[') {
          ++depth;
          continue;
        }
        if (ch == ']') {
          flush();
          if (depth == 3) {
            if (pair.size() == 2)
              currentRow.push_back(TileCR{pair[0], pair[1]});
            pair.clear();
          } else if (depth == 2) { /* end of row */
          } else if (depth == 1) {
            grid.push_back(std::move(currentRow));
            currentRow = {};
            break;
          }
          --depth;
          if (depth == 1) {
            if (!currentRow.empty()) {
              grid.push_back(std::move(currentRow));
              currentRow = {};
            }
          }
          continue;
        }
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' ||
            ch == '+')
          num.push_back(ch);
        else if (ch == ',' || std::isspace(static_cast<unsigned char>(ch)))
          flush();
      }
      while (!grid.empty() && grid.back().empty()) grid.pop_back();
      return grid;
    };

    auto hasKey = [&](const std::string& key) -> bool {
      const std::string quoted = '"' + key + '"';
      return json.find(quoted) != std::string::npos;
    };

    // Detect multi-layer schema
    bool isLayered = hasKey("layers");
    if (!isLayered) {
      // Backward compatible single-layer path
      // Reset to one layer
      layers.clear();
      ensureInitialLayer();
      activeLayer = 0;
      // Apply tileset if present
      std::string ts = extractStringField(json, "tileset");
      int jsonTileW = extractIntField(json, "tileW");
      int jsonTileH = extractIntField(json, "tileH");
      if (!ts.empty()) {
        std::string resolvedTs = findAssetPath(ts);
        if (tileset.loadTexture(resolvedTs)) {
          tilesetPath = resolvedTs;
          if (jsonTileW > 0) tileWBuf = std::to_string(jsonTileW);
          if (jsonTileH > 0) tileHBuf = std::to_string(jsonTileH);
          applyTilesetConfig();
        }
      }
      size_t gs = extractGridStart(json);
      if (gs == std::string::npos) {
        showInfo("Invalid JSON: missing grid");
        return;
      }
      auto grid = parseGrid(json, gs);
      chunks.clear();
      for (int y = 0; y < gsl::narrow_cast<int>(grid.size()); ++y) {
        for (int x = 0; x < gsl::narrow_cast<int>(grid[y].size()); ++x) {
          TileCR t = grid[y][x];
          if (t.col != 0 || t.row != 0) setTileAt(x, y, t);
        }
      }
      showInfo("Loaded JSON map: " + resolved);
      return;
    }

    // Layered path: parse array of layers crudely
    layers.clear();
    activeLayer = 0;
    size_t layersPos = json.find("\"layers\"");
    size_t arrStart = layersPos == std::string::npos
                          ? std::string::npos
                          : json.find('[', layersPos);
    if (arrStart == std::string::npos) {
      showInfo("Invalid JSON: layers array not found");
      return;
    }
    size_t i = arrStart + 1;
    int depth = 1;  // inside array
    while (i < json.size() && depth > 0) {
      // Find next object '{'
      while (i < json.size() && json[i] != '{' && json[i] != ']') ++i;
      if (i >= json.size()) break;
      if (json[i] == ']') {
        --depth;
        break;
      }
      // Parse object span
      size_t objStart = i;
      int objDepth = 0;
      do {
        if (json[i] == '{')
          ++objDepth;
        else if (json[i] == '}')
          --objDepth;
        ++i;
      } while (i < json.size() && objDepth > 0);
      size_t objEnd = i;  // one past '}'
      std::string obj = json.substr(objStart, objEnd - objStart);

      // Extract fields
      std::string ts = extractStringField(obj, "tileset");
      std::string name = extractStringField(obj, "name");
      int jsonTileW = extractIntField(obj, "tileW");
      int jsonTileH = extractIntField(obj, "tileH");
      size_t gstart = std::string::npos;
      {
        const std::string quoted = "\"grid\"";
        size_t p = obj.find(quoted);
        if (p != std::string::npos) {
          p = obj.find(':', p);
          if (p != std::string::npos) gstart = obj.find('[', p);
        }
      }
      std::vector<std::vector<TileCR>> grid;
      if (gstart != std::string::npos) grid = parseGrid(obj, gstart);

      // Create new layer and configure it via active layer macros to avoid
      // macro collisions
      layers.emplace_back();
      int li = gsl::narrow_cast<int>(layers.size()) - 1;
      layers[li].name = !name.empty()
                            ? name
                            : (std::string("Layer ") + std::to_string(li + 1));
      layers[li].visible = true;
      activeLayer = li;
      if (!ts.empty()) {
        std::string resolvedTs = findAssetPath(ts);
        if (tileset.loadTexture(resolvedTs)) {
          tilesetPath = resolvedTs;
          if (jsonTileW > 0) tileset.tileW = jsonTileW;
          if (jsonTileH > 0) tileset.tileH = jsonTileH;
          // Configure grid sizes (rows/cols) based on texture and tile dims
          tileset.configureGrid(tileset.tileW > 0 ? tileset.tileW : tilePx,
                                tileset.tileH > 0 ? tileset.tileH : tilePx,
                                /*cols*/ 0, /*rows*/ 0);
        }
      }
      // Fill tiles for this layer
      chunks.clear();
      for (int y = 0; y < gsl::narrow_cast<int>(grid.size()); ++y) {
        for (int x = 0; x < gsl::narrow_cast<int>(grid[y].size()); ++x) {
          TileCR t = grid[y][x];
          if (t.col != 0 || t.row != 0) setTileAt(x, y, t);
        }
      }
    }
    ensureInitialLayer();
    activeLayer = 0;
    showInfo("Loaded layered JSON map: " + resolved);
  };

  // Load default map if available
  {
    std::string jsonDefault = findAssetPath(ASSETS_MAPS_JSON);
    if (!jsonDefault.empty() && fs::exists(jsonDefault)) {
      loadJsonFromFile(jsonDefault);
    }
  }

  // Palette draw origin and layout (shared by draw and hit-test)
  const float thumbScale = 2.0f;
  const int cell = gsl::narrow_cast<int>(tilePx * thumbScale);
  const int padding = 6;
  const int x0 = 8;
  // Layout for tileset config and save controls
  const int cfgLabelY = 128;
  const int cfgInputsY = 148;
  const int cfgButtonsY = 178;
  const int saveLabelY = 214;
  const int saveInputY = 238;    // +4 for spacing
  const int saveButtonsY = 272;  // +6 for spacing
  const int saveButtonsY2 = saveButtonsY + 34;
  // Shift tileset thumbnails down to make space for config + save controls
  // (including second row)
  const int buttonsBottom = saveButtonsY2 + 28;  // 28 = button height
  const int y0 = buttonsBottom + 16;             // add margin below buttons

  // --- Palette vertical scrolling state & helpers ---
  float paletteScrollY = 0.f;  // in palette coordinate space
  auto computePaletteContentHeight = [&]() -> float {
    // Base content up to start of thumbnails
    float content = static_cast<float>(y0);
    if (tileset.loaded) {
      int colsPerRow =
          std::max(1, (paletteWidth - padding - x0) / (cell + padding));
      int total = tileset.rows * tileset.cols;
      int rowsNeeded = (total + colsPerRow - 1) / colsPerRow;
      content = std::max(
          content,
          static_cast<float>(y0 + rowsNeeded * (cell + padding) + padding));
    }
    // Ensure at least window height so we don't get negative maxScroll
    return std::max(content, static_cast<float>(winH));
  };
  auto clampAndApplyPaletteScroll = [&]() {
    float contentH = computePaletteContentHeight();
    float maxScroll = std::max(0.f, contentH - static_cast<float>(winH));
    if (paletteScrollY < 0.f) paletteScrollY = 0.f;
    if (paletteScrollY > maxScroll) paletteScrollY = maxScroll;
    paletteView.setSize(sf::Vector2f(static_cast<float>(paletteWidth),
                                     static_cast<float>(winH)));
    paletteView.setCenter(
        sf::Vector2f(static_cast<float>(paletteWidth) / 2.f,
                     static_cast<float>(winH) / 2.f + paletteScrollY));
  };
  // Helper to get scrollbar track and thumb rects in palette-view coordinates
  auto getScrollbarRects =
      [&]() -> std::optional<std::pair<sf::FloatRect, sf::FloatRect>> {
    float contentH = computePaletteContentHeight();
    if (contentH <= static_cast<float>(winH))
      return std::nullopt;  // no scrollbar
    float trackW = 8.f;     // widened for easier interaction
    float trackX =
        static_cast<float>(paletteWidth) - (trackW + 2.f);  // 2px margin
    float trackH = static_cast<float>(winH) - 8.f;
    float viewTop = paletteView.getCenter().y - paletteView.getSize().y / 2.f;
    float trackY = viewTop + 4.f;
    float thumbH =
        std::max(24.f, trackH * (static_cast<float>(winH) / contentH));
    float maxScroll = contentH - static_cast<float>(winH);
    float t = (maxScroll > 0.f) ? (paletteScrollY / maxScroll) : 0.f;
    float thumbY = trackY + t * (trackH - thumbH);
    sf::FloatRect track(sf::Vector2f(trackX, trackY),
                        sf::Vector2f(trackW, trackH));
    sf::FloatRect thumb(sf::Vector2f(trackX, thumbY),
                        sf::Vector2f(trackW, thumbH));
    return std::make_pair(track, thumb);
  };
  // Dragging state for scrollbar thumb
  bool draggingScrollThumb = false;
  float dragOffsetY =
      0.f;  // distance from mouse Y to thumb top when drag starts

  // Painting state for grid drag drawing/erasing
  bool paintingLeft = false;
  bool paintingRight = false;
  int lastPaintGX = -1;
  int lastPaintGY = -1;
  auto paintCell = [&](int gx, int gy, bool leftButton) {
    // No fixed bounds: infinite grid
    TileCR v = leftButton ? selected : TileCR{0, 0};
    recordStrokeChange(gx, gy, v);
    int savedLayer = activeLayer;
    if (strokeActive) activeLayer = strokeLayer;
    setTileAt(gx, gy, v);
    activeLayer = savedLayer;
  };
  auto paintLine = [&](int x0, int y0, int x1, int y1, bool leftButton) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int steps = std::max(adx, ady);
    if (steps <= 0) {
      paintCell(x1, y1, leftButton);
      return;
    }
    // Step-inclusive from previous cell toward current to avoid gaps on fast
    // mouse moves
    for (int i = 1; i <= steps; ++i) {
      int x = x0 + (dx * i + (dx >= 0 ? steps / 2 : -steps / 2)) / steps;
      int y = y0 + (dy * i + (dy >= 0 ? steps / 2 : -steps / 2)) / steps;
      paintCell(x, y, leftButton);
    }
  };

  // Initialize view center with scroll
  clampAndApplyPaletteScroll();

  // Simple helpers for drawing
  sf::RectangleShape paletteBG(
      sf::Vector2f(static_cast<float>(paletteWidth), static_cast<float>(winH)));
  paletteBG.setFillColor(sf::Color(30, 30, 40));

  sf::RectangleShape gridBG(
      sf::Vector2f(static_cast<float>(gridPxW), static_cast<float>(gridPxH)));
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
        panningGrid = false;
        // finalize any active stroke (no-op if none)
        finalizeStroke();
      }

      // Keep views and palette size in sync with window to avoid overlap on
      // resize
      if (event.is<sf::Event::Resized>()) {
        auto sz = window.getSize();
        winW = gsl::narrow_cast<int>(sz.x);
        winH = gsl::narrow_cast<int>(sz.y);
        defaultView = sf::View(sf::FloatRect(
            {0.f, 0.f}, {static_cast<float>(winW), static_cast<float>(winH)}));
        window.setView(defaultView);
        setPaletteViewport();
        paletteBG.setSize(sf::Vector2f(static_cast<float>(paletteWidth),
                                       static_cast<float>(winH)));
        clampAndApplyPaletteScroll();
      }

      // Mouse wheel: scroll palette or zoom grid depending on mouse area
      if (event.is<sf::Event::MouseWheelScrolled>()) {
        const auto* e = event.getIf<sf::Event::MouseWheelScrolled>();
        if (e) {
          // Only scroll when mouse is over the left palette area
          sf::Vector2i mp = sf::Mouse::getPosition(window);
          if (mp.x >= 0 && mp.x < paletteWidth) {
            // delta > 0 typically means scroll up
            float step =
                60.f;  // pixels per notch; trackpads provide fractional deltas
            paletteScrollY -= e->delta * step;
            clampAndApplyPaletteScroll();
          } else {
            // Zoom grid when mouse is over the grid panel area
            float panelLeft = static_cast<float>(paletteWidth + margin);
            float panelTop = static_cast<float>(margin);
            float panelW =
                std::max(0.f, static_cast<float>(winW) -
                                  (panelLeft + static_cast<float>(margin)));
            float panelH =
                std::max(0.f, static_cast<float>(winH) -
                                  (2.f * static_cast<float>(margin)));
            sf::FloatRect panelRect({panelLeft, panelTop}, {panelW, panelH});
            if (panelRect.contains(sf::Vector2f(static_cast<float>(mp.x),
                                                static_cast<float>(mp.y)))) {
              float oldZoom = gridZoom;
              float cellPx = static_cast<float>(tilePx) * tileScale * gridZoom;
              // Zoom speed factor
              float zoomStep = 1.1f;
              if (e->delta > 0)
                gridZoom *= zoomStep;
              else
                gridZoom /= zoomStep;
              if (gridZoom < 0.25f) gridZoom = 0.25f;
              if (gridZoom > 8.0f) gridZoom = 8.0f;
              float newCellPx =
                  static_cast<float>(tilePx) * tileScale * gridZoom;
              float oldCellPx = cellPx;
              // Keep tile under cursor stable by adjusting origin
              if (gridZoom != oldZoom && oldCellPx > 0.0f) {
                sf::Vector2f mouseF(static_cast<float>(mp.x),
                                    static_cast<float>(mp.y));
                sf::Vector2f rel = mouseF - gridOrigin;
                sf::Vector2f world(rel.x / oldCellPx, rel.y / oldCellPx);
                sf::Vector2f newRel(world.x * newCellPx, world.y * newCellPx);
                gridOrigin = mouseF - newRel;
              }
            }
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
            float t = (trackH - thumbH) > 0.f
                          ? (newThumbTop - trackY) / (trackH - thumbH)
                          : 0.f;
            paletteScrollY = t * std::max(0.f, maxScroll);
            clampAndApplyPaletteScroll();
          }
        }

        // Grid painting while holding mouse button (default view coords) or
        // panning
        if (paintingLeft || paintingRight) {
          sf::Vector2i mp(me->position.x, me->position.y);
          float cellPx = static_cast<float>(tilePx) * tileScale * gridZoom;
          float panelLeft = static_cast<float>(paletteWidth + margin);
          float panelTop = static_cast<float>(margin);
          float panelW =
              std::max(0.f, static_cast<float>(winW) -
                                (panelLeft + static_cast<float>(margin)));
          float panelH = std::max(0.f, static_cast<float>(winH) -
                                           (2.f * static_cast<float>(margin)));
          sf::FloatRect panelRect({panelLeft, panelTop}, {panelW, panelH});
          if (panelRect.contains(sf::Vector2f(static_cast<float>(mp.x),
                                              static_cast<float>(mp.y)))) {
            int gx = static_cast<int>(
                std::floor((static_cast<float>(mp.x) - gridOrigin.x) / cellPx));
            int gy = static_cast<int>(
                std::floor((static_cast<float>(mp.y) - gridOrigin.y) / cellPx));
            if (gx != lastPaintGX || gy != lastPaintGY) {
              bool leftBtn = paintingLeft;
              if (lastPaintGX >= 0 && lastPaintGY >= 0)
                paintLine(lastPaintGX, lastPaintGY, gx, gy, leftBtn);
              else
                paintCell(gx, gy, leftBtn);
              lastPaintGX = gx;
              lastPaintGY = gy;
            }
          }
        }
        // Panning with middle mouse button
        if (panningGrid) {
          sf::Vector2i mp(me->position.x, me->position.y);
          sf::Vector2i delta = mp - lastPanMouse;
          gridOrigin.x += static_cast<float>(delta.x);
          gridOrigin.y += static_cast<float>(delta.y);
          lastPanMouse = mp;
        }
      }
      if (event.is<sf::Event::MouseButtonReleased>()) {
        const auto* e = event.getIf<sf::Event::MouseButtonReleased>();
        if (e && e->button == sf::Mouse::Button::Left) {
          draggingScrollThumb = false;
          paintingLeft = false;
          lastPaintGX = lastPaintGY = -1;
          finalizeStroke();
        }
        if (e && e->button == sf::Mouse::Button::Right) {
          paintingRight = false;
          lastPaintGX = lastPaintGY = -1;
          finalizeStroke();
        }
        if (e && e->button == sf::Mouse::Button::Middle) {
          panningGrid = false;
        }
      }

      if (event.is<sf::Event::KeyPressed>()) {
        const auto* e = event.getIf<sf::Event::KeyPressed>();
        if (e) {
          if (!enteringPath && !enteringSaveDir && !enteringTileW &&
              !enteringTileH && !enteringRows && !enteringCols) {
            if (e->code == sf::Keyboard::Key::Escape) window.close();
            // Removed legacy grid save shortcut (S). Use J for JSON.
            if (e->code == sf::Keyboard::Key::J) saveJson();
            // Undo/Redo shortcuts: Ctrl/Cmd+Z (undo), Ctrl/Cmd+Y or
            // Shift+Ctrl/Cmd+Z (redo)
            bool ctrl = e->control || e->system;  // system covers Cmd on macOS
            bool shift = e->shift;
            if (ctrl && e->code == sf::Keyboard::Key::Z && !shift) {
              doUndo();
            } else if ((ctrl && e->code == sf::Keyboard::Key::Y) ||
                       (ctrl && e->code == sf::Keyboard::Key::Z && shift)) {
              doRedo();
            }
            // Layer controls
            if (e->code == sf::Keyboard::Key::F1) {
              selectPrevLayer();
            }
            if (e->code == sf::Keyboard::Key::F2) {
              selectNextLayer();
            }
            if (e->code == sf::Keyboard::Key::F4) {
              int idxInsert = activeLayer + 1;
              activeLayer = insertNewLayerAt(idxInsert);
              showInfo("Layer added");
            }
            if (e->code == sf::Keyboard::Key::F5) {
              if (layers.size() > 1) {
                layers.erase(layers.begin() + activeLayer);
                if (activeLayer >= static_cast<int>(layers.size()))
                  activeLayer = static_cast<int>(layers.size()) - 1;
                showInfo("Layer deleted");
              }
            }
            if (e->code == sf::Keyboard::Key::F3) {
              if (!layers.empty()) {
                layers[activeLayer].visible = !layers[activeLayer].visible;
                showInfo(std::string("Visibility: ") +
                         (layers[activeLayer].visible ? "On" : "Off"));
              }
            }
            if (e->code == sf::Keyboard::Key::N) {
              chunks.clear();
              showInfo("New empty grid");
            }
            if (e->code == sf::Keyboard::Key::L) {
              enteringPath = true;
              enteringSaveDir = false;
              enteringTileW = enteringTileH = enteringRows = enteringCols =
                  false;
              pathBuffer = tilesetPath;
              showInfo("Type tileset path and press Enter");
            }
            if (e->code == sf::Keyboard::Key::O) {
#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
              if (auto sel = winPickMapFile(L"Open map file")) {
                std::string p = *sel;
                loadJsonFromFile(p);
              } else
                showInfo("Open canceled");
#else
              std::string jsonDefault = findAssetPath(ASSETS_MAPS_JSON);
              if (!jsonDefault.empty() && fs::exists(jsonDefault))
                loadJsonFromFile(jsonDefault);
#endif
            }
            // Optional keyboard scroll helpers
            if (e->code == sf::Keyboard::Key::PageDown) {
              paletteScrollY += 0.9f * static_cast<float>(winH);
              clampAndApplyPaletteScroll();
            }
            if (e->code == sf::Keyboard::Key::PageUp) {
              paletteScrollY -= 0.9f * static_cast<float>(winH);
              clampAndApplyPaletteScroll();
            }
            if (e->code == sf::Keyboard::Key::Home) {
              paletteScrollY = 0.f;
              clampAndApplyPaletteScroll();
            }
            if (e->code == sf::Keyboard::Key::End) {
              paletteScrollY = 1e9f;
              clampAndApplyPaletteScroll();
            }
          } else {
            if (e->code == sf::Keyboard::Key::Enter) {
              if (enteringPath) {
                if (tileset.loadTexture(pathBuffer)) {
                  // Configurar grid con los buffers actuales
                  applyTilesetConfig();
                  propagateTilesetToAllLayers(tileset, pathBuffer);
                  showInfo("Loaded tileset: " + pathBuffer);
                } else {
                  showInfo("Failed tileset: " + pathBuffer);
                }
                enteringPath = false;
              } else if (enteringSaveDir) {
                enteringSaveDir = false;
                showInfo(std::string("Save folder set: ") + saveDirPath);
              } else if (enteringTileW || enteringTileH) {
                enteringTileW = enteringTileH = enteringRows = enteringCols =
                    false;
                applyTilesetConfig();
                clampAndApplyPaletteScroll();
              }
            }
            if (e->code == sf::Keyboard::Key::Escape) {
              enteringPath = false;
              enteringSaveDir = false;
              enteringTileW = enteringTileH = enteringRows = enteringCols =
                  false;
            }
          }
        }
      }

      if ((enteringPath || enteringSaveDir || enteringTileW || enteringTileH ||
           enteringRows || enteringCols) &&
          event.is<sf::Event::TextEntered>()) {
        const auto* e = event.getIf<sf::Event::TextEntered>();
        if (e) {
          if (e->unicode == 8) {  // backspace
            if (enteringPath) {
              if (!pathBuffer.empty()) pathBuffer.pop_back();
            } else if (enteringSaveDir) {
              if (!saveDirPath.empty()) saveDirPath.pop_back();
            } else if (enteringTileW) {
              if (!tileWBuf.empty()) tileWBuf.pop_back();
            } else if (enteringTileH) {
              if (!tileHBuf.empty()) tileHBuf.pop_back();
            } else if (enteringRows) {
              if (!rowsBuf.empty()) rowsBuf.pop_back();
            } else if (enteringCols) {
              if (!colsBuf.empty()) colsBuf.pop_back();
            }
          } else if (enteringPath) {
            if (e->unicode >= 32 && e->unicode < 127)
              pathBuffer += static_cast<char>(e->unicode);
          } else {
            // Numeric-only for config
            if (e->unicode >= '0' && e->unicode <= '9') {
              if (enteringTileW)
                tileWBuf += static_cast<char>(e->unicode);
              else if (enteringTileH)
                tileHBuf += static_cast<char>(e->unicode);
            }
          }
        }
      }

      if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* e = event.getIf<sf::Event::MouseButtonPressed>();
        if (!e) continue;
        // Use event-provided position to avoid HiDPI scaling issues
        sf::Vector2i mp(e->position.x, e->position.y);
        // Map to palette coordinate space for accurate hit testing while
        // scrolled
        sf::Vector2f mpPaletteF = window.mapPixelToCoords(mp, paletteView);
        sf::Vector2i mpPalette(gsl::narrow_cast<int>(mpPaletteF.x),
                               gsl::narrow_cast<int>(mpPaletteF.y));

        // Tileset path input and Load button hit-tests
        if (mp.x >= 0 && mp.x < paletteWidth) {
          // Hit-test for layer selector (dropdown) and add/delete at top
          {
            const int layerBtnY = layerHeaderY();
            const int btnW = 24;
            const int btnH = 22;
            const int gap = 4;
            const int selW = std::max(
                120,
                paletteWidth - 8 - (2 * gap + 2 * btnW) -
                    140);  // responsive width, leaving ~140px para path label
            const int startX = paletteWidth - 8 - (selW + 2 * gap + 2 * btnW);
            const int xSelect = startX;
            const int xAdd = xSelect + selW + gap;
            const int xDel = xAdd + btnW + gap;
            sf::IntRect rectSelect({xSelect, layerBtnY}, {selW, btnH});
            sf::IntRect rectAdd({xAdd, layerBtnY}, {btnW, btnH});
            sf::IntRect rectDel({xDel, layerBtnY}, {btnW, btnH});

            // Dropdown open area (below select field)
            const int itemH = btnH;  // one item per button height
            const int dropY = layerBtnY + btnH + 2;
            const int dropH = itemH * gsl::narrow_cast<int>(layers.size());
            sf::IntRect rectDropdown({xSelect, dropY}, {selW, dropH});

            if (e->button == sf::Mouse::Button::Left) {
              // Toggle open/close on select field click
              if (rectSelect.contains(mpPalette)) {
                layerDropdownOpen = !layerDropdownOpen;
                continue;
              }
              // If dropdown open, check selection or close when clicking
              // outside
              if (layerDropdownOpen) {
                if (rectDropdown.contains(mpPalette)) {
                  int idx = (mpPalette.y - dropY) / itemH;
                  if (idx >= 0 && idx < gsl::narrow_cast<int>(layers.size())) {
                    activeLayer = idx;
                    ensureLayerTileset(activeLayer);
                    showInfo(std::string("Active: ") +
                             layers[activeLayer].name);
                  }
                  layerDropdownOpen = false;
                  continue;
                } else if (!rectSelect.contains(mpPalette)) {
                  layerDropdownOpen = false;  // click outside closes
                }
              }

              // Add/Delete buttons
              if (rectAdd.contains(mpPalette)) {
                int idxInsert = activeLayer + 1;
                activeLayer = insertNewLayerAt(idxInsert);
                showInfo("Layer added");
                continue;
              }
              if (rectDel.contains(mpPalette)) {
                if (layers.size() > 1) {
                  layers.erase(layers.begin() + activeLayer);
                  if (activeLayer >= gsl::narrow_cast<int>(layers.size()))
                    activeLayer = gsl::narrow_cast<int>(layers.size()) - 1;
                  showInfo("Layer deleted");
                }
                continue;
              }
            }
          }
          int pathInputW =
              paletteWidth - 24 - 100 - 6;  // input + gap + load button
          sf::IntRect pathRect({12, 62}, {pathInputW, 26});
          sf::IntRect loadRect({12 + pathInputW + 6, 62}, {100, 26});
          if (pathRect.contains(mpPalette)) {
            enteringPath = true;
            enteringSaveDir = enteringTileW = enteringTileH = enteringRows =
                enteringCols = false;
            continue;
          }
          if (loadRect.contains(mpPalette) &&
              e->button == sf::Mouse::Button::Left) {
#ifdef _WIN32
            // Open dialog to pick image and load immediately
            if (auto sel = winPickImageFile()) {
              pathBuffer = *sel;
              if (tileset.loadTexture(pathBuffer)) {
                applyTilesetConfig();
                propagateTilesetToAllLayers(tileset, pathBuffer);
                clampAndApplyPaletteScroll();
                showInfo(std::string("Loaded tileset: ") + pathBuffer);
              } else {
                showInfo(std::string("Failed tileset: ") + pathBuffer);
              }
            } else {
              showInfo("Load canceled");
            }
#else
            // Fallback: use typed path
            if (tileset.loadTexture(pathBuffer)) {
              applyTilesetConfig();
              propagateTilesetToAllLayers(tileset, pathBuffer);
              clampAndApplyPaletteScroll();
              showInfo(std::string("Loaded tileset: ") + pathBuffer);
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
            enteringTileW = true;
            enteringTileH = enteringRows = enteringCols = enteringSaveDir =
                enteringPath = false;
            continue;
          }
          if (hRect.contains(mpPalette)) {
            enteringTileH = true;
            enteringTileW = enteringRows = enteringCols = enteringSaveDir =
                enteringPath = false;
            continue;
          }
          if (applyRect.contains(mpPalette) &&
              e->button == sf::Mouse::Button::Left) {
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
          sf::IntRect saveAsBtnRect({12 + 110, saveButtonsY}, {100, 28});
          sf::IntRect openMapRect({12, saveButtonsY2}, {230, 28});
          if (inputRect.contains(mpPalette)) {
            enteringSaveDir = true;
            enteringPath = enteringTileW = enteringTileH = enteringRows =
                enteringCols = false;
            continue;
          }
          if (saveBtnRect.contains(mpPalette) &&
              e->button == sf::Mouse::Button::Left) {
            saveJson();
            continue;
          }
          if (saveAsBtnRect.contains(mpPalette) &&
              e->button == sf::Mouse::Button::Left) {
#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
            if (auto out = winSaveMapAs(L"Save map as")) {
              std::string path = *out;
              if (path.size() < 5 || path.substr(path.size() - 5) != ".json")
                path += ".json";
              saveJsonToPath(fs::path(path));
            } else {
              showInfo("Save As canceled");
            }
#else
            saveJson();
#endif
            continue;
          }
          if (openMapRect.contains(mpPalette) &&
              e->button == sf::Mouse::Button::Left) {
#if defined(_WIN32) && defined(MAPEDITOR_ENABLE_WIN32_DIALOGS)
            if (auto sel = winPickMapFile(L"Open map file")) {
              std::string p = *sel;
              loadJsonFromFile(p);
            } else {
              showInfo("Open canceled");
            }
#else
            std::string jsonDefault = findAssetPath(ASSETS_MAPS_JSON);
            if (!jsonDefault.empty() && fs::exists(jsonDefault))
              loadJsonFromFile(jsonDefault);
#endif
            continue;
          }
        }

        // Scrollbar hit-tests (track and thumb) when clicking in palette area
        if (mp.x >= 0 && mp.x < paletteWidth &&
            e->button == sf::Mouse::Button::Left) {
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
                dragOffsetY =
                    mpPaletteF.y -
                    thumb.position.y;  // preserve grab offset inside the thumb
                continue;              // consume
              } else {
                // Jump to clicked position on track
                float trackY = track.position.y;
                float trackH = track.size.y;
                float thumbH = thumb.size.y;
                float contentH = computePaletteContentHeight();
                float maxScroll = contentH - static_cast<float>(winH);
                float t = (trackH - thumbH) > 0.f
                              ? (mpPaletteF.y - trackY - thumbH * 0.5f) /
                                    (trackH - thumbH)
                              : 0.f;
                if (t < 0.f) t = 0.f;
                if (t > 1.f) t = 1.f;
                paletteScrollY = t * std::max(0.f, maxScroll);
                clampAndApplyPaletteScroll();
                continue;  // consume
              }
            }
          }
        }

        // Click in palette area to select tile
        if (mp.x >= 0 && mp.x < paletteWidth) {
          if (!tileset.loaded) continue;
          const int colsPerRow =
              std::max(1, (paletteWidth - padding - x0) / (cell + padding));

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

        // Click in grid panel to paint / erase or start panning
        float cellPx = static_cast<float>(tilePx) * tileScale * gridZoom;
        float panelLeft = static_cast<float>(paletteWidth + margin);
        float panelTop = static_cast<float>(margin);
        float panelW =
            std::max(0.f, static_cast<float>(winW) -
                              (panelLeft + static_cast<float>(margin)));
        float panelH = std::max(
            0.f, static_cast<float>(winH) - (2.f * static_cast<float>(margin)));
        sf::FloatRect panelRect({panelLeft, panelTop}, {panelW, panelH});
        if (panelRect.contains(sf::Vector2f(static_cast<float>(mp.x),
                                            static_cast<float>(mp.y)))) {
          int gx = static_cast<int>(
              std::floor((static_cast<float>(mp.x) - gridOrigin.x) / cellPx));
          int gy = static_cast<int>(
              std::floor((static_cast<float>(mp.y) - gridOrigin.y) / cellPx));
          if (e->button == sf::Mouse::Button::Left) {
            if (!strokeActive) {
              strokeActive = true;
              strokeLayer = activeLayer;
              strokeChanges.clear();
            }
            paintCell(gx, gy, true);
            paintingLeft = true;
            lastPaintGX = gx;
            lastPaintGY = gy;
          } else if (e->button == sf::Mouse::Button::Right) {
            if (!strokeActive) {
              strokeActive = true;
              strokeLayer = activeLayer;
              strokeChanges.clear();
            }
            paintCell(gx, gy, false);
            paintingRight = true;
            lastPaintGX = gx;
            lastPaintGY = gy;
          } else if (e->button == sf::Mouse::Button::Middle) {
            panningGrid = true;
            lastPanMouse = mp;
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
      paletteBG.setSize(
          sf::Vector2f(static_cast<float>(paletteWidth),
                       std::max(static_cast<float>(winH), contentH)));
    }

    // Left palette background
    paletteBG.setPosition(sf::Vector2f(0.f, 0.f));
    window.draw(paletteBG);

    // Palette title
    sf::Text title(font);
    title.setString(
        std::string("Tileset (Layer: ") +
        (layers.empty() ? std::string("-") : layers[activeLayer].name) + ")");
    title.setCharacterSize(22);
    title.setFillColor(sf::Color(230, 230, 240));
    title.setPosition(sf::Vector2f(16.f, 10.f));
    window.draw(title);

    // Tileset path input
    {
      // Layer selector (dropdown) + add/delete buttons
      const int layerBtnY = 34;
      const int btnW = 24;
      const int btnH = 22;
      const int gap = 4;
      const int selW = 160;
      const int startX = paletteWidth - 8 - (selW + 2 * gap + 2 * btnW);
      // Draw helper for a small square button
      auto drawBtn = [&](int x, const char* label) {
        sf::RectangleShape r(
            sf::Vector2f(static_cast<float>(btnW), static_cast<float>(btnH)));
        r.setPosition(
            sf::Vector2f(static_cast<float>(x), static_cast<float>(layerBtnY)));
        r.setFillColor(sf::Color(60, 60, 75));
        r.setOutlineThickness(1);
        r.setOutlineColor(sf::Color(90, 90, 110));
        window.draw(r);
        sf::Text t(font);
        t.setCharacterSize(16);
        t.setFillColor(sf::Color(230, 230, 240));
        t.setString(label);
        auto b = t.getLocalBounds();
        float tx =
            static_cast<float>(x) + (btnW - b.size.x) * 0.5f - b.position.x;
        float ty = static_cast<float>(layerBtnY) + (btnH - b.size.y) * 0.5f -
                   b.position.y - 2.f;
        t.setPosition(sf::Vector2f(tx, ty));
        window.draw(t);
      };
      int xSelect = startX;             // dropdown field
      int xAdd = xSelect + selW + gap;  // add button
      int xDel = xAdd + btnW + gap;     // delete button
      // Draw dropdown field
      {
        sf::RectangleShape selBox(
            sf::Vector2f(static_cast<float>(selW), static_cast<float>(btnH)));
        selBox.setPosition(sf::Vector2f(static_cast<float>(xSelect),
                                        static_cast<float>(layerBtnY)));
        selBox.setFillColor(sf::Color(60, 60, 75));
        selBox.setOutlineThickness(1);
        selBox.setOutlineColor(sf::Color(90, 90, 110));
        window.draw(selBox);
        // Text: current layer name
        sf::Text t(font);
        t.setCharacterSize(16);
        t.setFillColor(sf::Color(230, 230, 240));
        std::string curName =
            layers.empty() ? std::string("-") : layers[activeLayer].name;
        // truncate if needed
        t.setString(ellipsizeEnd(curName, 14u, static_cast<float>(selW - 20)));
        auto b = t.getLocalBounds();
        float tx = static_cast<float>(xSelect) + 6.f - b.position.x;
        float ty = static_cast<float>(layerBtnY) + (btnH - b.size.y) * 0.5f -
                   b.position.y - 2.f;
        t.setPosition(sf::Vector2f(tx, ty));
        window.draw(t);
        // Small ▼ indicator
        sf::Text caret(font);
        caret.setCharacterSize(14);
        caret.setFillColor(sf::Color(200, 200, 210));
        caret.setString("▼");
        auto cb = caret.getLocalBounds();
        caret.setPosition(sf::Vector2f(
            static_cast<float>(xSelect + selW - 12) - cb.position.x,
            static_cast<float>(layerBtnY + (btnH - cb.size.y) * 0.5f -
                               cb.position.y - 1)));
        window.draw(caret);
      }
      // Draw add/delete buttons
      drawBtn(xAdd, "+");
      drawBtn(xDel, "-");

      // Nota: la lista desplegable se dibuja más abajo para que quede por
      // encima del resto de la UI

      int pathInputW =
          paletteWidth - 24 - 100 - 6;  // input + gap + Load button
      // Input box
      sf::RectangleShape box(
          sf::Vector2f(static_cast<float>(pathInputW), 26.f));
      box.setFillColor(sf::Color(50, 50, 60));
      box.setOutlineThickness(1);
      box.setOutlineColor(enteringPath ? sf::Color(120, 160, 220)
                                       : sf::Color(90, 90, 110));
      box.setPosition(sf::Vector2f(12.f, 62.f));
      window.draw(box);

      sf::Text path(font);
      path.setCharacterSize(16);
      path.setFillColor(sf::Color::White);
      path.setPosition(sf::Vector2f(18.f, 64.f));
      path.setString(
          ellipsizeStart(pathBuffer, 16u, static_cast<float>(pathInputW - 12)));
      window.draw(path);

      // Load button
      sf::RectangleShape loadBtn(sf::Vector2f(100.f, 26.f));
      loadBtn.setFillColor(sf::Color(85, 120, 160));
      loadBtn.setOutlineThickness(1);
      loadBtn.setOutlineColor(sf::Color(90, 110, 140));
      loadBtn.setPosition(
          sf::Vector2f(12.f + static_cast<float>(pathInputW) + 6.f, 62.f));
      window.draw(loadBtn);
      sf::Text loadTxt(font);
      loadTxt.setCharacterSize(16);
      loadTxt.setFillColor(sf::Color(235, 240, 255));
      loadTxt.setPosition(sf::Vector2f(
          12.f + static_cast<float>(pathInputW) + 6.f + 24.f, 66.f));
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
      auto drawInput = [&](sf::Vector2f pos, const std::string& text,
                           bool focused) {
        sf::RectangleShape ibox(sf::Vector2f(52.f, 26.f));
        ibox.setFillColor(sf::Color(50, 50, 60));
        ibox.setOutlineThickness(1);
        ibox.setOutlineColor(focused ? sf::Color(120, 160, 220)
                                     : sf::Color(90, 90, 110));
        ibox.setPosition(pos);
        window.draw(ibox);
        sf::Text t(font);
        t.setCharacterSize(16);
        t.setFillColor(sf::Color::White);
        t.setPosition(pos + sf::Vector2f(6.f, 4.f));
        t.setString(text.empty() ? "" : text);
        window.draw(t);
      };
      drawInput(sf::Vector2f(12.f, static_cast<float>(cfgInputsY)), tileWBuf,
                enteringTileW);
      drawInput(sf::Vector2f(72.f, static_cast<float>(cfgInputsY)), tileHBuf,
                enteringTileH);
      // Apply button (moved below inputs to avoid overlap and stay within
      // palette)
      sf::RectangleShape applyBtn(sf::Vector2f(100.f, 26.f));
      applyBtn.setFillColor(sf::Color(85, 120, 160));
      applyBtn.setOutlineThickness(1);
      applyBtn.setOutlineColor(sf::Color(90, 110, 140));
      applyBtn.setPosition(sf::Vector2f(12.f, static_cast<float>(cfgButtonsY)));
      window.draw(applyBtn);
      sf::Text applyTxt(font);
      applyTxt.setCharacterSize(16);
      applyTxt.setFillColor(sf::Color(235, 240, 255));
      applyTxt.setPosition(
          sf::Vector2f(12.f + 12.f, static_cast<float>(cfgButtonsY) + 4.f));
      applyTxt.setString("Apply");
      window.draw(applyTxt);

      // Save folder controls
      sf::Text saveLabel(font);
      saveLabel.setCharacterSize(14);
      saveLabel.setFillColor(sf::Color(180, 180, 200));
      saveLabel.setPosition(sf::Vector2f(16.f, static_cast<float>(saveLabelY)));
      saveLabel.setString("Save folder:");
      window.draw(saveLabel);

      sf::RectangleShape saveBox(
          sf::Vector2f(static_cast<float>(paletteWidth - 24), 26.f));
      saveBox.setFillColor(sf::Color(50, 50, 60));
      saveBox.setOutlineThickness(1);
      saveBox.setOutlineColor(sf::Color(90, 90, 110));
      saveBox.setPosition(sf::Vector2f(12.f, static_cast<float>(saveInputY)));
      window.draw(saveBox);

      sf::Text savePathText(font);
      savePathText.setCharacterSize(14);
      savePathText.setFillColor(sf::Color::White);
      savePathText.setPosition(
          sf::Vector2f(18.f, static_cast<float>(saveInputY + 2)));
      savePathText.setString(ellipsizeStart(
          saveDirPath, 14u, static_cast<float>((paletteWidth - 24) - 12)));
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
      saveBtnText.setPosition(
          sf::Vector2f(12.f + 8.f, static_cast<float>(saveButtonsY) + 4.f));
      saveBtnText.setString("Save JSON");
      window.draw(saveBtnText);

      // Save As button
      sf::RectangleShape saveAsBtn(sf::Vector2f(140.f, 28.f));
      saveAsBtn.setFillColor(sf::Color(70, 90, 120));
      saveAsBtn.setOutlineThickness(1);
      saveAsBtn.setOutlineColor(sf::Color(90, 110, 140));
      saveAsBtn.setPosition(
          sf::Vector2f(12.f + 110.f, static_cast<float>(saveButtonsY)));
      window.draw(saveAsBtn);
      sf::Text saveAsBtnText(font);
      saveAsBtnText.setCharacterSize(16);
      saveAsBtnText.setFillColor(sf::Color(235, 240, 255));
      saveAsBtnText.setPosition(sf::Vector2f(
          12.f + 110.f + 8.f, static_cast<float>(saveButtonsY) + 4.f));
      saveAsBtnText.setString("Save As...");
      window.draw(saveAsBtnText);

      // Open Map... button (second row)
      {
        sf::RectangleShape openBtn(sf::Vector2f(230.f, 28.f));
        openBtn.setFillColor(sf::Color(90, 100, 140));
        openBtn.setOutlineThickness(1);
        openBtn.setOutlineColor(sf::Color(110, 120, 160));
        openBtn.setPosition(
            sf::Vector2f(12.f, static_cast<float>(saveButtonsY2)));
        window.draw(openBtn);
        sf::Text openTxt(font);
        openTxt.setCharacterSize(16);
        openTxt.setFillColor(sf::Color(235, 240, 255));
        openTxt.setPosition(
            sf::Vector2f(12.f + 8.f, static_cast<float>(saveButtonsY2) + 4.f));
        openTxt.setString("Open Map...");
        window.draw(openTxt);
      }

      // (Browse button removed)
    }

    // Draw palette thumbnails (still clipped by palette view)
    if (tileset.loaded) {
      const int colsPerRow =
          std::max(1, (paletteWidth - padding - x0) / (cell + padding));
      for (int r = 0; r < tileset.rows; ++r) {
        for (int c = 0; c < tileset.cols; ++c) {
          int i = r * tileset.cols + c;
          int tx = (i % colsPerRow);
          int ty = (i / colsPerRow);
          int px = x0 + tx * (cell + padding);
          int py = y0 + ty * (cell + padding);

          sf::Sprite spr(tileset.texture,
                         sf::IntRect({gsl::narrow_cast<int>(c * tileset.tileW),
                                      gsl::narrow_cast<int>(r * tileset.tileH)},
                                     {gsl::narrow_cast<int>(tileset.tileW),
                                      gsl::narrow_cast<int>(tileset.tileH)}));
          // Fit uniformly into cell and center
          float scale = std::min(
              static_cast<float>(cell) / static_cast<float>(tileset.tileW),
              static_cast<float>(cell) / static_cast<float>(tileset.tileH));
          float offX = (cell - tileset.tileW * scale) * 0.5f;
          float offY = (cell - tileset.tileH * scale) * 0.5f;
          spr.setPosition(sf::Vector2f(static_cast<float>(px) + offX,
                                       static_cast<float>(py) + offY));
          spr.setScale(sf::Vector2f(scale, scale));
          window.draw(spr);

          // Highlight selected
          if (selected.col == c && selected.row == r) {
            sf::RectangleShape sel(sf::Vector2f(static_cast<float>(cell),
                                                static_cast<float>(cell)));
            sel.setPosition(
                sf::Vector2f(static_cast<float>(px), static_cast<float>(py)));
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
        float viewTop =
            paletteView.getCenter().y - paletteView.getSize().y / 2.f;
        // Thumb size proportional to visible fraction
        float thumbH =
            std::max(24.f, trackH * (static_cast<float>(winH) / contentH));
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

    // Draw layer dropdown list last within palette view (so it appears on top
    // of palette UI)
    if (layerDropdownOpen) {
      const int layerBtnY = layerHeaderY();
      const int btnW = 24;
      const int btnH = 22;
      const int gap = 4;
      const int selW =
          std::max(120, paletteWidth - 8 - (2 * gap + 2 * btnW) - 140);
      const int startX = paletteWidth - 8 - (selW + 2 * gap + 2 * btnW);
      const int xSelect = startX;
      const int itemH = btnH;
      const int dropY = layerBtnY + btnH + 2;
      sf::RectangleShape listBg(sf::Vector2f(
          static_cast<float>(selW),
          static_cast<float>(itemH * gsl::narrow_cast<int>(layers.size()))));
      listBg.setPosition(
          sf::Vector2f(static_cast<float>(xSelect), static_cast<float>(dropY)));
      listBg.setFillColor(sf::Color(50, 50, 65));
      listBg.setOutlineThickness(1);
      listBg.setOutlineColor(sf::Color(90, 90, 110));
      window.draw(listBg);
      for (int i = 0; i < gsl::narrow_cast<int>(layers.size()); ++i) {
        if (i == activeLayer) {
          sf::RectangleShape hi(sf::Vector2f(static_cast<float>(selW),
                                             static_cast<float>(itemH)));
          hi.setPosition(sf::Vector2f(static_cast<float>(xSelect),
                                      static_cast<float>(dropY + i * itemH)));
          hi.setFillColor(sf::Color(70, 70, 95));
          window.draw(hi);
        }
        sf::Text it(font);
        it.setCharacterSize(16);
        it.setFillColor(sf::Color(230, 230, 240));
        it.setString(
            ellipsizeEnd(layers[i].name, 14u, static_cast<float>(selW - 10)));
        auto ib = it.getLocalBounds();
        float tx = static_cast<float>(xSelect) + 6.f - ib.position.x;
        float ty = static_cast<float>(dropY + i * itemH) +
                   (itemH - ib.size.y) * 0.5f - ib.position.y - 2.f;
        it.setPosition(sf::Vector2f(tx, ty));
        window.draw(it);
      }
    }

    // Switch back to default (full-window) view for the grid
    window.setView(defaultView);

    // Grid panel background (fixed to the right of palette)
    {
      float panelLeft = static_cast<float>(paletteWidth + margin);
      float panelTop = static_cast<float>(margin);
      float panelW =
          std::max(0.f, static_cast<float>(winW) -
                            (panelLeft + static_cast<float>(margin)));
      float panelH = std::max(
          0.f, static_cast<float>(winH) - (2.f * static_cast<float>(margin)));
      gridBG.setSize(sf::Vector2f(panelW, panelH));
      gridBG.setPosition(sf::Vector2f(panelLeft, panelTop));
      window.draw(gridBG);
    }

    // Grid lines (visible range only)
    {
      float cellPx = static_cast<float>(tilePx) * tileScale * gridZoom;
      float panelLeft = static_cast<float>(paletteWidth + margin);
      float panelTop = static_cast<float>(margin);
      float panelW =
          std::max(0.f, static_cast<float>(winW) -
                            (panelLeft + static_cast<float>(margin)));
      float panelH = std::max(
          0.f, static_cast<float>(winH) - (2.f * static_cast<float>(margin)));
      float panelRight = panelLeft + panelW;
      float panelBottom = panelTop + panelH;
      int minGX = gsl::narrow_cast<int>(
                      std::floor((panelLeft - gridOrigin.x) / cellPx)) -
                  1;
      int maxGX = gsl::narrow_cast<int>(
                      std::floor((panelRight - gridOrigin.x) / cellPx)) +
                  1;
      int minGY = gsl::narrow_cast<int>(
                      std::floor((panelTop - gridOrigin.y) / cellPx)) -
                  1;
      int maxGY = gsl::narrow_cast<int>(
                      std::floor((panelBottom - gridOrigin.y) / cellPx)) +
                  1;
      for (int gx = minGX; gx <= maxGX; ++gx) {
        float x = gridOrigin.x + gx * cellPx;
        if (x < panelLeft - 2.f || x > panelRight + 2.f) continue;
        sf::RectangleShape line(sf::Vector2f(1.f, panelH));
        line.setFillColor(sf::Color(45, 45, 55));
        line.setPosition(sf::Vector2f(x, panelTop));
        window.draw(line);
      }
      for (int gy = minGY; gy <= maxGY; ++gy) {
        float y = gridOrigin.y + gy * cellPx;
        if (y < panelTop - 2.f || y > panelBottom + 2.f) continue;
        sf::RectangleShape line(sf::Vector2f(panelW, 1.f));
        line.setFillColor(sf::Color(45, 45, 55));
        line.setPosition(sf::Vector2f(panelLeft, y));
        window.draw(line);
      }
    }

    // Draw placed tiles from all visible layers (bottom to top, visible range
    // only)
    if (!layers.empty()) {
      float cellPx = static_cast<float>(tilePx) * tileScale * gridZoom;
      float panelLeft = static_cast<float>(paletteWidth + margin);
      float panelTop = static_cast<float>(margin);
      float panelW =
          std::max(0.f, static_cast<float>(winW) -
                            (panelLeft + static_cast<float>(margin)));
      float panelH = std::max(
          0.f, static_cast<float>(winH) - (2.f * static_cast<float>(margin)));
      float panelRight = panelLeft + panelW;
      float panelBottom = panelTop + panelH;
      int minGX = gsl::narrow_cast<int>(
                      std::floor((panelLeft - gridOrigin.x) / cellPx)) -
                  1;
      int maxGX = gsl::narrow_cast<int>(
                      std::floor((panelRight - gridOrigin.x) / cellPx)) +
                  1;
      int minGY = gsl::narrow_cast<int>(
                      std::floor((panelTop - gridOrigin.y) / cellPx)) -
                  1;
      int maxGY = gsl::narrow_cast<int>(
                      std::floor((panelBottom - gridOrigin.y) / cellPx)) +
                  1;
      for (size_t li = 0; li < layers.size(); ++li) {
        int prev = activeLayer;
        activeLayer = gsl::narrow_cast<int>(li);
        if (!layers[li].visible || !tileset.loaded) {
          activeLayer = prev;
          continue;
        }
        for (int gy = minGY; gy <= maxGY; ++gy) {
          for (int gx = minGX; gx <= maxGX; ++gx) {
            TileCR t = getTileAt(gx, gy);
            if (t.col == 0 && t.row == 0) continue;
            sf::Sprite spr(
                tileset.texture,
                sf::IntRect({t.col * tileset.tileW, t.row * tileset.tileH},
                            {tileset.tileW, tileset.tileH}));
            float scaleX = (cellPx) / static_cast<float>(tileset.tileW);
            float scaleY = (cellPx) / static_cast<float>(tileset.tileH);
            spr.setScale(sf::Vector2f(scaleX, scaleY));
            spr.setPosition(sf::Vector2f(gridOrigin.x + gx * cellPx,
                                         gridOrigin.y + gy * cellPx));
            window.draw(spr);
          }
        }
        activeLayer = prev;
      }
    }

    // Hover highlight on grid (panel area)
    sf::Vector2i mp = sf::Mouse::getPosition(window);
    float cellPx = static_cast<float>(tilePx) * tileScale * gridZoom;
    float panelLeft = static_cast<float>(paletteWidth + margin);
    float panelTop = static_cast<float>(margin);
    float panelW = std::max(0.f, static_cast<float>(winW) -
                                     (panelLeft + static_cast<float>(margin)));
    float panelH = std::max(
        0.f, static_cast<float>(winH) - (2.f * static_cast<float>(margin)));
    sf::FloatRect panelRect({panelLeft, panelTop}, {panelW, panelH});
    if (panelRect.contains(
            sf::Vector2f(static_cast<float>(mp.x), static_cast<float>(mp.y)))) {
      int gx = gsl::narrow_cast<int>(
          std::floor((static_cast<float>(mp.x) - gridOrigin.x) / cellPx));
      int gy = gsl::narrow_cast<int>(
          std::floor((static_cast<float>(mp.y) - gridOrigin.y) / cellPx));
      sf::RectangleShape hover(sf::Vector2f(cellPx, cellPx));
      hover.setPosition(
          sf::Vector2f(gridOrigin.x + gx * cellPx, gridOrigin.y + gy * cellPx));
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
        float viewBottom =
            paletteView.getCenter().y + paletteView.getSize().y / 2.f;
        msg.setPosition(sf::Vector2f(16.f, viewBottom - 32.f));
        // Keep within left palette to avoid overlapping the grid
        msg.setString(ellipsizeEnd(infoMessage, 16u,
                                   static_cast<float>(paletteWidth - 32)));
        window.draw(msg);
      }
      // Restore default view for anything drawn afterwards (none here, but keep
      // tidy)
      window.setView(defaultView);
    }

    window.display();
  }

  return 0;
}

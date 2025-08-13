# BlackEngine - 2D Game Engine

A modern C++ game engine built with SFML, Box2D, and ImGui, featuring an Entity-Component System (ECS) architecture.

## 🎮 Features

### Core Engine
- **Entity-Component System (ECS)**: Flexible and modular game object architecture
- **2D Physics**: Box2D integration for realistic physics simulation
- **Graphics**: SFML-based rendering with sprite management
- **Audio**: SFML 3 audio (miniaudio backend, no OpenAL dependency)
- **Input System**: Keyboard and mouse input handling
- **Animation System**: JSON-based animation clips with frame-by-frame control
- **Debug UI**: ImGui integration for real-time debugging and development tools

### Components
- **TransformComponent**: Position, scale, and rotation management
- **SpriteComponent**: Texture and sprite rendering
- **RigidBodyComponent**: Physics body integration
- **AnimatorComponent**: Animation playback and management
- **AudioListenerComponent**: Audio playback and spatial audio
- **Movement**: Player movement with friction and speed control
- **FlipSprite**: Automatic sprite flipping based on movement direction

### GUI System
- **Button**: Interactive button components with texture support
- **TextObject**: Text rendering with custom fonts and styling

## 🏗️ Architecture

### Entity-Component System (ECS)
```
Entity
├── TransformComponent (Position, Scale, Rotation)
├── SpriteComponent (Texture, Sprite)
├── RigidBodyComponent (Physics Body)
├── AnimatorComponent (Animation Clips)
├── AudioListenerComponent (Audio Playback)
├── Movement (Player Controls)
└── FlipSprite (Sprite Orientation)
```

### Core Classes
- **Game**: Main game loop and window management
- **EntityManager**: Entity lifecycle and component management
- **ContactEventManager**: Collision detection and event handling
- **ImGuiManager**: Debug UI and development tools
- **TileGroup**: Tile-based level rendering system

## 🚀 Getting Started

### Prerequisites
- **Windows 10/11** or **macOS** (tested on macOS 12)
- **CMake** 3.16 or higher
- On macOS: **Homebrew** (for dependency management)

### Dependencies
```bash
# Install required packages via Homebrew (SFML 3 is fetched automatically)
brew install box2d jsoncpp
```

### Building the Project on Windows (MSVC + CMake)
```powershell
# From a Developer PowerShell / PowerShell
git clone https://github.com/jfcarocota/BlackEngineProject.git
cd BlackEngineProject

# Configure and build (generates Visual Studio solution and builds Release)
cmake -S . -B build
cmake --build build --config Release --parallel

# Run from distribution output
dist-win/bin/BlackEngineProject.exe
dist-win/bin/TileMapEditor.exe
```

Optional: package the app and assets into dist-win and a ZIP:
```powershell
cmake --build build --target package_app --config Release
```

### Building the Project on macOS (dev executable)
```bash
# Clone the repository
git clone https://github.com/jfcarocota/BlackEngineProject.git
cd BlackEngineProject

# Create build directory
mkdir cmake-build-debug
cd cmake-build-debug

# Configure and build
cmake ..
make

# Run the game from the build dir
./BlackEngineProject
```

### macOS App (.app) usage
- A self-contained `.app` is generated at the project root: `BlackEngineProject.app`
  - Binary at `Contents/MacOS/BlackEngineProject-bin`
  - Assets at `Contents/Resources/assets`
- You can double-click the app in Finder, or run:
```bash
open ./BlackEngineProject.app
```
- If Finder blocks the app (Gatekeeper), clear quarantine once:
```bash
xattr -dr com.apple.quarantine ./BlackEngineProject.app
```

### Packaging to dist/ (macOS)
- A distribution copy is produced via the custom target `package_app`:
```bash
# From build directory
make
# Outputs: dist/BlackEngineProject.app
```

## 📦 Project Structure
```
BlackEngineProject/
├── assets/                 # Game assets (copied into build and .app)
├── include/                # Header files
├── src/                    # Source files
├── third_party/            # External libraries
├── CMakeLists.txt          # Build configuration
├── BlackEngineProject.app  # macOS app bundle (dev output)
├── dist/                   # Packaged app for macOS distribution
└── dist-win/               # Packaged app for Windows (bin/, assets/, libs/include)
```

## 🎯 Usage

### Tile Map Editor
- Launch: `dist-win/bin/TileMapEditor.exe` (Windows) or from the build dir on macOS.
- Load tileset image:
  - Click "Load" to open a file dialog and pick an image (png/jpg/bmp/gif).
  - The editor previews the tiles in a scrollable palette.
- Configure tiles:
  - Only W and H are needed. Rows/Cols are computed automatically from the image size.
  - Click "Apply" to re-slice the tileset.
- Layers UI:
  - Layer dropdown at the top-left to select the active layer; buttons [+] and [-] to add/delete.
  - Visibility toggle via F3. Layers share the same tileset image/config.
- Paint the grid:
  - Left click to paint, right click to erase. Drag to paint continuously.
  - Zoom with mouse wheel over the grid; pan with middle mouse button. Zoom anchors under the cursor.
- Undo/Redo:
  - Undo: Ctrl+Z (Windows/Linux) or Cmd+Z (macOS)
  - Redo: Ctrl+Y or Ctrl+Shift+Z (Cmd+Shift+Z on macOS)
- Save/Open:
  - "Save JSON" crea un archivo `.json` con capas por defecto en:
    - Windows: `%APPDATA%/BlackEngineProject/Maps/map_YYYYMMDD_HHMMSS.json`
  - "Save As..." y "Open Map..." soportan únicamente `.json`.

Map file formats:
- JSON (default, supports layers):
  - Single layer
    ```json
    {
      "tileset": "assets/tiles.png",
      "tileW": 16, "tileH": 16,
      "grid": [ [ [c,r], ... ], ... ]
    }
    ```
  - Multi-layer
    ```json
    {
      "layers": [
        { "name": "Base", "tileset": "assets/tiles.png", "tileW": 16, "tileH": 16, "grid": [ ... ] },
        { "name": "Detail", "tileset": "assets/tiles.png", "tileW": 16, "tileH": 16, "grid": [ ... ] }
      ]
    }
    ```
  - Nota: la celda `[0,0]` se considera vacía (no se dibuja).

Notes:
- El editor usa fuentes del sistema cuando es posible; fallback a la Arcade incluida.

### Creating Game Objects
```cpp
// Create an entity
auto& hero = entityManager.AddEntity("hero");

// Add components
hero.AddComponent<TransformComponent>(500.f, 300.f, 16.f, 16.f, 4.f);
hero.AddComponent<SpriteComponent>(ASSETS_SPRITES, 0, 5);
hero.AddComponent<RigidBodyComponent>(world, b2BodyType::b2_dynamicBody, 1, 0, 0, 0.f, true, &hero);
hero.AddComponent<AnimatorComponent>();
hero.AddComponent<AudioListenerComponent>();
hero.AddComponent<Movement>(200.f, 0.28f, AudioClip("assets/audio/steps.ogg"));
hero.AddComponent<FlipSprite>();
```

### Animation System
```cpp
// Add animation to an entity
auto& animator = entity.AddComponent<AnimatorComponent>();
animator.AddAnimation("idle", AnimationClip("assets/animations/player/idle.json"));
animator.AddAnimation("walk", AnimationClip("assets/animations/player/walk.json"));

// Play animation
animator.PlayAnimation("walk");
```

## 🎮 Controls
- **WASD**: Move player character
- **ESC**: Close ImGui debug windows
 - Editor: F1/F2 para cambiar capa, F4 añadir capa, F5 eliminar capa

## 🔊 Audio
- SFML 3 audio uses miniaudio internally. No OpenAL or extra dylibs required.
- Audio works when running from Terminal or Finder.

## 🪟 Windows Notes
- Binaries and assets ready-to-run are placed under `dist-win/` after building.
- A ZIP package (e.g., `BlackEngineProject-<version>-Windows-AMD64.zip`) is produced in `build/` when using `package_app`.

## 🧰 Troubleshooting
- If the app shows missing assets when double-clicked: the app sets its working directory to `Contents/Resources`; rebuild if assets changed.
- If macOS blocks the app: remove quarantine (`xattr -dr com.apple.quarantine BlackEngineProject.app`).

### Runtime map loading (engine)
- Orden de selección al iniciar el juego:
  1) JSON de assets definidos en `Constants.hh` (por ejemplo `ASSETS_MAPS_JSON_THREE`) si existen.
  2) JSON más reciente en `assets/maps/`.
  3) Si no hay ninguno, el juego arrancará sin mapa.
- El motor parsea el JSON con jsoncpp y dibuja todas las capas en orden; `[0,0]` es celda vacía.

## 🔐 Code Signing & Notarization (optional)
CMake provides targets to sign and notarize the `.app`.

- Configure with identity and options:
```bash
cmake -B cmake-build-debug -S . \
  -DMAC_SIGN=ON \
  -DMAC_IDENTITY="Developer ID Application: Your Name (TEAMID)" \
  -DMAC_TEAM_ID=TEAMID \
  -DMAC_NOTARIZE=ON \
  -DAPPLE_ID="your@appleid.com" \
  -DAPPLE_ID_PASS="@keychain:AC_PASSWORD"
```
- Build, sign, notarize:
```bash
cmake --build cmake-build-debug --target sign_app
cmake --build cmake-build-debug --target notarize_app
```

## 🧩 Modern C++ Practices
- Uses [Microsoft GSL](https://github.com/microsoft/GSL) for safer pointer and array handling:
  - `gsl::not_null` ensures pointers are never null when dereferenced.
  - `gsl::span` is used for safe, flexible array/buffer access (see `EntityManager::GetEntities`).

### Example: Entity Access
```cpp
// Get all entities as a gsl::span
for (auto* entity : entityManager.GetEntities()) {
    // ...
}
// Get count
size_t count = entityManager.GetEntities().size();
```

## 📚 References
- SFML 3 repository and docs: [SFML/SFML](https://github.com/SFML/SFML)

## 📝 License
This project is open source. See LICENSE file for details.

## 🤝 Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

---

**BlackEngine** - Building games with modern C++ and ECS architecture! 🎮

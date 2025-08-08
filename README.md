# BlackEngine - 2D Game Engine

A modern C++ game engine built with SFML, Box2D, and ImGui, featuring an Entity-Component System (ECS) architecture.

## 🎮 Features

### Core Engine
- **Entity-Component System (ECS)**: Flexible and modular game object architecture
- **2D Physics**: Box2D integration for realistic physics simulation
- **Graphics**: SFML-based rendering with sprite management
- **Audio**: SFML audio system (OpenAL bundled inside the macOS app)
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
- **macOS** (tested on macOS 12)
- **CMake** 3.16 or higher
- **Homebrew** (for dependency management)

### Dependencies
```bash
# Install required packages via Homebrew
brew install sfml box2d jsoncpp
# (OpenAL is bundled in the .app; no need to install system-wide)
```

### Building the Project (dev executable)
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
  - OpenAL at `Contents/Frameworks/libopenal.1.dylib`
- You can double-click the app in Finder, or run:
```bash
open ./BlackEngineProject.app
```
- If Finder blocks the app (Gatekeeper), clear quarantine once:
```bash
xattr -dr com.apple.quarantine ./BlackEngineProject.app
```

### Packaging to dist/
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
│   ├── animations/         # Animation JSON files
│   ├── audio/              # Sound effects and music
│   ├── fonts/              # Font files
│   ├── GUI/                # UI textures
│   ├── maps/               # Level data
│   ├── sprites.png         # Sprite sheet
│   └── tiles.png           # Tile textures
├── include/                # Header files
├── src/                    # Source files
├── third_party/            # External libraries
├── CMakeLists.txt          # Build configuration
├── BlackEngineProject.app  # macOS app bundle (dev output)
└── dist/                   # Packaged app for distribution
```

## 🎯 Usage

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

## 🔊 Audio
- Uses SFML audio with OpenAL. The `.app` bundles `libopenal.1.dylib` so end-users do not need Homebrew.
- In development, audio also works when running the app from Finder or Terminal.

## 🧰 Troubleshooting
- If the app shows missing assets when double-clicked: the app now sets its working directory to `Contents/Resources`; assets are copied there. Rebuild if assets changed.
- If macOS blocks the app: remove quarantine (`xattr -dr com.apple.quarantine BlackEngineProject.app`).

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
This will sign with hardened runtime and entitlements, submit to Apple Notary Service, and staple the ticket.

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

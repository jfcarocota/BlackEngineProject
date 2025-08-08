# BlackEngine - 2D Game Engine

A modern C++ game engine built with SFML, Box2D, and ImGui, featuring an Entity-Component System (ECS) architecture.

## 🎮 Features

### Core Engine
- **Entity-Component System (ECS)**: Flexible and modular game object architecture
- **2D Physics**: Box2D integration for realistic physics simulation
- **Graphics**: SFML-based rendering with sprite management
- **Audio**: SFML audio system with spatial audio support
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
- **macOS** (tested on macOS 21.6.0)
- **CMake** 3.16 or higher
- **Homebrew** (for dependency management)

### Dependencies
```bash
# Install required packages via Homebrew
brew install sfml box2d jsoncpp
```

### Building the Project
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

# Run the game
./BlackEngineProject
```

### Project Structure
```
BlackEngineProject/
├── assets/                 # Game assets
│   ├── animations/         # Animation JSON files
│   ├── audio/             # Sound effects and music
│   ├── fonts/             # Font files
│   ├── GUI/               # UI textures
│   ├── maps/              # Level data
│   ├── sprites.png        # Sprite sheet
│   └── tiles.png          # Tile textures
├── include/               # Header files
│   ├── Components/        # ECS component headers
│   ├── GUI/               # GUI system headers
│   └── *.hh               # Core engine headers
├── src/                   # Source files
│   ├── Components/        # ECS component implementations
│   ├── GUI/               # GUI system implementations
│   └── *.cc               # Core engine implementations
├── third_party/           # External libraries
│   └── imgui/             # Dear ImGui library
├── CMakeLists.txt         # Build configuration
└── README.md              # This file
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

### Physics Integration
```cpp
// Create physics body
auto& rigidBody = entity.AddComponent<RigidBodyComponent>(
    world,                    // Box2D world
    b2BodyType::b2_dynamicBody, // Body type
    1,                        // Density
    0,                        // Friction
    0,                        // Restitution
    0.f,                      // Angle
    true,                     // Fixed rotation
    &entity                   // User data
);
```

### Collision Detection
```cpp
// Collision events are automatically handled by ContactEventManager
// Custom collision responses can be added in ContactEventManager.cc
```

## 🎮 Controls

### Game Controls
- **WASD**: Move player character
- **Mouse**: Click UI elements
- **ESC**: Close ImGui debug windows

### Debug Controls
- **F3**: Toggle ImGui test window
- **Debug Physics Button**: Toggle physics visualization
- **ImGui Windows**: Various debug information panels

## 🔧 Configuration

### Game Constants
All game constants are defined in `include/Constants.hh`:

```cpp
namespace GameConstants {
    constexpr float PLAYER_SPEED = 200.0f;
    constexpr float PLAYER_FRICTION = 0.28f;
    constexpr int PHYSICS_VELOCITY_ITERATIONS = 8;
    constexpr int PHYSICS_POSITION_ITERATIONS = 8;
    constexpr float TILE_SIZE = 16.0f;
    constexpr float TILE_SCALE = 4.0f;
    constexpr int MAP_WIDTH = 12;
    constexpr int MAP_HEIGHT = 12;
}
```

### Window Configuration
```cpp
const unsigned int WINDOW_WIDTH{760};
const unsigned int WINDOW_HEIGHT{760};
const char* GAME_NAME{"Game1"};
```

## 🎨 Asset System

### Supported Asset Types
- **Sprites**: PNG format with sprite sheet support
- **Audio**: OGG format for sound effects and music
- **Fonts**: TTF format for text rendering
- **Animations**: JSON format with frame data
- **Maps**: Grid-based level data

### Asset Loading
Assets are automatically copied to the build directory during compilation. All asset paths are relative to the executable location.

## 🐛 Debugging

### ImGui Integration
The engine includes a comprehensive debug UI system:
- **Demo Window**: ImGui demo and examples
- **Debug Info**: Real-time game state information
- **Entity Info**: Component and entity details
- **Test Window**: Custom debug interface

### Physics Debugging
- **Debug Draw**: Visualize physics bodies and shapes
- **Collision Detection**: Real-time collision event logging
- **Performance Metrics**: Frame rate and physics step timing

## 🔄 Game Loop

The engine follows a standard game loop pattern:

1. **Event Processing**: Handle input and window events
2. **Physics Update**: Update Box2D physics simulation
3. **Game Logic**: Update entities and components
4. **Rendering**: Draw all game objects and UI
5. **Frame Limiting**: Maintain consistent frame rate

## 🚧 Development

### Adding New Components
1. Create header file in `include/Components/`
2. Inherit from `Component` base class
3. Implement required virtual methods
4. Add to CMakeLists.txt build configuration

### Adding New Systems
1. Create system class in `src/`
2. Integrate with EntityManager
3. Add to Game class initialization
4. Update build configuration

### Code Style
- **Naming**: PascalCase for classes, camelCase for methods
- **Includes**: Use `#include <...>` for standard library, `#include "..."` for project files
- **Memory**: Use smart pointers (`std::unique_ptr`) for ownership
- **Error Handling**: Check return values and log errors appropriately

## 📝 License

This project is open source. See LICENSE file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📞 Support

For issues and questions:
- Create an issue on GitHub
- Check the documentation
- Review the code examples

---

**BlackEngine** - Building games with modern C++ and ECS architecture! 🎮

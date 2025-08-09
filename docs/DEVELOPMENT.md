# BlackEngine Development Guide

## Table of Contents
1. [Development Setup](#development-setup)
2. [Code Style Guidelines](#code-style-guidelines)
3. [Adding New Components](#adding-new-components)
4. [Adding New Systems](#adding-new-systems)
5. [Asset Pipeline](#asset-pipeline)
6. [Testing and Debugging](#testing-and-debugging)
7. [Performance Optimization](#performance-optimization)
8. [Contributing Guidelines](#contributing-guidelines)

## Development Setup

### Prerequisites
- **macOS** (primary development platform)
- **CMake** 3.16+
- **Homebrew** for dependency management
- **Git** for version control
- **CLion** or **VSCode** (recommended IDEs)

### Initial Setup
```bash
# Clone the repository
git clone https://github.com/jfcarocota/BlackEngineProject.git
cd BlackEngineProject

# Install dependencies
brew install sfml box2d jsoncpp

# Create build directory
mkdir cmake-build-debug
cd cmake-build-debug

# Configure project
cmake ..

# Build project
make
```

### IDE Configuration

#### CLion
1. Open the project in CLion
2. Set CMake build directory to `cmake-build-debug`
3. Configure run configuration to use `./BlackEngineProject`
4. Set working directory to `cmake-build-debug`

#### VSCode
1. Install C/C++ extension
2. Install CMake Tools extension
3. Configure `c_cpp_properties.json` for include paths
4. Set up build tasks in `tasks.json`

## Code Style Guidelines

### Naming Conventions
```cpp
// Classes: PascalCase
class TransformComponent : public Component { };

// Methods: camelCase
void setPosition(sf::Vector2f position);
float getWidth() const;

// Variables: camelCase
float deltaTime;
sf::Vector2f playerPosition;

// Constants: UPPER_SNAKE_CASE
constexpr float PLAYER_SPEED = 200.0f;
const char* ASSETS_SPRITES = "assets/sprites.png";

// Namespaces: PascalCase
namespace GameConstants { }
```

### File Organization
```
include/
├── Components/
│   ├── Component.hh          # Base component class
│   ├── TransformComponent.hh
│   └── SpriteComponent.hh
├── GUI/
│   ├── Button.hh
│   └── TextObject.hh
└── Core/
    ├── Game.hh
    └── EntityManager.hh

src/
├── Components/
│   ├── TransformComponent.cc
│   └── SpriteComponent.cc
├── GUI/
│   ├── Button.cc
│   └── TextObject.cc
└── Core/
    ├── Game.cc
    └── EntityManager.cc
```

### Include Guidelines
```cpp
// Standard library includes first
#include <vector>
#include <memory>
#include <iostream>

// Third-party includes
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

// Project includes (relative to include directory)
#include "Components/Component.hh"
#include "Game.hh"
```

### Memory Management
```cpp
// Use smart pointers for ownership
std::unique_ptr<Game> game;
std::shared_ptr<Texture> sharedTexture;

// Avoid raw pointers for ownership
// ❌ Bad
Game* game = new Game();

// ✅ Good
auto game = std::make_unique<Game>();
```

## Adding New Components

### Step 1: Create Header File
```cpp
// include/Components/HealthComponent.hh
#pragma once
#include "Component.hh"
#include <SFML/Graphics.hpp>

class HealthComponent : public Component {
private:
    float maxHealth;
    float currentHealth;
    bool isAlive;

public:
    HealthComponent(float maxHealth);
    ~HealthComponent() override;
    
    void Initialize() override;
    void Update(float& deltaTime) override;
    void Render(sf::RenderWindow& window) override;
    
    // Health management
    void TakeDamage(float damage);
    void Heal(float amount);
    float GetHealthPercent() const;
    bool IsAlive() const;
};
```

### Step 2: Create Implementation File
```cpp
// src/Components/HealthComponent.cc
#include "Components/HealthComponent.hh"
#include "Components/Entity.hh"
#include <iostream>

HealthComponent::HealthComponent(float maxHealth) 
    : maxHealth(maxHealth), currentHealth(maxHealth), isAlive(true) {
}

HealthComponent::~HealthComponent() = default;

void HealthComponent::Initialize() {
    // Initialize component
}

void HealthComponent::Update(float& deltaTime) {
    // Update logic
    if (currentHealth <= 0 && isAlive) {
        isAlive = false;
        // Handle death
    }
}

void HealthComponent::Render(sf::RenderWindow& window) {
    // Render health bar or other visual elements
}

void HealthComponent::TakeDamage(float damage) {
    currentHealth = std::max(0.0f, currentHealth - damage);
}

void HealthComponent::Heal(float amount) {
    currentHealth = std::min(maxHealth, currentHealth + amount);
}

float HealthComponent::GetHealthPercent() const {
    return currentHealth / maxHealth;
}

bool HealthComponent::IsAlive() const {
    return isAlive;
}
```

### Step 3: Update Build Configuration
```cmake
# CMakeLists.txt
add_executable(BlackEngineProject
  # ... existing files ...
  src/Components/HealthComponent.cc
)
```

### Step 4: Usage Example
```cpp
// In Game.cc
auto& player = entityManager.AddEntity("player");
player.AddComponent<HealthComponent>(100.0f);

// Access component
if (auto* health = player.GetComponent<HealthComponent>()) {
    health->TakeDamage(20.0f);
}
```

## Adding New Systems

### Step 1: Create System Class
```cpp
// include/Systems/CombatSystem.hh
#pragma once
#include "Components/EntityManager.hh"
#include "Components/HealthComponent.hh"
#include <vector>

class CombatSystem {
private:
    EntityManager& entityManager;
    std::vector<std::pair<Entity*, Entity*>> combatPairs;

public:
    explicit CombatSystem(EntityManager& manager);
    ~CombatSystem();
    
    void Update(float deltaTime);
    void ProcessCombat();
    void AddCombatPair(Entity* attacker, Entity* target);
};
```

### Step 2: Implement System
```cpp
// src/Systems/CombatSystem.cc
#include "Systems/CombatSystem.hh"
#include "Components/HealthComponent.hh"

CombatSystem::CombatSystem(EntityManager& manager) 
    : entityManager(manager) {
}

CombatSystem::~CombatSystem() = default;

void CombatSystem::Update(float deltaTime) {
    ProcessCombat();
}

void CombatSystem::ProcessCombat() {
    for (auto& [attacker, target] : combatPairs) {
        if (attacker && target) {
            auto* attackerHealth = attacker->GetComponent<HealthComponent>();
            auto* targetHealth = target->GetComponent<HealthComponent>();
            
            if (attackerHealth && targetHealth) {
                // Process combat logic
                targetHealth->TakeDamage(10.0f);
            }
        }
    }
    combatPairs.clear();
}

void CombatSystem::AddCombatPair(Entity* attacker, Entity* target) {
    combatPairs.emplace_back(attacker, target);
}
```

### Step 3: Integrate with Game Class
```cpp
// include/Game.hh
class Game {
private:
    // ... existing members ...
    std::unique_ptr<CombatSystem> combatSystem;
    
public:
    // ... existing methods ...
};
```

```cpp
// src/Game.cc
Game::Game() {
    // ... existing initialization ...
    combatSystem = std::make_unique<CombatSystem>(entityManager);
}

void Game::Update() {
    // ... existing update logic ...
    combatSystem->Update(deltaTime);
}
```

## Asset Pipeline

### Sprite Sheet Guidelines
- **Format**: PNG with transparency
- **Grid**: Power of 2 tile sizes (16x16, 32x32, 64x64)
- **Naming**: `sprites.png`, `tiles.png`, `ui.png`

### Animation JSON Format
```json
{
  "name": "player_walk",
  "frameWidth": 16,
  "frameHeight": 16,
  "frameRate": 8,
  "frames": [
    {"x": 0, "y": 0, "duration": 0.125},
    {"x": 16, "y": 0, "duration": 0.125},
    {"x": 32, "y": 0, "duration": 0.125},
    {"x": 48, "y": 0, "duration": 0.125}
  ]
}
```

### Audio Guidelines
- **Format**: OGG for best compression
- **Sample Rate**: 44.1kHz
- **Channels**: Mono for effects, Stereo for music
- **Naming**: `steps.ogg`, `jump.ogg`, `background_music.ogg`

### Asset Loading Best Practices
```cpp
// Centralized asset management
class AssetManager {
private:
    std::unordered_map<std::string, sf::Texture> textures;
    std::unordered_map<std::string, sf::SoundBuffer> sounds;
    
public:
    sf::Texture& LoadTexture(const std::string& path);
    sf::SoundBuffer& LoadSound(const std::string& path);
    void PreloadAssets();
};
```

## Testing and Debugging

### Debug Output
```cpp
// Use consistent debug output
#ifdef DEBUG
    std::cout << "[DEBUG] Entity created: " << entityName << std::endl;
#endif

// Error logging
std::cerr << "[ERROR] Failed to load texture: " << path << std::endl;
```

### ImGui Debug Windows
```cpp
void ImGuiManager::ShowDebugInfo() {
    if (ImGui::Begin("Debug Info")) {
        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("Entities: %zu", entityManager.GetEntities().size()); // .size() works with gsl::span
        ImGui::Text("Delta Time: %.3f", deltaTime);
        
        if (ImGui::Button("Reset Game")) {
            // Reset game state
        }
    }
    ImGui::End();
}
```

### Physics Debugging
```cpp
// Enable physics debug drawing
flags += b2Draw::e_shapeBit;
flags += b2Draw::e_aabbBit;
world->SetDebugDraw(drawPhysics);
drawPhysics->SetFlags(flags);

// In render loop
if (debugPhysics) {
    world->DebugDraw();
}
```

### Performance Profiling
```cpp
// Simple frame time measurement
auto frameStart = std::chrono::high_resolution_clock::now();

// ... game logic ...

auto frameEnd = std::chrono::high_resolution_clock::now();
auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
std::cout << "Frame time: " << frameTime.count() << "μs" << std::endl;
```

## Performance Optimization

### Entity Management
```cpp
// Use object pools for frequently created/destroyed entities
class EntityPool {
private:
    std::vector<std::unique_ptr<Entity>> pool;
    std::vector<Entity*> available;
    
public:
    Entity* GetEntity();
    void ReturnEntity(Entity* entity);
};
```

### Component Access Optimization
```cpp
// Cache component pointers in Update methods
void SomeComponent::Update(float& deltaTime) {
    if (!transform) {
        transform = owner->GetComponent<TransformComponent>();
    }
    
    if (transform) {
        // Use cached pointer
        transform->SetPosition(newPosition);
    }
}
```

### Rendering Optimization
```cpp
// Batch similar sprites
class SpriteBatch {
private:
    std::vector<sf::Sprite> sprites;
    sf::Texture* texture;
    
public:
    void AddSprite(const sf::Sprite& sprite);
    void Render(sf::RenderWindow& window);
};
```

### Memory Optimization
```cpp
// Use move semantics
std::vector<Entity*> entities = std::move(newEntities);

// Reserve vector capacity
entities.reserve(expectedSize);

// Use references where possible
void UpdateEntity(Entity& entity);
```

## Contributing Guidelines

### Pull Request Process
1. **Fork** the repository
2. **Create** a feature branch: `git checkout -b feature/new-component`
3. **Make** your changes following the code style guidelines
4. **Test** your changes thoroughly
5. **Commit** with descriptive messages: `git commit -m "Add HealthComponent with damage system"`
6. **Push** to your fork: `git push origin feature/new-component`
7. **Create** a pull request with detailed description

### Commit Message Format
```
type(scope): description

[optional body]

[optional footer]
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Maintenance tasks

**Examples:**
```
feat(components): add HealthComponent with damage system
fix(physics): resolve collision detection edge case
docs(api): update component documentation
style(core): fix include style consistency
```

### Code Review Checklist
- [ ] Code follows style guidelines
- [ ] No memory leaks or undefined behavior
- [ ] Proper error handling implemented
- [ ] Documentation updated
- [ ] Tests added (if applicable)
- [ ] Performance impact considered
- [ ] Backward compatibility maintained

### Testing Requirements
```cpp
// Add unit tests for new components
#include <gtest/gtest.h>

TEST(HealthComponentTest, TakeDamage) {
    HealthComponent health(100.0f);
    health.TakeDamage(30.0f);
    EXPECT_EQ(health.GetHealthPercent(), 0.7f);
}

TEST(HealthComponentTest, Death) {
    HealthComponent health(100.0f);
    health.TakeDamage(100.0f);
    EXPECT_FALSE(health.IsAlive());
}
```

## Common Patterns

### Singleton Pattern (Use Sparingly)
```cpp
class AssetManager {
private:
    static AssetManager* instance;
    AssetManager() = default;
    
public:
    static AssetManager* GetInstance();
    static void DestroyInstance();
};
```

### Observer Pattern
```cpp
class EventSystem {
private:
    std::unordered_map<std::string, std::vector<std::function<void()>>> listeners;
    
public:
    void Subscribe(const std::string& event, std::function<void()> callback);
    void Emit(const std::string& event);
};
```

### Factory Pattern
```cpp
class EntityFactory {
public:
    static Entity& CreatePlayer(EntityManager& manager, float x, float y);
    static Entity& CreateEnemy(EntityManager& manager, float x, float y);
    static Entity& CreateItem(EntityManager& manager, float x, float y);
};
```

---

This development guide provides comprehensive information for extending and contributing to the BlackEngine. Follow these guidelines to maintain code quality and consistency across the project.

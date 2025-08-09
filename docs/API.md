# BlackEngine API Documentation

## Table of Contents
1. [Core Classes](#core-classes)
2. [Component System](#component-system)
3. [GUI System](#gui-system)
4. [Utility Classes](#utility-classes)
5. [Constants and Configuration](#constants-and-configuration)

## Core Classes

### Game Class
The main game class that manages the game loop, window, and core systems.

#### Constructor
```cpp
Game::Game()
```
Initializes the game window, physics world, and creates initial entities.

#### Public Methods
```cpp
void Initialize()
```
Sets up the game systems and starts the main loop.

#### Private Methods
```cpp
void Update()
void Render()
void MainLoop()
void Destroy()
void UpdatePhysics()
```

### EntityManager Class
Manages entity lifecycle and component operations.

#### Constructor
```cpp
EntityManager::EntityManager()
```

#### Public Methods
```cpp
Entity& AddEntity(std::string entityName)
```
Creates and returns a new entity with the given name.

```cpp
void Update(float& deltaTime)
```
Updates all active entities and manages entity lifecycle.

```cpp
void Render(sf::RenderWindow& window)
```
Renders all active entities.

```cpp
void ClearData()
```
Destroys all entities and cleans up memory.

```cpp
bool HasNoEntities()
```
Returns true if no entities exist.

```cpp
gsl::span<Entity*> GetEntities() const
```
Returns a span of pointers to all entities managed by the EntityManager. Use range-based for or `.size()` for iteration and count.

```cpp
unsigned int GetentityCount() const
```
Returns the total number of entities.

### Entity Class
Represents a game object that can have multiple components.

#### Constructor
```cpp
Entity::Entity(EntityManager& manager, std::string name)
```

#### Public Methods
```cpp
template<typename T, typename... Args>
T& AddComponent(Args&&... args)
```
Adds a component of type T to the entity.

```cpp
template<typename T>
T* GetComponent()
```
Returns a pointer to the component of type T, or nullptr if not found.

```cpp
template<typename T>
bool HasComponent()
```
Returns true if the entity has a component of type T.

```cpp
void Update(float deltaTime)
```
Updates all components of the entity.

```cpp
void Render(sf::RenderWindow& window)
```
Renders all renderable components.

```cpp
void Destroy()
```
Marks the entity for destruction.

```cpp
bool IsActive() const
```
Returns true if the entity is active.

```cpp
std::string GetName() const
```
Returns the entity's name.

## Component System

### Component Base Class
Base class for all components in the ECS system.

#### Public Members
```cpp
Entity* owner;
```

#### Virtual Methods
```cpp
virtual ~Component()
virtual void Initialize()
virtual void Update(float& deltaTime)
virtual void Render(sf::RenderWindow& window)
```

### TransformComponent
Manages position, scale, and rotation of entities.

#### Constructor
```cpp
TransformComponent(float posX, float posY, float width, float height, float scale)
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
float GetWidth() const
float GetHeight() const
float GetScale() const
sf::Vector2f GetPosition() const
void SetPosition(sf::Vector2f position)
void SetWidth(float width)
void SetHeight(float height)
void SetScale(float scale)
void Translate(sf::Vector2f direction)
```

### SpriteComponent
Handles texture loading and sprite rendering.

#### Constructor
```cpp
SpriteComponent(const char* textureUrl, unsigned int col, unsigned int row)
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
void Render(sf::RenderWindow& window) override
void SetFlipTexture(bool flip)
bool GetFlipTexture() const
sf::Vector2f GetOrigin() const
void RebindRectTexture(int col, int row, float width, float height)
```

### RigidBodyComponent
Integrates entities with Box2D physics.

#### Constructor
```cpp
RigidBodyComponent(b2World* world, b2BodyType bodyType, float density, 
                   float friction, float restitution, float angle, 
                   bool fixedRotation, void* userData)
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
b2Body* GetBody() const
b2Vec2 GetBodyPosition() const
void SetBodyPosition(b2Vec2 position)
```

### AnimatorComponent
Manages animation playback and frame timing.

#### Constructor
```cpp
AnimatorComponent()
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
void AddAnimation(std::string name, AnimationClip clip)
void PlayAnimation(std::string name)
void StopAnimation()
bool IsPlaying() const
std::string GetCurrentAnimation() const
```

### AudioListenerComponent
Handles audio playback and spatial audio.

#### Constructor
```cpp
AudioListenerComponent()
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
void PlaySound(const AudioClip& clip)
void StopSound()
bool IsPlaying() const
void SetVolume(float volume)
float GetVolume() const
```

### Movement Component
Handles player movement with physics integration.

#### Constructor
```cpp
Movement(float speed, float friction, AudioClip stepSound)
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
void SetSpeed(float speed)
float GetSpeed() const
void SetFriction(float friction)
float GetFriction() const
```

### FlipSprite Component
Automatically flips sprites based on movement direction.

#### Constructor
```cpp
FlipSprite()
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
```

## GUI System

### Button Class
Interactive button component with texture support.

#### Constructor
```cpp
Button(TransformComponent& transform, float borderThickness, 
       sf::Color fillColor, sf::Color outlineColor, 
       std::function<void()> onClick)
```

#### Public Methods
```cpp
void Initialize() override
void Update(float& deltaTime) override
void Render(sf::RenderWindow& window) override
void SetTexture(std::string texturePath)
void SetText(std::string text)
void SetOnClick(std::function<void()> onClick)
bool IsHovered() const
bool IsClicked() const
```

### TextObject Class
Text rendering with custom fonts and styling.

#### Constructor
```cpp
TextObject(const char* fontPath, unsigned int fontSize, 
           sf::Color color, sf::Text::Style style)
```

#### Public Methods
```cpp
void SetTextStr(std::string text)
void SetFontSize(unsigned int size)
void SetColor(sf::Color color)
void SetPosition(sf::Vector2f position)
sf::Text* GetText() const
sf::FloatRect GetBounds() const
```

## Utility Classes

### AnimationClip Class
Represents an animation sequence with frame data.

#### Constructor
```cpp
AnimationClip(const char* jsonPath)
```

#### Public Methods
```cpp
void LoadFromJson(const char* jsonPath)
std::vector<AnimationFrame> GetFrames() const
int GetFrameCount() const
float GetDuration() const
AnimationFrame GetFrameAtTime(float time) const
```

### AudioClip Class
Represents an audio file for playback.

#### Constructor
```cpp
AudioClip(const char* audioPath)
```

#### Public Methods
```cpp
bool LoadFromFile(const char* audioPath)
sf::SoundBuffer* GetBuffer() const
float GetDuration() const
bool IsLoaded() const
```

### TileGroup Class
Manages tile-based level rendering.

#### Constructor
```cpp
TileGroup(sf::RenderWindow* window, int mapWidth, int mapHeight, 
          const char* mapPath, float tileScale, int tileWidth, 
          int tileHeight, const char* tilesetPath)
```

#### Public Methods
```cpp
void LoadMap(const char* mapPath)
void LoadTileset(const char* tilesetPath)
void Draw()
void SetTileScale(float scale)
float GetTileScale() const
```

### ContactEventManager Class
Handles collision detection and event dispatching.

#### Constructor
```cpp
ContactEventManager()
```

#### Public Methods
```cpp
void BeginContact(b2Contact* contact) override
void EndContact(b2Contact* contact) override
void AddCollisionHandler(std::string entityA, std::string entityB, 
                        std::function<void(Entity*, Entity*)> handler)
```

### ImGuiManager Class
Manages ImGui debug interface and development tools.

#### Constructor
```cpp
ImGuiManager()
```

#### Public Methods
```cpp
void Initialize(sf::RenderWindow& window)
void ProcessEvent(const sf::Event& event)
void Update(sf::RenderWindow& window, sf::Time deltaTime)
void Render(sf::RenderWindow& window)
void Shutdown()
void ShowMainMenuBar()
void ShowDemoWindow()
void ShowDebugInfo()
void ShowEntityInfo()
void ShowTestWindow()
void RenderTestIndicator(sf::RenderWindow& window)
```

## Constants and Configuration

### Game Constants
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

### Asset Paths
```cpp
const char* ASSETS_SPRITES{"assets/sprites.png"};
const char* ASSETS_TILES{"assets/tiles.png"};
const char* ASSETS_MAPS{"assets/maps/level1.grid"};
const char* ASSETS_FONT_ARCADECLASSIC{"assets/fonts/ARCADECLASSIC.ttf"};
```

## Usage Examples

### Creating a Simple Entity
```cpp
// Create entity
auto& player = entityManager.AddEntity("player");

// Add transform component
player.AddComponent<TransformComponent>(100.f, 100.f, 32.f, 32.f, 2.f);

// Add sprite component
player.AddComponent<SpriteComponent>(ASSETS_SPRITES, 0, 0);

// Add physics
player.AddComponent<RigidBodyComponent>(world, b2BodyType::b2_dynamicBody, 
                                       1, 0, 0, 0.f, true, &player);
```

### Creating an Animated Entity
```cpp
// Create animated entity
auto& enemy = entityManager.AddEntity("enemy");

// Add components
enemy.AddComponent<TransformComponent>(200.f, 200.f, 32.f, 32.f, 2.f);
enemy.AddComponent<SpriteComponent>(ASSETS_SPRITES, 1, 0);

// Add animation
auto& animator = enemy.AddComponent<AnimatorComponent>();
animator.AddAnimation("idle", AnimationClip("assets/animations/enemy/idle.json"));
animator.AddAnimation("walk", AnimationClip("assets/animations/enemy/walk.json"));

// Play animation
animator.PlayAnimation("idle");
```

### Creating Interactive UI
```cpp
// Create button entity
auto& button = entityManager.AddEntity("button");

// Add transform
auto& buttonTransform = button.AddComponent<TransformComponent>(50.f, 50.f, 100.f, 50.f, 1.f);

// Add button component
auto& buttonComp = button.AddComponent<Button>(buttonTransform, 2.f, 
                                              sf::Color::White, sf::Color::Black,
                                              []() { std::cout << "Button clicked!" << std::endl; });

// Set button texture
buttonComp.SetTexture("assets/GUI/button.png");
```

### Handling Collisions
```cpp
// In ContactEventManager.cc
void ContactEventManager::BeginContact(b2Contact* contact) {
    Entity* entityA = static_cast<Entity*>(contact->GetFixtureA()->GetBody()->GetUserData());
    Entity* entityB = static_cast<Entity*>(contact->GetFixtureB()->GetBody()->GetUserData());
    
    if (entityA && entityB) {
        if (entityA->GetName() == "player" && entityB->GetName() == "enemy") {
            // Handle player-enemy collision
            std::cout << "Player hit enemy!" << std::endl;
        }
    }
}
```

## Error Handling

### Component Access
```cpp
// Safe component access
if (auto* sprite = entity.GetComponent<SpriteComponent>()) {
    sprite->SetFlipTexture(true);
} else {
    std::cerr << "Entity missing SpriteComponent" << std::endl;
}
```

### Asset Loading
```cpp
// Check if assets loaded successfully
if (!texture.loadFromFile(texturePath)) {
    std::cerr << "Failed to load texture: " << texturePath << std::endl;
    // Handle error appropriately
}
```

### Physics Validation
```cpp
// Validate physics body
if (body && body->IsActive()) {
    body->SetLinearVelocity(b2Vec2(0, 0));
} else {
    std::cerr << "Invalid or inactive physics body" << std::endl;
}
```

---

This API documentation provides a comprehensive reference for all classes and components in the BlackEngine. For more detailed examples and tutorials, see the main README.md file.

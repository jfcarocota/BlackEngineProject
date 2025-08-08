# BlackEngine Quick Reference

## 🚀 Quick Start

### Basic Entity Creation
```cpp
// Create entity
auto& entity = entityManager.AddEntity("myEntity");

// Add components
entity.AddComponent<TransformComponent>(x, y, width, height, scale);
entity.AddComponent<SpriteComponent>(texturePath, col, row);
entity.AddComponent<RigidBodyComponent>(world, bodyType, density, friction, restitution, angle, fixedRotation, &entity);
```

### Player Entity Template
```cpp
auto& player = entityManager.AddEntity("player");
player.AddComponent<TransformComponent>(500.f, 300.f, 16.f, 16.f, 4.f);
player.AddComponent<SpriteComponent>(ASSETS_SPRITES, 0, 5);
player.AddComponent<RigidBodyComponent>(world, b2BodyType::b2_dynamicBody, 1, 0, 0, 0.f, true, &player);
player.AddComponent<AnimatorComponent>();
player.AddComponent<AudioListenerComponent>();
player.AddComponent<Movement>(GameConstants::PLAYER_SPEED, GameConstants::PLAYER_FRICTION, AudioClip("assets/audio/steps.ogg"));
player.AddComponent<FlipSprite>();
```

## 🎮 Component Reference

### TransformComponent
```cpp
// Create
auto& transform = entity.AddComponent<TransformComponent>(x, y, width, height, scale);

// Access
auto* transform = entity.GetComponent<TransformComponent>();
if (transform) {
    transform->SetPosition(sf::Vector2f(100.f, 100.f));
    transform->SetScale(2.0f);
    sf::Vector2f pos = transform->GetPosition();
}
```

### SpriteComponent
```cpp
// Create
auto& sprite = entity.AddComponent<SpriteComponent>("assets/sprites.png", 0, 0);

// Access
auto* sprite = entity.GetComponent<SpriteComponent>();
if (sprite) {
    sprite->SetFlipTexture(true);
    sprite->RebindRectTexture(1, 0, 16, 16);
}
```

### RigidBodyComponent
```cpp
// Create
auto& body = entity.AddComponent<RigidBodyComponent>(
    world, b2BodyType::b2_dynamicBody, 1, 0, 0, 0.f, true, &entity
);

// Access
auto* body = entity.GetComponent<RigidBodyComponent>();
if (body) {
    b2Vec2 pos = body->GetBodyPosition();
    body->SetBodyPosition(b2Vec2(10.f, 10.f));
}
```

### AnimatorComponent
```cpp
// Create
auto& animator = entity.AddComponent<AnimatorComponent>();

// Add animations
animator.AddAnimation("idle", AnimationClip("assets/animations/idle.json"));
animator.AddAnimation("walk", AnimationClip("assets/animations/walk.json"));

// Control
animator.PlayAnimation("walk");
animator.StopAnimation();
```

### AudioListenerComponent
```cpp
// Create
auto& audio = entity.AddComponent<AudioListenerComponent>();

// Play sounds
audio.PlaySound(AudioClip("assets/audio/sound.ogg"));
audio.SetVolume(0.5f);
```

## 🎯 Common Patterns

### Safe Component Access
```cpp
// Always check if component exists
if (auto* component = entity.GetComponent<ComponentType>()) {
    // Use component
    component->DoSomething();
} else {
    std::cerr << "Component not found!" << std::endl;
}
```

### Entity Iteration
```cpp
// Iterate through all entities
for (auto* entity : entityManager.GetEntities()) {
    if (entity->IsActive()) {
        // Process entity
        if (auto* sprite = entity->GetComponent<SpriteComponent>()) {
            // Do something with sprite
        }
    }
}
```

### Collision Detection
```cpp
// In ContactEventManager.cc
void ContactEventManager::BeginContact(b2Contact* contact) {
    Entity* entityA = static_cast<Entity*>(contact->GetFixtureA()->GetBody()->GetUserData());
    Entity* entityB = static_cast<Entity*>(contact->GetFixtureB()->GetBody()->GetUserData());
    
    if (entityA && entityB) {
        if (entityA->GetName() == "player" && entityB->GetName() == "enemy") {
            // Handle collision
            std::cout << "Player hit enemy!" << std::endl;
        }
    }
}
```

### Input Handling
```cpp
// In component Update method
void MyComponent::Update(float& deltaTime) {
    sf::Vector2f axis = InputSystem::Axis();
    
    if (axis.x != 0 || axis.y != 0) {
        // Handle movement input
        if (auto* transform = owner->GetComponent<TransformComponent>()) {
            transform->Translate(axis * speed * deltaTime);
        }
    }
}
```

## 🎨 Rendering

### Custom Rendering
```cpp
void MyComponent::Render(sf::RenderWindow& window) {
    // Draw custom shapes
    sf::RectangleShape rect(sf::Vector2f(100.f, 100.f));
    rect.setPosition(transform->GetPosition());
    rect.setFillColor(sf::Color::Red);
    window.draw(rect);
    
    // Draw text
    sf::Text text;
    text.setString("Hello World");
    text.setPosition(transform->GetPosition());
    window.draw(text);
}
```

### Debug Drawing
```cpp
// Enable physics debug
flags += b2Draw::e_shapeBit;
world->SetDebugDraw(drawPhysics);
drawPhysics->SetFlags(flags);

// In render loop
if (debugPhysics) {
    world->DebugDraw();
}
```

## 🔧 Configuration

### Game Constants
```cpp
// Add to Constants.hh
namespace GameConstants {
    constexpr float MY_SPEED = 150.0f;
    constexpr int MY_COUNT = 10;
    constexpr float MY_TIMER = 2.0f;
}

// Use in code
float speed = GameConstants::MY_SPEED;
```

### Asset Paths
```cpp
// Add to Constants.hh
const char* ASSETS_MY_TEXTURE = "assets/my_texture.png";
const char* ASSETS_MY_SOUND = "assets/my_sound.ogg";

// Use in code
auto& sprite = entity.AddComponent<SpriteComponent>(ASSETS_MY_TEXTURE, 0, 0);
```

## 🐛 Debugging

### Debug Output
```cpp
// Console output
std::cout << "Debug: " << value << std::endl;
std::cerr << "Error: " << errorMessage << std::endl;

// ImGui debug window
void ImGuiManager::ShowDebugInfo() {
    if (ImGui::Begin("Debug")) {
        ImGui::Text("Value: %f", myValue);
        ImGui::Text("Entities: %zu", entityManager.GetEntities().size());
        
        if (ImGui::Button("Reset")) {
            // Reset logic
        }
    }
    ImGui::End();
}
```

### Performance Monitoring
```cpp
// Frame time measurement
auto start = std::chrono::high_resolution_clock::now();
// ... code ...
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Time: " << duration.count() << "μs" << std::endl;
```

## 📁 File Structure

### New Component
```
include/Components/MyComponent.hh
src/Components/MyComponent.cc
```

### New System
```
include/Systems/MySystem.hh
src/Systems/MySystem.cc
```

### Update CMakeLists.txt
```cmake
add_executable(BlackEngineProject
  # ... existing files ...
  src/Components/MyComponent.cc
  src/Systems/MySystem.cc
)
```

## 🎵 Audio

### Sound Effects
```cpp
// Create audio clip
AudioClip clip("assets/audio/jump.ogg");

// Play sound
if (auto* audio = entity.GetComponent<AudioListenerComponent>()) {
    audio->PlaySound(clip);
}
```

### Background Music
```cpp
// For longer audio files
AudioClip music("assets/audio/background.ogg");
// Set volume lower for music
audio->SetVolume(0.3f);
audio->PlaySound(music);
```

## 🎬 Animation

### JSON Animation Format
```json
{
  "name": "walk",
  "frameWidth": 16,
  "frameHeight": 16,
  "frameRate": 8,
  "frames": [
    {"x": 0, "y": 0, "duration": 0.125},
    {"x": 16, "y": 0, "duration": 0.125},
    {"x": 32, "y": 0, "duration": 0.125}
  ]
}
```

### Animation Control
```cpp
// Add animations
animator.AddAnimation("idle", AnimationClip("assets/animations/idle.json"));
animator.AddAnimation("walk", AnimationClip("assets/animations/walk.json"));

// Switch animations
if (isMoving) {
    animator.PlayAnimation("walk");
} else {
    animator.PlayAnimation("idle");
}
```

## 🎮 Input

### Keyboard Input
```cpp
// In InputSystem.hh
bool IsKeyPressed(sf::Keyboard::Key key);
bool IsKeyJustPressed(sf::Keyboard::Key key);
sf::Vector2f Axis(); // WASD movement
```

### Mouse Input
```cpp
// Mouse position
sf::Vector2i mousePos = sf::Mouse::getPosition(window);
sf::Vector2f worldPos = window.mapPixelToCoords(mousePos);

// Mouse buttons
if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
    // Left click
}
```

## 🔄 Game Loop

### Update Pattern
```cpp
void Game::Update() {
    // Calculate delta time
    deltaTime = gameClock->getElapsedTime().asSeconds();
    gameClock->restart();
    
    // Update systems
    entityManager.Update(deltaTime);
    imguiManager->Update(*window, sf::seconds(deltaTime));
}
```

### Render Pattern
```cpp
void Game::Render() {
    window->clear(sf::Color::Black);
    
    // Render game objects
    tileGroup->Draw();
    entityManager.Render(*window);
    
    // Render debug
    if (debugPhysics) {
        world->DebugDraw();
    }
    
    // Render UI
    imguiManager->Render(*window);
    
    window->display();
}
```

## 🚨 Common Issues

### Memory Leaks
```cpp
// ❌ Bad - raw pointer
Game* game = new Game();

// ✅ Good - smart pointer
auto game = std::make_unique<Game>();
```

### Missing Components
```cpp
// ❌ Bad - no null check
auto* sprite = entity.GetComponent<SpriteComponent>();
sprite->SetFlipTexture(true); // Crash if null!

// ✅ Good - null check
if (auto* sprite = entity.GetComponent<SpriteComponent>()) {
    sprite->SetFlipTexture(true);
}
```

### Asset Loading Errors
```cpp
// ❌ Bad - no error handling
texture.loadFromFile("missing.png");

// ✅ Good - error handling
if (!texture.loadFromFile("missing.png")) {
    std::cerr << "Failed to load texture: missing.png" << std::endl;
    // Handle error
}
```

## 📚 Useful Links

- [SFML Documentation](https://www.sfml-dev.org/documentation.php)
- [Box2D Manual](https://box2d.org/documentation/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/)

---

This quick reference provides the most commonly used patterns and code snippets for the BlackEngine. For detailed documentation, see the main README.md and API.md files.

#pragma once

#include <box2d/box2d.h>

#include <SFML/Graphics.hpp>
#include <memory>

#include "ContactEventManager.hh"
#include "DrawPhysics.hh"
#include "ImGuiManager.hh"

// Forward declarations to reduce header coupling
class TextObject;
class TileGroup;
class EntityManager;

class Game {
 private:
  std::unique_ptr<sf::RenderWindow> window;
  std::unique_ptr<ContactEventManager> contactEventManager;
  std::unique_ptr<ImGuiManager> imguiManager;
  std::unique_ptr<b2Vec2> gravity;
  std::unique_ptr<b2World> world;
  std::unique_ptr<DrawPhysics> drawPhysics;
  bool debugPhysics{};

  // Moved from file-scope globals to class members to control lifetime
  std::unique_ptr<TextObject> textObj1;
  std::unique_ptr<sf::Clock> gameClock;
  float deltaTime{};
  std::unique_ptr<TileGroup> tileGroup;

  // Ensure this is destroyed before 'world' so Box2D world is still valid
  // during component (RigidBodyComponent) destruction.
  // Destruction is in reverse declaration order, so declare this AFTER 'world'.
  std::unique_ptr<EntityManager> entityManager;

  void Update();
  void Render();
  void MainLoop();
  void Destroy();
  void UpdatePhysics();

 public:
  Game();
  ~Game();
  void Initialize();
};

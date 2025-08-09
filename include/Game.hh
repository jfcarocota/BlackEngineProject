#pragma once

#include <SFML/Graphics.hpp>
#include "ContactEventManager.hh"
#include "DrawPhysics.hh"
#include "ImGuiManager.hh"
#include <box2d/box2d.h>
#include <queue>
#include <memory>

class Game
{
private:
  std::unique_ptr<sf::RenderWindow> window;
  std::unique_ptr<ContactEventManager> contactEventManager;
  std::unique_ptr<ImGuiManager> imguiManager;
  std::unique_ptr<b2Vec2> gravity;
  std::unique_ptr<b2World> world;
  std::unique_ptr<DrawPhysics> drawPhysics;
  bool debugPhysics{};

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

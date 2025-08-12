// Standard and external includes
#include <box2d/box2d.h>
#include <iostream>
#include <memory>
#include <cstring>
#include <filesystem>
#include <optional>
#include <gsl/gsl>

#ifdef _WIN32
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
#endif

// Project includes
#include "Game.hh"
#include "Constants.hh"
#include "InputSystem.hh"
#include "GUI/TextObject.hh"
#include "TileGroup.hh"
#include "Components/EntityManager.hh"
#include "Components/TransformComponent.hh"
#include "Components/SpriteComponent.hh"
#include "Components/RigidBodyComponent.hh"
#include "Components/AnimatorComponent.hh"
#include "Components/AudioListenerComponent.hh"
#include "Movement.hh"
#include "FlipSprite.hh"
#include "Components/Entity.hh"
#include "GUI/Button.hh"
#include "GUI/TextObject.hh"
#include "TileGroup.hh"
#include "Components/EntityManager.hh"

// All state moved into Game class members (see Game.hh)

uint32 flags{};
    //flags += b2Draw::e_aabbBit;
    //flags += b2::e_shapeBit;

// Function to find the project root directory
std::string findProjectRoot() {
    std::filesystem::path exeDir;

    #ifdef _WIN32
      wchar_t wpath[MAX_PATH];
      DWORD len = GetModuleFileNameW(nullptr, wpath, MAX_PATH);
      if (len > 0) {
          exeDir = std::filesystem::path(wpath).parent_path();
      } else {
          exeDir = std::filesystem::current_path();
      }
    #elif defined(__APPLE__)
      char path[1024];
      uint32_t size = sizeof(path);
      if (_NSGetExecutablePath(path, &size) == 0) {
          exeDir = std::filesystem::path(path).parent_path();
      } else {
          exeDir = std::filesystem::current_path();
      }
    #else
      exeDir = std::filesystem::current_path();
    #endif

    // Walk up to 5 levels to find a folder containing "assets"
    auto current = exeDir;
    for (int i = 0; i <= 5; ++i) {
        if (std::filesystem::exists(current / "assets")) {
            return current.string();
        }
        if (!current.has_parent_path()) break;
        current = current.parent_path();
    }

    // Fallback to current working directory
    return std::filesystem::current_path().string();
}

Game::Game()
{
  std::string projectRoot = findProjectRoot();
  std::filesystem::current_path(projectRoot);

  window = std::make_unique<sf::RenderWindow>(sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)), GAME_NAME);
  gravity = std::make_unique<b2Vec2>(0.f, 0.f);
  world = std::make_unique<b2World>(*gravity);
  drawPhysics = std::make_unique<DrawPhysics>(window.get());
  entityManager = std::make_unique<EntityManager>();

  tileGroup = std::make_unique<TileGroup>(window.get(), GameConstants::MAP_WIDTH, GameConstants::MAP_HEIGHT, 
                                        ASSETS_MAPS_JSON_THREE, GameConstants::TILE_SCALE, GameConstants::TILE_SIZE, 
                                        GameConstants::TILE_SIZE, ASSETS_TILES);

  auto& hero{entityManager->AddEntity("hero")};
  auto& candle1{entityManager->AddEntity("candle")};
  auto& chest1{entityManager->AddEntity("chest")};
  auto& chest2{entityManager->AddEntity("chest")};
  auto& chest3{entityManager->AddEntity("chest")};

  Entity& buttonDebugPhysics{entityManager->AddEntity("button")};

  hero.AddComponent<TransformComponent>(500.f, 300.f, 16.f, 16.f, 4.f);
  hero.AddComponent<SpriteComponent>(ASSETS_SPRITES, 0, 5);
  hero.AddComponent<RigidBodyComponent>(world.get(), b2BodyType::b2_dynamicBody, 1, 0, 0, 0.f, true, (void*) &hero);
  hero.AddComponent<AnimatorComponent>();
  hero.AddComponent<AudioListenerComponent>();
  hero.AddComponent<Movement>(GameConstants::PLAYER_SPEED, GameConstants::PLAYER_FRICTION, AudioClip("assets/audio/steps.ogg"));
  hero.AddComponent<FlipSprite>();

  candle1.AddComponent<TransformComponent>(500.f, 500.f, 16.f, 16.f, 3.f);
  candle1.AddComponent<SpriteComponent>(ASSETS_SPRITES, 0, 5);
  candle1.AddComponent<RigidBodyComponent>(world.get(), b2BodyType::b2_staticBody, 1, 0, 0, 0.f, true, (void*) &candle1);
  auto& candle1Animator = candle1.AddComponent<AnimatorComponent>();
  candle1Animator.AddAnimation("idle", AnimationClip("assets/animations/candle/idle.json"));

  chest1.AddComponent<TransformComponent>(300.f, 500.f, 16.f, 16.f, 4.f);
  chest1.AddComponent<SpriteComponent>(ASSETS_SPRITES, 6, 1);
  chest1.AddComponent<RigidBodyComponent>(world.get(), b2BodyType::b2_staticBody, 1, 0, 0, 0.f, true, (void*) &chest1);

  chest2.AddComponent<TransformComponent>(300.f, 400.f, 16.f, 16.f, 4.f);
  chest2.AddComponent<SpriteComponent>(ASSETS_SPRITES, 6, 1);
  chest2.AddComponent<RigidBodyComponent>(world.get(), b2BodyType::b2_staticBody, 1, 0, 0, 0.f, true, (void*) &chest2);

  chest3.AddComponent<TransformComponent>(300.f, 300.f, 16.f, 16.f, 4.f);
  chest3.AddComponent<SpriteComponent>(ASSETS_SPRITES, 6, 1);
  chest3.AddComponent<RigidBodyComponent>(world.get(), b2BodyType::b2_staticBody, 1, 0, 0, 0.f, true, (void*) &chest3);

  auto& btnPhysicsDebugTrs{buttonDebugPhysics.AddComponent<TransformComponent>(100.f, 100.f, 200.f, 100.f, 1.f)};
  auto& buttonPhysicsComp = buttonDebugPhysics.AddComponent<Button>(btnPhysicsDebugTrs, 0.f,
  sf::Color::White, sf::Color::Transparent, [this](){
    std::cout << "clicked" << std::endl;
    debugPhysics = !debugPhysics;
  });
  buttonPhysicsComp.SetTexture("assets/GUI/button.png");

  contactEventManager = std::make_unique<ContactEventManager>();
  imguiManager = std::make_unique<ImGuiManager>();
}

Game::~Game() = default;

void Game::Initialize()
{
  flags += b2Draw::e_shapeBit;
  world->SetDebugDraw(drawPhysics.get());
  drawPhysics->SetFlags(flags);
  world->SetContactListener(contactEventManager.get());

  imguiManager->Initialize(*window);

  textObj1 = std::make_unique<TextObject>(ASSETS_FONT_ARCADECLASSIC, 14, sf::Color::White, sf::Text::Bold);
  gameClock = std::make_unique<sf::Clock>();
  
  textObj1->SetTextStr("Hello game engine");
  // Ensure the text is positioned in-view
  if (auto txt = textObj1->GetText()) {
    txt->setPosition(sf::Vector2f(10.f, 10.f));
  }
  MainLoop();
}

void Game::UpdatePhysics()
{
  world->ClearForces();
  world->Step(deltaTime, GameConstants::PHYSICS_VELOCITY_ITERATIONS, GameConstants::PHYSICS_POSITION_ITERATIONS);
}

void Game::Update()
{
  if (gameClock) {
    // Use gsl::not_null for safety
    deltaTime = gsl::not_null<sf::Clock*>(gameClock.get())->getElapsedTime().asSeconds();
    gameClock->restart();
  }
  if (entityManager) entityManager->Update(deltaTime);
  imguiManager->Update(*window, sf::seconds(deltaTime));
}

void Game::MainLoop()
{
  while (window->isOpen())
  {
    while (true)
    {
      auto evt = window->pollEvent();
      if (!evt.has_value()) break;
      imguiManager->ProcessEvent(evt.value());
      if (evt->is<sf::Event::Closed>())
      {
        window->close();
      }
    }

    UpdatePhysics();
    Update();
    Render();
  }
  Destroy();
}

void Game::Render()
{
  window->clear(sf::Color::Black);

  if (tileGroup) {
    gsl::not_null<TileGroup*>(tileGroup.get())->Draw();
  }
  if (entityManager) entityManager->Render(*gsl::not_null<sf::RenderWindow*>(window.get()));
  if(debugPhysics)
  {
    gsl::not_null<b2World*>(world.get())->DebugDraw();
  }
  
  // Draw UI text above world/debug
  if (textObj1) {
    gsl::not_null<TextObject*>(textObj1.get());
    window->draw(*textObj1->GetText());
  }

  // Render ImGui on top
  gsl::not_null<ImGuiManager*>(imguiManager.get())->Render(*window);
  
  window->display();
}

void Game::Destroy()
{
  // Shutdown ImGui
  imguiManager->Shutdown();
  // Smart pointers automatically clean up
  // Detach Box2D hooks before destroying their owners
  if (world) {
    world->SetDebugDraw(nullptr);
    world->SetContactListener(nullptr);
  }
  // Explicitly reset components in safe order: entities (with Box2D bodies) before Box2D world
  entityManager.reset();
  tileGroup.reset();
  drawPhysics.reset();
  contactEventManager.reset();
  imguiManager.reset();
  world.reset();
  gravity.reset();
  textObj1.reset();
  gameClock.reset();
}
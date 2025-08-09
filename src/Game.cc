// Standard and external includes
#include <box2d/box2d.h>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <cstring>
#include <filesystem>
#include <mach-o/dyld.h>
#include <optional>
#include <gsl/gsl>

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

EntityManager entityManager;

std::unique_ptr<TextObject> textObj1;
std::unique_ptr<sf::Clock> gameClock;
float deltaTime{};

std::unique_ptr<TileGroup> tileGroup;

uint32 flags{};
    //flags += b2Draw::e_aabbBit;
    //flags += b2::e_shapeBit;

// Function to find the project root directory
std::string findProjectRoot() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string exePath(path);
        size_t lastSlash = exePath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string exeDir = exePath.substr(0, lastSlash);
            if (exeDir.find("cmake-build-debug") != std::string::npos) {
                std::string projectRoot = exeDir.substr(0, exeDir.find("cmake-build-debug"));
                return projectRoot;
            }
        }
    }
    std::string currentDir = std::filesystem::current_path().string();
    for (int i = 0; i <= 3; i++) {
        std::string testPath = currentDir;
        for (int j = 0; j < i; j++) testPath += "/..";
        std::string assetsPath = testPath + "/assets";
        if (std::filesystem::exists(assetsPath)) return testPath;
    }
    return currentDir;
}

Game::Game()
{
  std::string projectRoot = findProjectRoot();
  chdir(projectRoot.c_str());

  window = std::make_unique<sf::RenderWindow>(sf::VideoMode(sf::Vector2u(WINDOW_WIDTH, WINDOW_HEIGHT)), GAME_NAME);
  gravity = std::make_unique<b2Vec2>(0.f, 0.f);
  world = std::make_unique<b2World>(*gravity);
  drawPhysics = std::make_unique<DrawPhysics>(window.get());

  tileGroup = std::make_unique<TileGroup>(window.get(), GameConstants::MAP_WIDTH, GameConstants::MAP_HEIGHT, 
                                        ASSETS_MAPS, GameConstants::TILE_SCALE, GameConstants::TILE_SIZE, 
                                        GameConstants::TILE_SIZE, ASSETS_TILES);

  auto& hero{entityManager.AddEntity("hero")};
  auto& candle1{entityManager.AddEntity("candle")};
  auto& chest1{entityManager.AddEntity("chest")};
  auto& chest2{entityManager.AddEntity("chest")};
  auto& chest3{entityManager.AddEntity("chest")};

  Entity& buttonDebugPhysics{entityManager.AddEntity("button")};

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
  sf::Color::White, sf::Color::Transparent, [=, this](){
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
  entityManager.Update(deltaTime);
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
  entityManager.Render(*gsl::not_null<sf::RenderWindow*>(window.get()));
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
}
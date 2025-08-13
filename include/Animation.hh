#pragma once
#include <fstream>

#include "Components/SpriteComponent.hh"
#include "Components/TransformComponent.hh"
#include "SFML/Graphics.hpp"
#include "json/json.h"

class Animation {
 private:
  int animationIndex{};
  int startFrame{};
  int endFrame{};
  float animationDelay{};
  float currentTime{};
  int currentAnimation{};
  SpriteComponent& sprite;
  TransformComponent& transform;
  Json::Value root{};

 public:
  Animation();
  Animation(SpriteComponent& sprite, TransformComponent& transform,
            const char* animUrl);

  void Play(float& deltaTime);
  ~Animation();
};
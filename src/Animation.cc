#include "Animation.hh"

#include <iostream>

Animation::Animation(SpriteComponent& sprite, TransformComponent& transform,
                     const char* animUrl)
    : sprite(sprite), transform(transform) {
  std::ifstream reader;
  reader.open(animUrl);

  if (!reader.is_open()) {
    std::cerr << "Failed to open animation file: " << animUrl << std::endl;
    return;
  }

  root = Json::Value();

  try {
    reader >> root;

    if (root.isNull() || !root.isObject()) {
      std::cerr << "Invalid JSON format in animation file: " << animUrl
                << std::endl;
      return;
    }

    if (!root["animation"].isObject()) {
      std::cerr << "Missing 'animation' object in file: " << animUrl
                << std::endl;
      return;
    }

    startFrame = root["animation"]["startFrame"].asInt();
    endFrame = root["animation"]["endFrame"].asInt();
    animationDelay = root["animation"]["delay"].asFloat();
    currentAnimation = root["animation"]["row"].asInt();
    animationIndex = startFrame;

  } catch (const Json::RuntimeError& e) {
    std::cerr << "JSON parsing error in animation file " << animUrl << ": "
              << e.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Error reading animation file " << animUrl << ": " << e.what()
              << std::endl;
  }

  reader.close();
}

void Animation::Play(float& deltaTime) {
  currentTime += deltaTime;
  sprite.RebindRectTexture(animationIndex * transform.GetWidth(),
                           currentAnimation * transform.GetHeight(),
                           transform.GetWidth(), transform.GetHeight());

  if (currentTime > animationDelay) {
    std::cout << "index: " << animationIndex;
    if (animationIndex == endFrame) {
      animationIndex = startFrame;
    } else {
      animationIndex++;
    }
    currentTime = 0.f;
  }
}

Animation::~Animation() {}
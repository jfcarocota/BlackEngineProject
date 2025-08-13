#pragma once
#include <fstream>
#include <iostream>

#include "json/json.h"

class AnimationClip {
 private:
  Json::Value root{};
  bool isValid{false};

 public:
  int animationIndex{};
  int startFrame{};
  int endFrame{};
  float animationDelay{};
  int currentAnimation{};

  AnimationClip() {}

  AnimationClip(const char* animUrl) {
    std::ifstream reader;
    reader.open(animUrl);

    if (!reader.is_open()) {
      std::cerr << "Failed to open animation file: " << animUrl << std::endl;
      isValid = false;
      return;
    }

    root = Json::Value();

    try {
      reader >> root;

      if (root.isNull() || !root.isObject()) {
        std::cerr << "Invalid JSON format in animation file: " << animUrl
                  << std::endl;
        isValid = false;
        return;
      }

      if (!root["animation"].isObject()) {
        std::cerr << "Missing 'animation' object in file: " << animUrl
                  << std::endl;
        isValid = false;
        return;
      }

      startFrame = root["animation"]["startFrame"].asInt();
      endFrame = root["animation"]["endFrame"].asInt();
      animationDelay = root["animation"]["delay"].asFloat();
      currentAnimation = root["animation"]["row"].asInt();
      animationIndex = startFrame;

      isValid = true;

    } catch (const Json::RuntimeError& e) {
      std::cerr << "JSON parsing error in animation file " << animUrl << ": "
                << e.what() << std::endl;
      isValid = false;
    } catch (const std::exception& e) {
      std::cerr << "Error reading animation file " << animUrl << ": "
                << e.what() << std::endl;
      isValid = false;
    }

    reader.close();
  }

  // Copy constructor
  AnimationClip(const AnimationClip& other) = default;

  // Copy assignment operator
  AnimationClip& operator=(const AnimationClip& other) = default;

  // Move constructor
  AnimationClip(AnimationClip&& other) = default;

  // Move assignment operator
  AnimationClip& operator=(AnimationClip&& other) = default;

  ~AnimationClip() = default;

  bool IsValid() const { return isValid; }
};

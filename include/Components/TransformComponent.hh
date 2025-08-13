#pragma once

#include <SFML/Graphics.hpp>

#include "Component.hh"

class TransformComponent : public Component {
 private:
  sf::Vector2f position{};
  float width{};
  float height{};
  float scale{};

 public:
  TransformComponent(float posX, float posY, float width, float height,
                     float scale);
  ~TransformComponent();
  void Initialize() override;
  void Update(float& deltaTime) override;
  float GetWidth() const;
  float GetHeight() const;
  float GetScale() const;
  sf::Vector2f GetPosition() const;
  void SetPosition(sf::Vector2f position);
  void SetWidth(float width);
  void SetHeight(float height);
  void SetScale(float scale);
  void Translate(sf::Vector2f direction);
};
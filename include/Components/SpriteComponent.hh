#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

#include "Component.hh"
#include "TransformComponent.hh"

class SpriteComponent : public Component {
 private:
  TransformComponent* transform;
  sf::Texture texture{};
  std::unique_ptr<sf::Sprite>
      sprite;  // SFML 3: construct after texture is ready
  const char* textureUrl{};
  unsigned int col{}, row{};
  bool flipTexture{false};

 public:
  SpriteComponent(const char* textureUrl, unsigned int col, unsigned int row);
  ~SpriteComponent();
  void Update(float& deltaTime) override;
  void Render(sf::RenderWindow& window) override;
  void SetFlipTexture(bool flip);
  bool GetFlipTexture() const;
  sf::Vector2f GetOrigin() const;
  void RebindRectTexture(int col, int row, float width, float height);
  void Initialize() override;
};

#include "Components/SpriteComponent.hh"

#include <gsl/assert>
#include <gsl/narrow>
#include <iostream>

#include "Components/EntityManager.hh"

SpriteComponent::SpriteComponent(const char* textureUrl, unsigned int col,
                                 unsigned int row) {
  this->textureUrl = textureUrl;
  this->col = col;
  this->row = row;

  if (!texture.loadFromFile(textureUrl)) {
    std::cerr << "Failed to load texture: " << textureUrl << std::endl;
  }
}

void SpriteComponent::Initialize() {
  transform = owner->GetComponent<TransformComponent>();

  Expects(transform != nullptr);

  const int w = gsl::narrow_cast<int>(transform->GetWidth());
  const int h = gsl::narrow_cast<int>(transform->GetHeight());
  const int left =
      gsl::narrow_cast<int>(static_cast<float>(col) * transform->GetWidth());
  const int top =
      gsl::narrow_cast<int>(static_cast<float>(row) * transform->GetHeight());

  // Create sprite once transform is available
  sprite =
      std::make_unique<sf::Sprite>(texture, sf::IntRect({left, top}, {w, h}));

  sprite->setPosition(transform->GetPosition());
  sprite->setScale(sf::Vector2f(transform->GetScale(), transform->GetScale()));
  sprite->setColor(sf::Color::White);
  sprite->setOrigin(sf::Vector2f(w * 0.5f, h * 0.5f));

  Ensures(sprite != nullptr);
}

SpriteComponent::~SpriteComponent() {}

void SpriteComponent::Update(float& deltaTime) {
  if (transform != nullptr && sprite) {
    sprite->setPosition(transform->GetPosition());
  }
}

void SpriteComponent::Render(sf::RenderWindow& window) {
  if (sprite) window.draw(*sprite);
}

void SpriteComponent::SetFlipTexture(bool flipTexture) {
  this->flipTexture = flipTexture;
  Expects(transform != nullptr);
  if (sprite)
    sprite->setScale(sf::Vector2f(
        flipTexture ? -transform->GetScale() : transform->GetScale(),
        transform->GetScale()));
}

bool SpriteComponent::GetFlipTexture() const { return flipTexture; }

sf::Vector2f SpriteComponent::GetOrigin() const {
  return sprite ? sprite->getOrigin() : sf::Vector2f{};
}

void SpriteComponent::RebindRectTexture(int col, int row, float width,
                                        float height) {
  if (sprite)
    sprite->setTextureRect(sf::IntRect(
        {col, row},
        {gsl::narrow_cast<int>(width), gsl::narrow_cast<int>(height)}));
}

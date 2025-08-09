#include "Components/SpriteComponent.hh"
#include "Components/EntityManager.hh"
#include <iostream>

SpriteComponent::SpriteComponent(const char* textureUrl, unsigned int col, unsigned int row)
{
  this->textureUrl = textureUrl;
  this->col = col;
  this->row = row;

  if (!texture.loadFromFile(textureUrl)) {
    std::cerr << "Failed to load texture: " << textureUrl << std::endl;
  }
}

void SpriteComponent::Initialize()
{
  transform = owner->GetComponent<TransformComponent>();

  const int w = static_cast<int>(transform->GetWidth());
  const int h = static_cast<int>(transform->GetHeight());
  const int left = static_cast<int>(col * transform->GetWidth());
  const int top  = static_cast<int>(row * transform->GetHeight());

  // Create sprite once transform is available
  sprite = std::make_unique<sf::Sprite>(
    texture,
    sf::IntRect({left, top}, {w, h})
  );

  sprite->setPosition(transform->GetPosition());
  sprite->setScale(sf::Vector2f(transform->GetScale(), transform->GetScale()));
  sprite->setColor(sf::Color::White);
  sprite->setOrigin(sf::Vector2f(w * 0.5f, h * 0.5f));
}

SpriteComponent::~SpriteComponent()
{
}

void SpriteComponent::Update(float& deltaTime)
{
  if(transform != nullptr && sprite)
  {
    sprite->setPosition(transform->GetPosition());
  }
}

void SpriteComponent::Render(sf::RenderWindow& window)
{
  if (sprite) window.draw(*sprite);
}

void SpriteComponent::SetFlipTexture(bool flipTexture)
{
  this->flipTexture = flipTexture;
  if (sprite)
    sprite->setScale(sf::Vector2f(flipTexture ? -transform->GetScale(): transform->GetScale(), transform->GetScale()));
}

bool SpriteComponent::GetFlipTexture() const
{
  return flipTexture;
}

sf::Vector2f SpriteComponent::GetOrigin() const
{
  return sprite ? sprite->getOrigin() : sf::Vector2f{};
}

void SpriteComponent::RebindRectTexture(int col, int row, float width, float height)
{
  if (sprite)
    sprite->setTextureRect(sf::IntRect({col, row}, {static_cast<int>(width), static_cast<int>(height)}));
}

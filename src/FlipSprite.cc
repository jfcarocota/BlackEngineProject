#include "FlipSprite.hh"
#include "InputSystem.hh"
#include "Components/EntityManager.hh"
#include <gsl/assert>

FlipSprite::FlipSprite()
{
  
}

FlipSprite::~FlipSprite()
{
}

void FlipSprite::Initialize()
{
  transform = owner->GetComponent<TransformComponent>();
  spriteComponent = owner->GetComponent<SpriteComponent>();
  Expects(transform != nullptr);
  Expects(spriteComponent != nullptr);
}

void FlipSprite::Update(float& deltaTime)
{
  Expects(spriteComponent != nullptr);
  sf::Vector2f axis = InputSystem::Axis();
  spriteComponent->SetFlipTexture(axis.x < 0 ? true : axis.x > 0 ? false : spriteComponent->GetFlipTexture());
}

#include "Components/TransformComponent.hh"
#include <gsl/assert>

TransformComponent::TransformComponent(float posX, float posY, float width, float height, float scale)
{
  position = sf::Vector2(posX, posY);
  this->width = width;
  this->height = height;
  this->scale = scale;
}

TransformComponent::~TransformComponent()
{

}

void TransformComponent::Initialize()
{

}
void TransformComponent::Update(float& deltaTime)
{

}

float TransformComponent::GetWidth() const
{
  return width;
}
float TransformComponent::GetHeight() const
{
  return height;
}
float TransformComponent::GetScale() const
{
  return scale;
}

void TransformComponent::SetWidth(float width)
{
  Expects(width >= 0.0f);
  this->width = width;
}
void TransformComponent::SetHeight(float height)
{
  Expects(height >= 0.0f);
  this->height = height;
}
void TransformComponent::SetScale(float scale)
{
  Expects(scale >= 0.0f);
  this->scale = scale;
}

sf::Vector2f TransformComponent::GetPosition() const
{
  return position;
}
void TransformComponent::SetPosition(sf::Vector2f position)
{
  this->position = position;
}

void TransformComponent::Translate(sf::Vector2f direction)
{
  position += direction;
}

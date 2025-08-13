#pragma once
#include <box2d/box2d.h>

#include <SFML/Graphics.hpp>
#include <gsl/gsl>

#include "Component.hh"
#include "SpriteComponent.hh"
#include "TransformComponent.hh"

class RigidBodyComponent : public Component {
 private:
  b2BodyDef* bodyDef{};
  b2Body* body{};
  b2PolygonShape* polygonShape{};
  b2FixtureDef* fixtureDef{};
  b2Fixture* fixture{};
  gsl::not_null<b2World*> world;
  TransformComponent* transform{};
  SpriteComponent* spriteComponent{};

  b2Vec2 bodyPos{};
  sf::Vector2f trsPos{};

  float density{};
  float friction{};
  float restitution{};
  float angle{};
  bool frezeRotation{};
  void* userData{};

 public:
  RigidBodyComponent(gsl::not_null<b2World*> world, b2BodyType bodyType,
                     float density, float friction, float restitution,
                     float angle, bool frezeRotation, void* userData);
  ~RigidBodyComponent();

  b2Body* GetBody() const;
  void FreezeRotation(bool freeze);
  sf::Vector2f GetPositionSFML() const;
  b2Vec2 GetPosition() const;
  void AddVelocity(b2Vec2 velocity);
  void Update(float& deltaTime) override;
  void Initialize() override;
};
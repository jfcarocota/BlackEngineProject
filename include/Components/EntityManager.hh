#pragma once
#include <SFML/Graphics.hpp>
#include "Component.hh"
#include "Entity.hh"
#include <vector>
#include <memory>
#include <gsl/span>

class EntityManager
{
private:
  std::vector<std::unique_ptr<Entity>> entities;
  std::vector<std::unique_ptr<Entity>> activeEntities;
  std::vector<std::unique_ptr<Entity>> inactiveEntities;
public:
  EntityManager(/* args */);
  ~EntityManager();

  void ClearData();
  void Update(float& deltaTime);
  void Render(sf::RenderWindow& window);
  bool HasNoEntities();
  Entity& AddEntity(std::string entityName);
  gsl::span<Entity*> GetEntities() const;
  unsigned int GetentityCount() const;
};
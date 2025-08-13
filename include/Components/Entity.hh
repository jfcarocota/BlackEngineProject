#pragma once

#include<SFML/Graphics.hpp>
#include <vector>
#include <string>
#include<map>
#include "Component.hh"
#include "EntityManager.hh"
#include <gsl/pointers>
#include <gsl/assert>

class Component;
class EntityManager;

class Entity
{
private:
  EntityManager& entityManager;
  bool isActive;
  // Owns the lifetime of attached components
  std::vector<gsl::owner<Component*>> components;
  std::map<const std::type_info*, Component*> componentTypeMap;
public:
  std::string name;
  Entity(EntityManager& entityManager);
  Entity(EntityManager& entityManager, std::string name);
  void Update(float& deltaTime);
  void Render(sf::RenderWindow& window);
  void Destroy();
  bool IsActive() const;
  ~Entity();

  template <typename T, typename... TArgs>
  T& AddComponent(TArgs&&... args)
  {
    T* newComponent{new T(std::forward<TArgs>(args)...)};
    newComponent->owner = this;
    components.emplace_back(newComponent);
    componentTypeMap[&typeid(*newComponent)] = newComponent;
    newComponent->Initialize();
  Ensures(componentTypeMap.find(&typeid(*newComponent)) != componentTypeMap.end());
    return *newComponent;
  }

  template<typename T>
  T* GetComponent()
  {
  auto it = componentTypeMap.find(&typeid(T));
  if (it == componentTypeMap.end()) return nullptr;
  return static_cast<T*>(it->second);
  }
};

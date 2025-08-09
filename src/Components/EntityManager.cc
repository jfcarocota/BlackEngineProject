#include "Components/EntityManager.hh"

EntityManager::EntityManager()
{

}

EntityManager::~EntityManager()
{

}

void EntityManager::ClearData()
{
  for(auto& entity : entities)
  {
    entity->Destroy();
  }
  entities.clear();
}

bool EntityManager::HasNoEntities()
{
  return entities.empty();
}

void EntityManager::Update(float& deltaTime)
{
  activeEntities.clear();
  activeEntities.reserve(entities.size());
  inactiveEntities.clear();
  
  for(auto& entity : entities)
  {
    if(entity->IsActive())
    {
      entity->Update(deltaTime);
      activeEntities.push_back(std::move(entity));
    }
    else
    {
      inactiveEntities.push_back(std::move(entity));
    }
  }
  
  entities = std::move(activeEntities);
  // inactiveEntities will be destroyed automatically
}

void EntityManager::Render(sf::RenderWindow& window)
{
  for(auto& entity : entities)
  {
    if(entity->IsActive())
    {
      entity->Render(window);
    }
  }
}

Entity& EntityManager::AddEntity(std::string entityName)
{
  Entity* entity{new Entity(*this, entityName)};
  entities.emplace_back(entity);
  return *entity;
}

gsl::span<Entity*> EntityManager::GetEntities() const {
  static std::vector<Entity*> result;
  result.clear();
  result.reserve(entities.size());
  for (const auto& e : entities) {
    result.push_back(e.get());
  }
  return gsl::span<Entity*>(result.data(), result.size());
}
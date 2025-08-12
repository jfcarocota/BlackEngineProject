#include "ContactEventManager.hh"
#include<iostream>
#include "Components/Component.hh"
#include "Components/Entity.hh"
#include "Components/EntityManager.hh"
#include "Components/RigidBodyComponent.hh"
#include "Components/AudioListenerComponent.hh"
#include "AudioClip.hh"

ContactEventManager::ContactEventManager()
{

}

ContactEventManager::~ContactEventManager()
{
}

void ContactEventManager::BeginContact(b2Contact* contact)
{
  if (!contact) return;
  auto* fA = contact->GetFixtureA();
  auto* fB = contact->GetFixtureB();
  if (!fA || !fB) return;
  auto* bA = fA->GetBody();
  auto* bB = fB->GetBody();
  if (!bA || !bB) return;
  Entity* actorA{reinterpret_cast<Entity*>(bA->GetUserData().pointer)};
  Entity* actorB{reinterpret_cast<Entity*>(bB->GetUserData().pointer)};

  if(actorA && actorB)
  {
    std::cout << "Collision: " << actorA->name << ", " << actorB->name << std::endl;
    if(actorB->name.compare("chest") == 0)
    {
      // Play chest hit sound effect using hero's audio listener
      if(actorA->name.compare("hero") == 0)
      {
        auto audioListener = actorA->GetComponent<AudioListenerComponent>();
        if(audioListener)
        {
          static AudioClip chestHitSound("assets/audio/steps.ogg");
          audioListener->PlayOneShot(chestHitSound, 0.5f);
        }
      }
      
      actorB->Destroy();
    }
  }
}
void ContactEventManager::EndContact(b2Contact* contact)
{

}
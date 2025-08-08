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
  Entity* actorA{(Entity*)contact->GetFixtureA()->GetBody()->GetUserData().pointer};
  Entity* actorB{(Entity*)contact->GetFixtureB()->GetBody()->GetUserData().pointer};

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
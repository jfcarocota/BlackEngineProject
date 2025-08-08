#pragma once
#include "Component.hh"
#include "AudioClip.hh"
#ifdef SFML_AUDIO_AVAILABLE
#include "SFML/Audio.hpp"
#endif

class AudioListenerComponent: public Component
{
private:
  AudioClip* audioClip{};
#ifdef SFML_AUDIO_AVAILABLE
  sf::SoundBuffer soundBuffer{};
#endif
public:
  AudioListenerComponent();
  ~AudioListenerComponent();

  AudioClip* GetAudioClip() const;
  void Play();
  void PlayOneShot(AudioClip& audioClip);
  void PlayOneShot(AudioClip& audioClip, float audioVolume);
  void Initialize() override;
};

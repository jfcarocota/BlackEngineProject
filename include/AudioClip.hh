#pragma once
#ifdef SFML_AUDIO_AVAILABLE
#include "SFML/Audio.hpp"
#endif
#include<string>

class AudioClip
{
private:
  std::string audioUrl{};
#ifdef SFML_AUDIO_AVAILABLE
  sf::Sound* sound{};
#endif
public:
  AudioClip();
  AudioClip(const char* audioUrl);
  ~AudioClip();

#ifdef SFML_AUDIO_AVAILABLE
  void Play(sf::SoundBuffer& buffer);
#endif
  void SetVolume(float volume);
};
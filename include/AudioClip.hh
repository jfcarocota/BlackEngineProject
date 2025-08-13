#pragma once
#ifdef SFML_AUDIO_AVAILABLE
#include "SFML/Audio.hpp"
#endif
#include <memory>
#include <string>

class AudioClip {
 private:
  std::string audioUrl{};
#ifdef SFML_AUDIO_AVAILABLE
  std::unique_ptr<sf::Sound> sound;
  std::unique_ptr<sf::SoundBuffer> buffer;
#endif
 public:
  AudioClip();
  AudioClip(const char* audioUrl);
  ~AudioClip();

  // Rule of five
  AudioClip(const AudioClip& other);
  AudioClip& operator=(const AudioClip& other);
  AudioClip(AudioClip&& other) noexcept;
  AudioClip& operator=(AudioClip&& other) noexcept;

#ifdef SFML_AUDIO_AVAILABLE
  void Play(sf::SoundBuffer& buffer);
#endif
  void SetVolume(float volume);
};
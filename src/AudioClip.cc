#include "AudioClip.hh"

#include <gsl/assert>
#include <iostream>
#include <memory>

AudioClip::AudioClip() {
#ifdef SFML_AUDIO_AVAILABLE
  sound = nullptr;
  buffer = nullptr;
#endif
}

AudioClip::AudioClip(const char* audioUrl) {
  this->audioUrl = audioUrl;

#ifdef SFML_AUDIO_AVAILABLE
  sound = nullptr;
  buffer = nullptr;

  if (!audioUrl) {
    std::cerr << "AudioClip: audioUrl is null" << std::endl;
    return;
  }

  try {
    buffer = std::make_unique<sf::SoundBuffer>();
    if (!buffer) {
      std::cerr << "Failed to create SFML SoundBuffer object" << std::endl;
      return;
    }
    if (!buffer->loadFromFile(audioUrl)) {
      std::cerr << "Failed to load audio file: " << audioUrl << std::endl;
      buffer.reset();
      return;
    }
    sound = std::make_unique<sf::Sound>(*buffer);
  } catch (const std::exception& e) {
    std::cerr << "Exception creating AudioClip: " << e.what() << std::endl;
    sound.reset();
    buffer.reset();
  }
#endif
}

void AudioClip::SetVolume(float volume) {
#ifdef SFML_AUDIO_AVAILABLE
  Expects(volume >= 0.0f && volume <= 100.0f);
  if (sound) {
    sound->setVolume(volume);
  }
#endif
}

AudioClip::~AudioClip() {
#ifdef SFML_AUDIO_AVAILABLE
  // Smart pointers automatically clean up
#endif
}

// Copy constructor
AudioClip::AudioClip(const AudioClip& other) {
  audioUrl = other.audioUrl;
#ifdef SFML_AUDIO_AVAILABLE
  sound = nullptr;
  buffer = nullptr;
  if (!other.audioUrl.empty()) {
    try {
      buffer = std::make_unique<sf::SoundBuffer>();
      if (buffer && buffer->loadFromFile(audioUrl)) {
        sound = std::make_unique<sf::Sound>(*buffer);
      }
    } catch (...) {
      sound.reset();
      buffer.reset();
    }
  }
#endif
}

// Copy assignment
AudioClip& AudioClip::operator=(const AudioClip& other) {
  if (this == &other) return *this;
  audioUrl = other.audioUrl;
#ifdef SFML_AUDIO_AVAILABLE
  sound.reset();
  buffer.reset();
  if (!other.audioUrl.empty()) {
    try {
      buffer = std::make_unique<sf::SoundBuffer>();
      if (buffer && buffer->loadFromFile(audioUrl)) {
        sound = std::make_unique<sf::Sound>(*buffer);
      }
    } catch (...) {
      sound.reset();
      buffer.reset();
    }
  }
#endif
  return *this;
}

// Move constructor
AudioClip::AudioClip(AudioClip&& other) noexcept {
#ifdef SFML_AUDIO_AVAILABLE
  sound = std::move(other.sound);
  buffer = std::move(other.buffer);
#endif
  audioUrl = std::move(other.audioUrl);
}

// Move assignment
AudioClip& AudioClip::operator=(AudioClip&& other) noexcept {
  if (this != &other) {
#ifdef SFML_AUDIO_AVAILABLE
    sound = std::move(other.sound);
    buffer = std::move(other.buffer);
#endif
    audioUrl = std::move(other.audioUrl);
  }
  return *this;
}

#ifdef SFML_AUDIO_AVAILABLE
void AudioClip::Play(sf::SoundBuffer&) {
  Expects(sound != nullptr);
  if (!sound) {
    std::cerr << "Sound object is null, cannot play audio" << std::endl;
    return;
  }
  try {
    sound->play();
  } catch (const std::exception& e) {
    std::cerr << "Exception playing audio: " << e.what() << std::endl;
  }
}
#endif
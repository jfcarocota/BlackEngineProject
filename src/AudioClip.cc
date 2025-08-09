#include "AudioClip.hh"
#include <iostream>

AudioClip::AudioClip()
{
#ifdef SFML_AUDIO_AVAILABLE
  sound = nullptr;
  buffer = nullptr;
#endif
}

AudioClip::AudioClip(const char* audioUrl)
{
  this->audioUrl = audioUrl;
  
#ifdef SFML_AUDIO_AVAILABLE
  sound = nullptr;
  buffer = nullptr;
  
  if (!audioUrl) {
    std::cerr << "AudioClip: audioUrl is null" << std::endl;
    return;
  }
  
  try {
    buffer = new sf::SoundBuffer();
    if (!buffer) {
      std::cerr << "Failed to create SFML SoundBuffer object" << std::endl;
      return;
    }
    if (!buffer->loadFromFile(audioUrl)) {
      std::cerr << "Failed to load audio file: " << audioUrl << std::endl;
      delete buffer; buffer = nullptr;
      return;
    }
    // Construct sound with buffer (SFML 3)
    sound = new sf::Sound(*buffer);
  } catch (const std::exception& e) {
    std::cerr << "Exception creating AudioClip: " << e.what() << std::endl;
    if (sound) { delete sound; sound = nullptr; }
    if (buffer) { delete buffer; buffer = nullptr; }
  }
#endif
}

void AudioClip::SetVolume(float volume)
{
#ifdef SFML_AUDIO_AVAILABLE
  if (sound) {
    sound->setVolume(volume);
  }
#endif
}

AudioClip::~AudioClip()
{
#ifdef SFML_AUDIO_AVAILABLE
  if (sound) { delete sound; sound = nullptr; }
  if (buffer) { delete buffer; buffer = nullptr; }
#endif
}

// Copy constructor
AudioClip::AudioClip(const AudioClip& other)
{
  audioUrl = other.audioUrl;
#ifdef SFML_AUDIO_AVAILABLE
  sound = nullptr;
  buffer = nullptr;
  if (!other.audioUrl.empty()) {
    try {
      buffer = new sf::SoundBuffer();
      if (buffer && buffer->loadFromFile(audioUrl)) {
        sound = new sf::Sound(*buffer);
      }
    } catch (...) {
      if (sound) { delete sound; sound = nullptr; }
      if (buffer) { delete buffer; buffer = nullptr; }
    }
  }
#endif
}

// Copy assignment
AudioClip& AudioClip::operator=(const AudioClip& other)
{
  if (this == &other) return *this;
  audioUrl = other.audioUrl;
#ifdef SFML_AUDIO_AVAILABLE
  if (sound) { delete sound; sound = nullptr; }
  if (buffer) { delete buffer; buffer = nullptr; }
  if (!other.audioUrl.empty()) {
    try {
      buffer = new sf::SoundBuffer();
      if (buffer && buffer->loadFromFile(audioUrl)) {
        sound = new sf::Sound(*buffer);
      }
    } catch (...) {
      if (sound) { delete sound; sound = nullptr; }
      if (buffer) { delete buffer; buffer = nullptr; }
    }
  }
#endif
  return *this;
}

// Move constructor
AudioClip::AudioClip(AudioClip&& other) noexcept
{
  audioUrl = std::move(other.audioUrl);
#ifdef SFML_AUDIO_AVAILABLE
  sound = other.sound; other.sound = nullptr;
  buffer = other.buffer; other.buffer = nullptr;
#endif
}

// Move assignment
AudioClip& AudioClip::operator=(AudioClip&& other) noexcept
{
  if (this == &other) return *this;
  audioUrl = std::move(other.audioUrl);
#ifdef SFML_AUDIO_AVAILABLE
  if (sound) { delete sound; sound = nullptr; }
  if (buffer) { delete buffer; buffer = nullptr; }
  sound = other.sound; other.sound = nullptr;
  buffer = other.buffer; other.buffer = nullptr;
#endif
  return *this;
}

#ifdef SFML_AUDIO_AVAILABLE
void AudioClip::Play(sf::SoundBuffer&)
{
  if (!sound) {
    std::cerr << "Sound object is null, cannot play audio" << std::endl;
    return;
  }
  try { sound->play(); }
  catch (const std::exception& e) { std::cerr << "Exception playing audio: " << e.what() << std::endl; }
}
#endif
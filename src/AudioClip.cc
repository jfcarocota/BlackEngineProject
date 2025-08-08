#include "AudioClip.hh"
#include <iostream>

AudioClip::AudioClip()
{

}
AudioClip::AudioClip(const char* audioUrl)
{
  this->audioUrl = audioUrl;
  
#ifdef SFML_AUDIO_AVAILABLE
  try {
    sound = new sf::Sound();
    if (!sound) {
      std::cerr << "Failed to create SFML Sound object" << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception creating AudioClip: " << e.what() << std::endl;
    sound = nullptr;
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
  if (sound) {
    delete sound;
  }
#endif
}

#ifdef SFML_AUDIO_AVAILABLE
void AudioClip::Play(sf::SoundBuffer& buffer)
{
  if (!sound) {
    std::cerr << "Sound object is null, cannot play audio" << std::endl;
    return;
  }
  
  try {
    if (!buffer.loadFromFile(audioUrl)) {
      std::cerr << "Failed to load audio file: " << audioUrl << std::endl;
      return;
    }
    sound->setBuffer(buffer);
    sound->play();
  } catch (const std::exception& e) {
    std::cerr << "Exception playing audio: " << e.what() << std::endl;
  }
}
#endif
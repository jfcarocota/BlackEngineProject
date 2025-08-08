#include "AudioClip.hh"
#include <iostream>

AudioClip::AudioClip()
{

}
AudioClip::AudioClip(const char* audioUrl)
{
#ifdef SFML_AUDIO_AVAILABLE
  sound = new sf::Sound();
#endif
  this->audioUrl = audioUrl;
}
void AudioClip::SetVolume(float volume)
{
#ifdef SFML_AUDIO_AVAILABLE
  sound->setVolume(volume);
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
  if (!buffer.loadFromFile(audioUrl)) {
    std::cerr << "Failed to load audio file: " << audioUrl << std::endl;
    return;
  }
  sound->setBuffer(buffer);
  sound->play();
}
#endif
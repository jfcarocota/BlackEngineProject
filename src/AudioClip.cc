#include "AudioClip.hh"

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
}

#ifdef SFML_AUDIO_AVAILABLE
void AudioClip::Play(sf::SoundBuffer& buffer)
{
  sound->setBuffer(buffer);
  buffer.loadFromFile(audioUrl);
  sound->play();
}
#endif
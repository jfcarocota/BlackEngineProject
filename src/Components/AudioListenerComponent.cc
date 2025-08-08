#include "Components/AudioListenerComponent.hh"

AudioListenerComponent::AudioListenerComponent()
{
#ifdef SFML_AUDIO_AVAILABLE
  this->soundBuffer = sf::SoundBuffer();
#endif
}

AudioListenerComponent::~AudioListenerComponent()
{
}

void AudioListenerComponent::Initialize()
{

}

AudioClip* AudioListenerComponent::GetAudioClip() const
{
  return audioClip;
}

void AudioListenerComponent::Play()
{
#ifdef SFML_AUDIO_AVAILABLE
  if(audioClip)
  {
    audioClip->SetVolume(1.f);
    audioClip->Play(soundBuffer);
  }
#endif
}

void AudioListenerComponent::PlayOneShot(AudioClip& audioClip, float audioVolume)
{
#ifdef SFML_AUDIO_AVAILABLE
  audioClip.SetVolume(audioVolume);
  audioClip.Play(soundBuffer);
#endif
}

void AudioListenerComponent::PlayOneShot(AudioClip& audioClip)
{
#ifdef SFML_AUDIO_AVAILABLE
  audioClip.SetVolume(1.f);
  audioClip.Play(soundBuffer);
#endif
}
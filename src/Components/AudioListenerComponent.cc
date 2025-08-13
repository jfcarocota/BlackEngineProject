#include "Components/AudioListenerComponent.hh"

#include <gsl/assert>
#include <iostream>

AudioListenerComponent::AudioListenerComponent() {
#ifdef SFML_AUDIO_AVAILABLE
  this->soundBuffer = sf::SoundBuffer();
  audioClip = nullptr;
#endif
}

AudioListenerComponent::~AudioListenerComponent() {}

void AudioListenerComponent::Initialize() {
  // No owner dependencies here, but document invariant for future changes
  Ensures(true);
}

AudioClip* AudioListenerComponent::GetAudioClip() const { return audioClip; }

void AudioListenerComponent::Play() {
#ifdef SFML_AUDIO_AVAILABLE
  if (audioClip) {
    audioClip->SetVolume(1.f);
    audioClip->Play(soundBuffer);
  }
#endif
}

void AudioListenerComponent::PlayOneShot(AudioClip& audioClip,
                                         float audioVolume) {
#ifdef SFML_AUDIO_AVAILABLE
  try {
    audioClip.SetVolume(audioVolume);
    audioClip.Play(soundBuffer);
  } catch (const std::exception& e) {
    std::cerr << "Exception in PlayOneShot: " << e.what() << std::endl;
  }
#endif
}

void AudioListenerComponent::PlayOneShot(AudioClip& audioClip) {
#ifdef SFML_AUDIO_AVAILABLE
  try {
    audioClip.SetVolume(1.f);
    audioClip.Play(soundBuffer);
  } catch (const std::exception& e) {
    std::cerr << "Exception in PlayOneShot: " << e.what() << std::endl;
  }
#endif
}
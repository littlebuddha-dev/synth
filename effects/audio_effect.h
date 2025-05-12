// synth/effects/audio_effect.h
#pragma once
#include <vector> 

class AudioEffect {
public:
  virtual ~AudioEffect() = default;
  
  virtual void processStereoSample(float inL, float inR, float& outL, float& outR) = 0;

  void setEnabled(bool enabledStatus) { this->enabled = enabledStatus; }
  bool isEnabled() const { return enabled; }

protected:
  bool enabled = true;
  float sampleRate = 44100.0f; 
};
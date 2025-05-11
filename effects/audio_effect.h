// synth/effects/audio_effect.h
#pragma once
#include <vector> // For potential use in derived classes

class AudioEffect {
public:
  virtual ~AudioEffect() = default;
  
  // Processes a stereo sample pair. Input samples are inL, inR.
  // Output samples should be written back to outL, outR.
  virtual void processStereoSample(float inL, float inR, float& outL, float& outR) = 0;

  void setEnabled(bool enabledStatus) { this->enabled = enabledStatus; }
  bool isEnabled() const { return enabled; }

protected:
  bool enabled = true;
  float sampleRate = 44100.0f; // Default, can be set by constructor
};
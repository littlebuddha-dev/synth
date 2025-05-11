// synth/effects/audio_effect.h
#pragma once
#include <vector> // For potential use in derived classes

class AudioEffect {
public:
  virtual ~AudioEffect() = default;
  virtual float processSample(float inputSample) = 0;

  void setEnabled(bool enabledStatus) { this->enabled = enabledStatus; }
  bool isEnabled() const { return enabled; }

protected:
  bool enabled = true;
  float sampleRate = 44100.0f; // Default, can be set by constructor
};
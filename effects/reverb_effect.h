// synth/effects/reverb_effect.h
#pragma once
#include "audio_effect.h"
#include <algorithm> // For std::clamp
#include <cmath>
#include <vector>

// Forward declaration for internal filter classes if complex
// For simplicity, basic filter logic can be within ReverbEffect or simple
// helper structs


class ReverbEffect : public AudioEffect {
private:
  // Simple internal comb filter
  class CombFilter {
  public:
    CombFilter(int sr, float delayMs, float feedback, float damping);
    float process(float input);
    void setDelay(float delayMs);
    void setFeedback(float fb);
    void setDamping(float damp); // 0 (no damp) to 1 (full damp)
  private:
    int sampleRate;
    std::vector<float> buffer;
    int bufferSize;
    int writePos;
    float currentFeedback;
    float currentDamping; // filter coefficient for LPF in feedback loop
    float filterStore;    // previous filtered value
  };

  // Simple internal all-pass filter
  class AllPassFilter {
  public:
    AllPassFilter(int sr, float delayMs, float feedback);
    float process(float input);
    void setDelay(float delayMs);
    void setFeedback(float fb);

  private:
    int sampleRate;
    std::vector<float> buffer;
    int bufferSize;
    int writePos;
    float currentFeedback;
  };

  std::vector<CombFilter> combFilters;
  std::vector<AllPassFilter> allPassFilters;

  float dryWetMix;
  float roomSize; // Affects delay times
  float damping;  // Affects comb filter damping

  // Tuned delay times for comb and all-pass filters (in milliseconds)
  // These are just example values and would need careful tuning
  const std::vector<float> baseCombDelayTimes = {29.7f, 37.1f, 41.1f, 43.7f}; // Example: 4 comb filters
  const std::vector<float> baseAllPassDelayTimes = {5.0f, 1.7f}; // Example: 2 all-pass filters
  // Initial feedbacks - RT60 will override comb feedbacks mostly
  const std::vector<float> baseCombFeedbacks = {0.80f, 0.78f, 0.75f, 0.72f}; 
  const std::vector<float> baseAllPassFeedbacks = {0.5f, 0.5f}; // All-pass feedback often around 0.5-0.7

public:
  ReverbEffect(float sr);
  ~ReverbEffect() override = default;

  float processSample(float inputSample) override;

  void setDryWetMix(float mix); // 0.0 (dry) to 1.0 (wet)
  void setRoomSize(float size); // 0.0 to 1.0
  void setDamping(float damp);  // 0.0 to 1.0
  
  float wetGain = 1.0f;  // デフォルト 100%
  void setWetGain(float gain);
  
  float rt60 = 1.2f; // デフォルト残響時間（秒）
  void setRT60(float seconds);
  // Process a stereo sample pair. Output samples are written to outL and outR.
  void processStereoSample(float inL, float inR, float &outL, float &outR);



private:
  void updateParameters();
};
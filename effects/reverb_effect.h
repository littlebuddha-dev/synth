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

  std::vector<CombFilter> combFiltersL; // Left channel comb filters
  std::vector<CombFilter> combFiltersR; // Right channel comb filters
  std::vector<AllPassFilter> allPassFiltersL; // Left channel all-pass filters
  std::vector<AllPassFilter> allPassFiltersR; // Right channel all-pass filters


  float dryWetMix;
  float roomSize; // Affects delay times
  float damping;  // Affects comb filter damping

  // Tuned delay times for comb and all-pass filters (in milliseconds)
  // These are just example values and would need careful tuning
  // For stereo, provide slightly different times for L/R
  const std::vector<float> baseCombDelayTimesL = {29.7f, 37.1f, 41.1f, 43.7f}; 
  const std::vector<float> baseCombDelayTimesR = {30.1f, 38.3f, 41.9f, 44.3f}; // Slightly offset
  const std::vector<float> baseAllPassDelayTimesL = {5.0f, 1.7f}; 
  const std::vector<float> baseAllPassDelayTimesR = {5.3f, 1.9f}; // Slightly offset
  
  const std::vector<float> baseCombFeedbacks = {0.80f, 0.78f, 0.75f, 0.72f}; 
  const std::vector<float> baseAllPassFeedbacks = {0.5f, 0.5f}; 

public:
  ReverbEffect(float sr);
  ~ReverbEffect() override = default;

  // This mono version could be deprecated or call the stereo version with L=R
  // float processSample(float inputSample) override; 
  void processStereoSample(float inL, float inR, float &outL, float &outR) override;


  void setDryWetMix(float mix); // 0.0 (dry) to 1.0 (wet)
  void setRoomSize(float size); // 0.0 to 1.0
  void setDamping(float damp);  // 0.0 to 1.0
  
  float wetGain = 1.0f;  // デフォルト 100%
  void setWetGain(float gain);
  
  float rt60 = 1.2f; // デフォルト残響時間（秒）
  void setRT60(float seconds);


private:
  void updateParameters();
};
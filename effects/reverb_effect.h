// synth/effects/reverb_effect.h
#pragma once
#include "audio_effect.h"
#include <algorithm> 
#include <cmath>
#include <vector>

class ReverbEffect : public AudioEffect {
private:
  class CombFilter {
  public:
    CombFilter(int sr, float delayMs, float initialFeedback, float dampingCutoffHz);
    float process(float input);
    void setDelay(float delayMs);
    float getDelayMs() const; 
    void setFeedback(float fb);
    void setDampingCutoff(float cutoffHz); 

  private:
    int sampleRate_;
    std::vector<float> buffer_;
    int bufferSize_;
    int writePos_;
    float currentFeedback_;
    float dampingAlpha_; 
    float filterStore_;    
    float delayMs_; 
  };

  class AllPassFilter {
  public:
    AllPassFilter(int sr, float delayMs, float feedback);
    float process(float input);
    void setDelay(float delayMs);
    void setFeedback(float fb);

  private:
    int sampleRate_;
    std::vector<float> buffer_;
    int bufferSize_;
    int writePos_;
    float currentFeedback_;
  };

  const std::vector<float> baseCombDelayTimesL = {
      29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 61.3f, 67.7f, 73.3f 
  }; 
  const std::vector<float> baseCombDelayTimesR = { 
      30.1f, 38.3f, 41.9f, 44.3f, 54.7f, 62.1f, 68.1f, 74.1f
  }; 
  const std::vector<float> baseAllPassDelayTimesL = {5.0f, 1.7f, 6.1f, 2.3f}; 
  const std::vector<float> baseAllPassDelayTimesR = {5.3f, 1.9f, 6.3f, 2.5f};
  
  const std::vector<float> baseAllPassFeedbacks = {0.5f, 0.5f, 0.5f, 0.5f}; 

  std::vector<CombFilter> combFiltersL; 
  std::vector<CombFilter> combFiltersR; 
  std::vector<AllPassFilter> allPassFiltersL; 
  std::vector<AllPassFilter> allPassFiltersR; 


  float dryWetMix_;     
  float roomSize_;      
  float dampingParam_;  
  float rt60_;          
  float wetGain_;       

public:
  ReverbEffect(float sr);
  ~ReverbEffect() override = default;

  void processStereoSample(float inL, float inR, float &outL, float &outR) override;

  void setDryWetMix(float mix); 
  float getDryWetMix() const { return dryWetMix_; }

  void setRoomSize(float size); 
  float getRoomSize() const { return roomSize_; }
  
  void setDamping(float dampParam); 
  float getDamping() const { return dampingParam_; }
  
  void setWetGain(float gain);
  float getWetGain() const { return wetGain_; }
  
  void setRT60(float seconds);
  float getRT60() const { return rt60_; }

private:
  void updateParameters();
  float calculateDampingCutoffHz(float dampingParamValue); 
};
// synth/effects/reverb_effect.cpp
#include "reverb_effect.h"
#include <algorithm> 
#include <numeric>   
#include <cmath>     

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// --- CombFilter Implementation ---
ReverbEffect::CombFilter::CombFilter(int sr, float delayMs, float /*initialFeedback*/,
                                     float dampingCutoffHz) 
    : sampleRate_(sr), writePos_(0), currentFeedback_(0.7f), // Initial feedback set by updateParameters
      filterStore_(0.0f), delayMs_(delayMs), dampingAlpha_(0.5f) { // dampingAlpha_ init
  setDelay(delayMs); 
  setDampingCutoff(dampingCutoffHz); 
}

float ReverbEffect::CombFilter::process(float input) {
    if (buffer_.empty()) {
        return input; 
    }
    float readVal = buffer_[writePos_];
    
    // Apply LPF to the signal from the delay line (feedback damping)
    // y[n] = (1-alpha) * x[n] + alpha * y[n-1]
    // Here, readVal is x[n], filterStore_ is y[n-1]
    filterStore_ = (1.0f - dampingAlpha_) * readVal + dampingAlpha_ * filterStore_;

    float outputToBuffer = input + filterStore_ * currentFeedback_; 
    
    buffer_[writePos_] = std::clamp(outputToBuffer, -2.0f, 2.0f); 
    
    writePos_ = (writePos_ + 1) % buffer_.size(); 
    return filterStore_; 
}

void ReverbEffect::CombFilter::setDelay(float delayMs) {
  delayMs_ = delayMs; 
  bufferSize_ = static_cast<int>(delayMs_ * 0.001f * static_cast<float>(sampleRate_));
  if (bufferSize_ < 1) {
    bufferSize_ = 1;
  }
  if (static_cast<int>(buffer_.size()) != bufferSize_) { 
    buffer_.assign(bufferSize_, 0.0f); 
    // writePos_ = 0; // Resetting writePos on delay change can be problematic.
                   // If buffer resizes, it's implicitly handled by assign or new allocation.
                   // If only delayMs changes but not bufferSize due to rounding, fine.
  }
  // filterStore_ = 0.0f; // Resetting filter state on delay change is often good.
}

float ReverbEffect::CombFilter::getDelayMs() const {
    return delayMs_;
}

void ReverbEffect::CombFilter::setFeedback(float fb) {
  currentFeedback_ = std::clamp(fb, 0.0f, 0.999f); 
}

void ReverbEffect::CombFilter::setDampingCutoff(float cutoffHz) {
   // alpha = exp(-2*PI*fc/fs) for y[n] = (1-a)x[n] + a y[n-1]
   // High fc (less damping) -> small alpha. Low fc (more damping) -> large alpha (near 1).
   if (cutoffHz >= static_cast<float>(sampleRate_) * 0.499f) { // Effectively no damping (Nyquist limit)
       dampingAlpha_ = 0.0f; // So (1-alpha) is 1, passes input directly
   } else if (cutoffHz <= 1.0f) { // Max damping (very low cutoff)
       dampingAlpha_ = 0.9999f; // Close to 1, holds previous value mostly
   } else {
       dampingAlpha_ = std::exp(-2.0f * static_cast<float>(M_PI) * cutoffHz / static_cast<float>(sampleRate_));
   }
   dampingAlpha_ = std::clamp(dampingAlpha_, 0.0f, 0.9999f);
}


// --- AllPassFilter Implementation ---
ReverbEffect::AllPassFilter::AllPassFilter(int sr, float delayMs,
                                           float feedback)
    : sampleRate_(sr), writePos_(0), currentFeedback_(feedback), bufferSize_(0) { // bufferSize_ init
  setDelay(delayMs);
}

float ReverbEffect::AllPassFilter::process(float input) {
  if (buffer_.empty()) {
    return input; 
  }
  float buf_out = buffer_[writePos_];
  // Schroeder Allpass: y[n] = -g*x[n] + x[n-M] + g*y[n-M] is complex to implement directly with one buffer.
  // Common variant: y[n] = x[n-M] - g * (x[n] - y[n-M]) (nested structure)
  // Simpler for non-nested:
  //   delayed_val = buffer[read_pos];
  //   output = -feedback * input + delayed_val;
  //   buffer[write_pos] = input + feedback * output;
  float output = -currentFeedback_ * input + buf_out;
  buffer_[writePos_] = std::clamp(input + currentFeedback_ * output, -2.0f, 2.0f);

  writePos_ = (writePos_ + 1) % buffer_.size();
  return output;
}

void ReverbEffect::AllPassFilter::setDelay(float delayMs) {
  bufferSize_ = static_cast<int>(delayMs * 0.001f * static_cast<float>(sampleRate_));
  if (bufferSize_ < 1) {
    bufferSize_ = 1;
  }
   if (static_cast<int>(buffer_.size()) != bufferSize_) {
       buffer_.assign(bufferSize_, 0.0f);
      //  writePos_ = 0; // Reset if buffer reallocated.
   }
}

void ReverbEffect::AllPassFilter::setFeedback(float fb) {
  currentFeedback_ = std::clamp(fb, -0.99f, 0.99f); 
}

// --- ReverbEffect Implementation ---
ReverbEffect::ReverbEffect(float sr)
    : dryWetMix_(0.3f), roomSize_(0.5f), dampingParam_(0.5f), rt60_(1.2f), wetGain_(1.0f) {
  this->sampleRate = sr; // sampleRate from AudioEffect base class
  
  combFiltersL.reserve(baseCombDelayTimesL.size());
  for (size_t i = 0; i < baseCombDelayTimesL.size(); ++i) {
    combFiltersL.emplace_back(static_cast<int>(this->sampleRate), baseCombDelayTimesL[i], 0.7f, 5000.0f); 
  }
  combFiltersR.reserve(baseCombDelayTimesR.size());
  for (size_t i = 0; i < baseCombDelayTimesR.size(); ++i) {
    combFiltersR.emplace_back(static_cast<int>(this->sampleRate), baseCombDelayTimesR[i], 0.7f, 5000.0f);
  }
  
  allPassFiltersL.reserve(baseAllPassDelayTimesL.size());
  for (size_t i = 0; i < baseAllPassDelayTimesL.size(); ++i) {
    allPassFiltersL.emplace_back(static_cast<int>(this->sampleRate), baseAllPassDelayTimesL[i], baseAllPassFeedbacks[i % baseAllPassFeedbacks.size()]);
  }
  allPassFiltersR.reserve(baseAllPassDelayTimesR.size());
   for (size_t i = 0; i < baseAllPassDelayTimesR.size(); ++i) {
    allPassFiltersR.emplace_back(static_cast<int>(this->sampleRate), baseAllPassDelayTimesR[i], baseAllPassFeedbacks[i % baseAllPassFeedbacks.size()]);
  }
  updateParameters(); 
}


void ReverbEffect::processStereoSample(float inL, float inR, float &outL, float &outR) {
  if (!enabled) {
    outL = inL;
    outR = inR;
    return;
  }

  float wetSignalAccumulatorL = 0.0f;
  float wetSignalAccumulatorR = 0.0f;

  for (auto &comb : combFiltersL) {
    wetSignalAccumulatorL += comb.process(inL);
  }
  for (auto &comb : combFiltersR) {
    wetSignalAccumulatorR += comb.process(inR);
  }

  // Normalization factor (empirical, often 1/sqrt(numCombs) or tuned by ear)
  // Can be absorbed into wetGain_ or individual comb gains.
  // For simplicity, let's assume filter outputs are already reasonably scaled.
  // float normL = combFiltersL.empty() ? 1.0f : 0.25f; // Example fixed gain factor
  // float normR = combFiltersR.empty() ? 1.0f : 0.25f;
  // wetSignalAccumulatorL *= normL;
  // wetSignalAccumulatorR *= normR;


  float diffusedL = wetSignalAccumulatorL;
  float diffusedR = wetSignalAccumulatorR;
  for (auto &apf : allPassFiltersL) {
    diffusedL = apf.process(diffusedL);
  }
  for (auto &apf : allPassFiltersR) {
    diffusedR = apf.process(diffusedR);
  }
  
  float finalWetL = std::tanh(diffusedL * wetGain_); 
  float finalWetR = std::tanh(diffusedR * wetGain_);

  outL = inL * (1.0f - dryWetMix_) + finalWetL * dryWetMix_;
  outR = inR * (1.0f - dryWetMix_) + finalWetR * dryWetMix_;
}


void ReverbEffect::setDryWetMix(float mix) {
  dryWetMix_ = std::clamp(mix, 0.0f, 1.0f);
}

void ReverbEffect::setRoomSize(float size) {
  roomSize_ = std::clamp(size, 0.0f, 1.0f);
  updateParameters();
}

float ReverbEffect::calculateDampingCutoffHz(float dampingParamValue) {
    float minCutoff = 500.0f;   // More damping = lower cutoff
    float maxCutoff = 20000.0f; // Less damping = higher cutoff
    if (this->sampleRate > 0) { // Ensure sampleRate is valid
         maxCutoff = std::min(maxCutoff, static_cast<float>(this->sampleRate) * 0.49f);
    }


    if (dampingParamValue <= 0.0f) return maxCutoff;
    if (dampingParamValue >= 1.0f) return minCutoff;

    // Logarithmic mapping: param 0 -> maxCutoff, param 1 -> minCutoff
    float logMin = std::log(minCutoff);
    float logMax = std::log(maxCutoff);
    return std::exp(logMax - dampingParamValue * (logMax - logMin));
}

void ReverbEffect::setDamping(float dampParam) {
  dampingParam_ = std::clamp(dampParam, 0.0f, 1.0f);
  updateParameters(); 
}

void ReverbEffect::setWetGain(float gain) {
  wetGain_ = std::clamp(gain, 0.0f, 2.0f);  
}

void ReverbEffect::setRT60(float seconds) {
  rt60_ = std::clamp(seconds, 0.05f, 20.0f); 
  updateParameters(); 
}


void ReverbEffect::updateParameters() {
  float roomDelayScale = 0.5f + roomSize_ * 1.0f; 
  float calculatedDampingCutoff = calculateDampingCutoffHz(dampingParam_);

  for (size_t i = 0; i < combFiltersL.size(); ++i) {
    float currentDelayMs = baseCombDelayTimesL[i] * roomDelayScale;
    combFiltersL[i].setDelay(currentDelayMs);
    float delaySeconds = currentDelayMs * 0.001f;
    if (rt60_ < 0.01f) rt60_ = 0.01f; // Avoid log(0) or division by zero
    float feedback = std::pow(10.0f, (-3.0f * delaySeconds) / rt60_);
    combFiltersL[i].setFeedback(feedback);
    combFiltersL[i].setDampingCutoff(calculatedDampingCutoff);
  }
  for (size_t i = 0; i < combFiltersR.size(); ++i) {
    float currentDelayMs = baseCombDelayTimesR[i] * roomDelayScale;
    combFiltersR[i].setDelay(currentDelayMs);
    float delaySeconds = currentDelayMs * 0.001f;
    if (rt60_ < 0.01f) rt60_ = 0.01f;
    float feedback = std::pow(10.0f, (-3.0f * delaySeconds) / rt60_);
    combFiltersR[i].setFeedback(feedback);
    combFiltersR[i].setDampingCutoff(calculatedDampingCutoff);
  }

  for (size_t i = 0; i < allPassFiltersL.size(); ++i) {
    allPassFiltersL[i].setDelay(baseAllPassDelayTimesL[i] * roomDelayScale);
    allPassFiltersL[i].setFeedback(baseAllPassFeedbacks[i % baseAllPassFeedbacks.size()]); 
  }
   for (size_t i = 0; i < allPassFiltersR.size(); ++i) {
    allPassFiltersR[i].setDelay(baseAllPassDelayTimesR[i] * roomDelayScale);
    allPassFiltersR[i].setFeedback(baseAllPassFeedbacks[i % baseAllPassFeedbacks.size()]);
  }
}
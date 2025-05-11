// synth/effects/reverb_effect.cpp
#include "reverb_effect.h"
#include <algorithm> // For std::clamp
#include <numeric>   // For std::accumulate if needed (現在は未使用)
#include <cmath>     // For std::abs in optional debug, std::tanh, std::sqrt

// --- CombFilter Implementation ---
ReverbEffect::CombFilter::CombFilter(int sr, float delayMs, float feedback,
                                     float dampingVal)
    : sampleRate(sr), writePos(0), currentFeedback(feedback),
      filterStore(0.0f) {
  setDelay(delayMs);
  setDamping(dampingVal);
}

float ReverbEffect::CombFilter::process(float input) {
    if (buffer.empty()) {
        return input; // Should not happen if initialized
    }
    float readVal = buffer[writePos];
    // Damping LPF: filterStore is the output of the LPF
    filterStore = (readVal * (1.0f - currentDamping)) + (filterStore * currentDamping);
    float outputToBuffer = input + filterStore * currentFeedback;
    
    // Clamp to prevent runaway due to extreme feedback/input
    buffer[writePos] = std::clamp(outputToBuffer, -2.0f, 2.0f); // Wider clamp for internal buffer
    
    writePos = (writePos + 1) % bufferSize;
    return filterStore; // Return the (damped) delayed signal
}

void ReverbEffect::CombFilter::setDelay(float delayMs) {
  bufferSize = static_cast<int>(delayMs * 0.001f * sampleRate);
  if (bufferSize < 1) {
    bufferSize = 1;
  }
  buffer.assign(bufferSize, 0.0f); // Initialize buffer with zeros
  writePos = 0;
  filterStore = 0.0f; // Reset filter state
}

void ReverbEffect::CombFilter::setFeedback(float fb) {
  currentFeedback =
      std::clamp(fb, 0.0f, 0.999f); // Keep feedback slightly below 1 for stability
}

void ReverbEffect::CombFilter::setDamping(float damp) {
  currentDamping = std::clamp(damp, 0.0f, 1.0f);
}

// --- AllPassFilter Implementation ---
ReverbEffect::AllPassFilter::AllPassFilter(int sr, float delayMs,
                                           float feedback)
    : sampleRate(sr), writePos(0), currentFeedback(feedback) {
  setDelay(delayMs);
}

float ReverbEffect::AllPassFilter::process(float input) {
  if (buffer.empty()) {
    return input; // Should not happen
  }

  float delayedSample = buffer[writePos];
  
  // Standard Schroeder All-Pass structure
  float output = -input * currentFeedback + delayedSample;
  buffer[writePos] = std::clamp(input + output * currentFeedback, -2.0f, 2.0f); // Wider clamp

  writePos = (writePos + 1) % bufferSize;
  return output;
}

void ReverbEffect::AllPassFilter::setDelay(float delayMs) {
  bufferSize = static_cast<int>(delayMs * 0.001f * sampleRate);
  if (bufferSize < 1) {
    bufferSize = 1;
  }
  buffer.assign(bufferSize, 0.0f);
  writePos = 0;
}
void ReverbEffect::AllPassFilter::setFeedback(float fb) {
  currentFeedback =
      std::clamp(fb, -0.99f, 0.99f); // All-pass feedback can be negative
}

// --- ReverbEffect Implementation ---
ReverbEffect::ReverbEffect(float sr)
    : dryWetMix(0.3f), roomSize(0.5f), damping(0.5f), rt60(1.2f), wetGain(1.0f) {
  this->sampleRate = sr;
  // Initialize filters for L and R channels
  for (size_t i = 0; i < baseCombDelayTimesL.size(); ++i) {
    combFiltersL.emplace_back(sampleRate, baseCombDelayTimesL[i],
                             baseCombFeedbacks[i], damping);
  }
  for (size_t i = 0; i < baseCombDelayTimesR.size(); ++i) {
    combFiltersR.emplace_back(sampleRate, baseCombDelayTimesR[i],
                             baseCombFeedbacks[i], damping);
  }
  for (size_t i = 0; i < baseAllPassDelayTimesL.size(); ++i) {
    allPassFiltersL.emplace_back(sampleRate, baseAllPassDelayTimesL[i],
                                baseAllPassFeedbacks[i]);
  }
   for (size_t i = 0; i < baseAllPassDelayTimesR.size(); ++i) {
    allPassFiltersR.emplace_back(sampleRate, baseAllPassDelayTimesR[i],
                                baseAllPassFeedbacks[i]);
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

  // Parallel Comb Filters for L channel
  for (auto &comb : combFiltersL) {
    wetSignalAccumulatorL += comb.process(inL);
  }
  // Parallel Comb Filters for R channel
  for (auto &comb : combFiltersR) {
    wetSignalAccumulatorR += comb.process(inR);
  }

  float scaledWetL = 0.0f;
  float scaledWetR = 0.0f;

  if (!combFiltersL.empty()) {
    scaledWetL = (wetSignalAccumulatorL / std::sqrt(static_cast<float>(combFiltersL.size()))) * wetGain;
  }
   if (!combFiltersR.empty()) {
    scaledWetR = (wetSignalAccumulatorR / std::sqrt(static_cast<float>(combFiltersR.size()))) * wetGain;
  }


  // Series All-Pass Filters for L channel
  for (auto &apf : allPassFiltersL) {
    scaledWetL = apf.process(scaledWetL);
  }
  // Series All-Pass Filters for R channel
  for (auto &apf : allPassFiltersR) {
    scaledWetR = apf.process(scaledWetR);
  }

  // Soft Limiter (tanh is a common choice)
  float finalWetL = std::tanh(scaledWetL);
  float finalWetR = std::tanh(scaledWetR);

  // Dry/Wet Mix
  outL = inL * (1.0f - dryWetMix) + finalWetL * dryWetMix;
  outR = inR * (1.0f - dryWetMix) + finalWetR * dryWetMix;
}


void ReverbEffect::setDryWetMix(float mix) {
  dryWetMix = std::clamp(mix, 0.0f, 1.0f);
}

void ReverbEffect::setRoomSize(float size) {
  roomSize = std::clamp(size, 0.0f, 1.0f);
  updateParameters();
}

void ReverbEffect::setDamping(float damp) {
  damping = std::clamp(damp, 0.0f, 1.0f);
  updateParameters(); 
}

void ReverbEffect::setWetGain(float gain) {
  wetGain = std::clamp(gain, 0.0f, 2.0f);  
}

void ReverbEffect::setRT60(float seconds) {
  rt60 = std::clamp(seconds, 0.1f, 10.0f); 
  updateParameters(); // RT60 change requires updating feedbacks
}


void ReverbEffect::updateParameters() {
  float roomScaleFactor = 0.2f + roomSize * 0.8f; // Scale delay times more meaningfully (e.g. 0.2 to 1.0 of base)

  for (size_t i = 0; i < combFiltersL.size(); ++i) {
    float scaledDelayL = baseCombDelayTimesL[i] * roomScaleFactor;
    combFiltersL[i].setDelay(scaledDelayL);
    float feedbackL = std::pow(10.0f, (-3.0f * scaledDelayL * 0.001f) / rt60); // delay in seconds for formula
    combFiltersL[i].setFeedback(std::min(feedbackL, 0.995f)); // Clamp feedback
    combFiltersL[i].setDamping(damping);
  }
  for (size_t i = 0; i < combFiltersR.size(); ++i) {
    float scaledDelayR = baseCombDelayTimesR[i] * roomScaleFactor;
    combFiltersR[i].setDelay(scaledDelayR);
    float feedbackR = std::pow(10.0f, (-3.0f * scaledDelayR * 0.001f) / rt60);
    combFiltersR[i].setFeedback(std::min(feedbackR, 0.995f));
    combFiltersR[i].setDamping(damping);
  }

  for (size_t i = 0; i < allPassFiltersL.size(); ++i) {
    allPassFiltersL[i].setDelay(baseAllPassDelayTimesL[i] * roomScaleFactor);
    // All-pass feedback is often kept constant, or can be part of roomSize tuning.
    // For now, use baseAllPassFeedbacks set in constructor.
     allPassFiltersL[i].setFeedback(baseAllPassFeedbacks[i]); 
  }
   for (size_t i = 0; i < allPassFiltersR.size(); ++i) {
    allPassFiltersR[i].setDelay(baseAllPassDelayTimesR[i] * roomScaleFactor);
    allPassFiltersR[i].setFeedback(baseAllPassFeedbacks[i]);
  }
}
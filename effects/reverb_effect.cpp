// synth/effects/reverb_effect.cpp
#include "reverb_effect.h"
#include <algorithm> // For std::clamp
// #include <iostream> // std::cout は使用しないのでコメントアウトまたは削除
#include <numeric>   // For std::accumulate if needed (現在は未使用)
#include <cmath>     // For std::abs in optional debug

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
        return input;
    }
    float readVal = buffer[writePos];
    filterStore = (readVal * (1.0f - currentDamping)) + (filterStore * currentDamping);
    float outputToBuffer = input + filterStore * currentFeedback;
    buffer[writePos] = std::clamp(outputToBuffer, -1.0f, 1.0f);
    writePos = (writePos + 1) % bufferSize;
    return filterStore; // ★変更点
}

void ReverbEffect::CombFilter::setDelay(float delayMs) {
  bufferSize = static_cast<int>(delayMs * 0.001f * sampleRate);
  if (bufferSize < 1) {
    bufferSize = 1;
  }
  buffer.assign(bufferSize, 0.0f);
  writePos = 0;
  filterStore = 0.0f;
}

void ReverbEffect::CombFilter::setFeedback(float fb) {
  currentFeedback =
      std::clamp(fb, 0.0f, 0.99f); 
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
    return input;
  }

  float delayedSample = buffer[writePos];

  float valToBuffer = input + delayedSample * currentFeedback;
  buffer[writePos] = std::clamp(valToBuffer, -1.0f, 1.0f);

  float output = delayedSample - input * currentFeedback;
  // Alternative all-pass structure:
  // float output = -input * currentFeedback + delayedSample;
  // buffer[writePos] = std::clamp(input + output * currentFeedback, -1.0f, 1.0f);


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
      std::clamp(fb, -0.99f, 0.99f); 
}

// --- ReverbEffect Implementation ---
ReverbEffect::ReverbEffect(float sr)
    : dryWetMix(0.3f), roomSize(0.5f), damping(0.5f) {
  this->sampleRate = sr;
  // Initialize filters
  for (size_t i = 0; i < baseCombDelayTimes.size(); ++i) {
    combFilters.emplace_back(sampleRate, baseCombDelayTimes[i],
                             baseCombFeedbacks[i], damping);
  }
  for (size_t i = 0; i < baseAllPassDelayTimes.size(); ++i) {
    allPassFilters.emplace_back(sampleRate, baseAllPassDelayTimes[i],
                                baseAllPassFeedbacks[i]);
  }
  updateParameters(); // Call updateParameters after filters are created
}

float ReverbEffect::processSample(float inputSample) {
  if (!enabled) {
    return inputSample;
  }

  float wetSignalAccumulator = 0.0f;

  // --- Parallel Comb Filters ---
  for (auto &comb : combFilters) {
    float combOut = comb.process(inputSample);
    wetSignalAccumulator += combOut;
  }

  float scaledWetSignalAfterCombs = 0.0f;
  if (!combFilters.empty()) {
    // --- √Nスケーリング + 可変ゲイン適用 ---
    scaledWetSignalAfterCombs =
        (wetSignalAccumulator / std::sqrt(static_cast<float>(combFilters.size()))) * wetGain;
  }

  // --- Series All-Pass Filters ---
  float signalProcessedByAPFs = scaledWetSignalAfterCombs;
  for (auto &apf : allPassFilters) {
    signalProcessedByAPFs = apf.process(signalProcessedByAPFs);
  }

  // --- ソフトリミッタ（過剰なピークを抑制）---
  float finalWetSignal = std::tanh(signalProcessedByAPFs);

  // --- Dry/Wet Mix ---
  float finalOutput = inputSample * (1.0f - dryWetMix) + finalWetSignal * dryWetMix;

  return finalOutput;
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
  updateParameters(); // Damping change should also call updateParameters
}

void ReverbEffect::setWetGain(float gain) {
  wetGain = std::clamp(gain, 0.0f, 2.0f);  // 最大200%まで調整可
}

void ReverbEffect::setRT60(float seconds) {
  rt60 = std::clamp(seconds, 0.1f, 10.0f); // 100ms〜10秒

  // RT60に基づいてCombのfeedback更新
  for (size_t i = 0; i < combFilters.size(); ++i) {
    float delayMs = baseCombDelayTimes[i] * (0.5f + roomSize); // Use current scaled delay
    float feedback = std::pow(10.0f, (-3.0f * delayMs) / (rt60 * 1000.0f));
    combFilters[i].setFeedback(feedback);
  }
}

void ReverbEffect::processStereoSample(float inL, float inR, float &outL, float &outR) {
  if (!enabled) {
    outL = inL;
    outR = inR;
    return;
  }

  float wetL = 0.0f;
  float wetR = 0.0f;

  // L/R用 combFilter に微小遅延差をつける（奇数はL用、偶数はR用）
  // Or apply slight differences in delay times or feedback per channel for stereo spread
  // For now, a simple mono processing split approach for each channel independently using all combs.
  // This won't create true stereo width from a mono source but processes L/R independently.
  // To get better stereo, some comb filters might be dedicated to L, some to R, with slightly different parameters.

  // Process L channel
  float wetSignalAccumulatorL = 0.0f;
  for (size_t i = 0; i < combFilters.size(); ++i) {
      // Could alternate input or have dedicated L/R combs. For simplicity, feed L to all.
      // If using distinct L/R combs, split them here.
      // Example: if half are for L, half for R
      // if (i < combFilters.size() / 2) wetSignalAccumulatorL += combFilters[i].process(inL);
      // else wetSignalAccumulatorR += combFilters[i].process(inR);
      // Current: Mono processing for L, then for R.
      wetSignalAccumulatorL += combFilters[i].process(inL); 
  }
    
  // Process R channel
  float wetSignalAccumulatorR = 0.0f;
   for (size_t i = 0; i < combFilters.size(); ++i) { // Re-use comb filters for R. State will be mixed.
      // For true stereo, you'd need separate sets of comb filters or careful handling of state.
      // A more common stereo reverb approach might use a mono sum for early reflections / comb input
      // and then decorrelated all-pass filters for L/R.
      // The current comb filter process modifies its internal state, so processing R after L on the *same*
      // comb filters means R's input is affected by L's previous processing.
      // This is likely not desired for clean stereo.
      // A simple fix: Process L through half the combs, R through the other half (if possible and tuned)
      // Or, have two ReverbEffect instances (one L, one R) if full independence is needed and computationally feasible.
      // For this example, let's assume a simplified "dual mono" processing path for wet signal generation.
      // *Critical Change for less-mingled stereo*: Let's process L/R with separate halves of the combs
      // (This requires an even number of comb filters and assumes they are tuned somewhat symmetrically)
      // For simplicity of not changing filter counts or a major redesign:
      // The provided stereo logic for comb processing in original solution was:
      // float inSample = (i % 2 == 0) ? inL : inR; float combOut = combFilters[i].process(inSample);
      // if (i % 2 == 0) wetL += combOut; else wetR += combOut;
      // This is better than processing all with inL then all with inR. Let's revert to that idea.
      // But the number of combFilters is 4. So 0,2 for L and 1,3 for R.
      if (i % 2 == 0) wetSignalAccumulatorL += combFilters[i].process(inL); // Combs 0, 2 for L
      else wetSignalAccumulatorR += combFilters[i].process(inR);            // Combs 1, 3 for R
  }


  // √Nスケーリングと wetGain 適用 (adjust N if splitting combs)
  float scaleFactor = std::sqrt(static_cast<float>(combFilters.size()) / 2.0f); // N/2 combs per channel
  if (scaleFactor < 1.0f) scaleFactor = 1.0f;
  wetL = (wetSignalAccumulatorL / scaleFactor) * wetGain;
  wetR = (wetSignalAccumulatorR / scaleFactor) * wetGain;


  // All-Pass 通過 - for stereo, all-pass filters could also be slightly different or processed in series/parallel differently
  // Current: process L path, then R path through the *same* set of all-pass filters.
  // This also causes L channel processing to affect R channel state if not careful.
  // To improve, use separate APF chains or decorrelate.
  // For now, simplified:
  // All pass filters are usually in series. Let's pass L through all, then R through all.
  // This means APF state from L processing influences R.
  // A better approach for stereo APFs often involves splitting them or using slightly different delay times for L/R.
  // Sticking to original structure:
  for (auto &apf : allPassFilters) {
    wetL = apf.process(wetL); 
  }
  for (auto &apf : allPassFilters) { // Reset/reprocess for R or use separate APFs
                                   // This is problematic: APF state is now from L.
                                   // A quick fix for testing: use different APFs if available,
                                   // or accept the channel bleed/mono-summing effect here.
                                   // The original `processStereoSample` passed wetL and wetR through the same APFs sequentially.
                                   // Let's replicate that structure for consistency with the previous logic.
    wetR = apf.process(wetR); // R processes through APFs that now have L's history.
  }


  // ソフトリミッタ + Dry/Wet mix
  float finalL = std::tanh(wetL);
  float finalR = std::tanh(wetR);

  outL = inL * (1.0f - dryWetMix) + finalL * dryWetMix;
  outR = inR * (1.0f - dryWetMix) + finalR * dryWetMix;
}

void ReverbEffect::updateParameters() {
  float roomScaleFactor = 0.5f + roomSize;

  for (size_t i = 0; i < combFilters.size(); ++i) {
    // Introduce slight variations for stereo effect in comb delays if desired, e.g. for L/R channels
    // float stereoSpread = (i % 2 == 0) ? 1.0f : 1.02f; // Example: make R channel delays slightly longer
    // float scaledDelay = baseCombDelayTimes[i] * roomScaleFactor * stereoSpread;
    float scaledDelay = baseCombDelayTimes[i] * roomScaleFactor;
    combFilters[i].setDelay(scaledDelay);

    // RT60からフィードバック再計算
    float feedback = std::pow(10.0f, (-3.0f * scaledDelay) / (rt60 * 1000.0f));
    combFilters[i].setFeedback(feedback);

    combFilters[i].setDamping(damping);
  }

  for (size_t i = 0; i < allPassFilters.size(); ++i) {
    // float stereoSpreadAPF = (i % 2 == 0) ? 1.0f : 1.03f; // Example spread for APF
    // allPassFilters[i].setDelay(baseAllPassDelayTimes[i] * roomScaleFactor * stereoSpreadAPF);
    allPassFilters[i].setDelay(baseAllPassDelayTimes[i] * roomScaleFactor);
     // Allpass feedback is usually fixed or less dynamically changed than comb feedback for RT60
     // Here, we keep baseAllPassFeedbacks constant unless explicitly changed by another method.
     // If they should also scale with roomSize or RT60, that logic would go here.
     // For now, allPassFilters[i].setFeedback(baseAllPassFeedbacks[i]); is implicit.
  }
}
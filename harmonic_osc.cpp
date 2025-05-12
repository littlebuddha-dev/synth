// synth/harmonic_osc.cpp
#include "harmonic_osc.h"
#include <cmath>
#include <algorithm> 

HarmonicOscillator::HarmonicOscillator(int sr, int numHarmonics_)
    : sampleRate(sr), numHarmonics(numHarmonics_), 
      baseFreq(440.0f), phase(0.0f), gateOpen(false), waveform(Waveform::Sine),
      // lfos and envelopes vectors are initialized but not directly used for osc modulation in current global design
      // Consider if these should be per-harmonic modulators or if this was for a different design.
      // For now, their presence doesn't harm, but they aren't wired into the process() output.
      lfos(numHarmonics), // Assuming default LFO constructor or provide params
      envelopes(numHarmonics), // Assuming default Envelope constructor
      noiseDist(-1.0f, 1.0f), // This is fine if used, otherwise remove
      rng(std::random_device{}()), // This is fine if used, otherwise remove
      pulseWidth(0.5f), pwmDepth(0.0f), currentPWMSourceValue(0.0f), 
      polyModPWValue(0.0f), wheelModPWValue(0.0f), driftPWValue(0.0f)
{
    harmonicAmplitudes_.resize(numHarmonics, 0.0f);
    if (numHarmonics > 0) {
        harmonicAmplitudes_[0] = 1.0f; 
    }
}

void HarmonicOscillator::setFrequency(float freq) {
    baseFreq = std::max(0.0f, freq);
}

float HarmonicOscillator::getBaseFrequency() const { 
    return baseFreq; 
}

void HarmonicOscillator::setWaveform(Waveform wf) {
    waveform = wf;
}

Waveform HarmonicOscillator::getWaveform() const {
    return waveform;
}

void HarmonicOscillator::noteOn() {
    gateOpen = true;
    // If envelopes/lfos per harmonic were intended, they'd be triggered here.
}

void HarmonicOscillator::noteOff() {
    gateOpen = false;
    // If envelopes/lfos per harmonic were intended, they'd be released here.
}

bool HarmonicOscillator::isRunning() const {
    // An oscillator might be considered "running" if its gate is open OR if an envelope tied to it is still active.
    // For simplicity now, only gateOpen.
    return gateOpen; 
}

bool HarmonicOscillator::isGateOpen() const {
    return gateOpen;
}

float HarmonicOscillator::process() {
    // --- 2x Oversampling ---
    float outSample = 0.0f;
    constexpr int oversamplingFactor = 2; // Or make this a member/configurable
    float subSamples[oversamplingFactor];

    // float originalSampleRate = static_cast<float>(sampleRate); // Not directly needed if sampleRate is member
    float oversampledRate = static_cast<float>(sampleRate * oversamplingFactor);
    // float phaseIncrementPerSubSample = baseFreq / oversampledRate; // baseFreq can change due to FM

    for (int i = 0; i < oversamplingFactor; ++i) {
        // Calculate effective frequency for this sub-sample if FM is sample-by-sample
        // For now, baseFreq is assumed constant across sub-samples within one main sample period.
        float currentEffectiveFreq = std::max(0.0f, baseFreq);


        float currentPhasePos = std::fmod(phase, 1.0f);
        if (currentPhasePos < 0.0f) currentPhasePos += 1.0f;

        float sample_component = 0.0f;
        switch (waveform) {
            case Waveform::Sine:
                sample_component = std::sin(2.0f * static_cast<float>(M_PI) * currentPhasePos);
                break;
            case Waveform::Saw:
                sample_component = 2.0f * currentPhasePos - 1.0f;
                // PolyBLEP for Saw:
                // float t = currentPhasePos; // Phase [0, 1)
                // float inc = currentEffectiveFreq / oversampledRate;
                // sample_component = 2.0f * t - 1.0f;
                // sample_component -= poly_blep(t, inc); // Subtract BLEP at discontinuity
                break;
            case Waveform::Square:
                sample_component = (currentPhasePos < 0.5f) ? 1.0f : -1.0f;
                // PolyBLEP for Square:
                // float t = currentPhasePos;
                // float inc = currentEffectiveFreq / oversampledRate;
                // sample_component = (t < 0.5f) ? 1.0f : -1.0f;
                // sample_component += poly_blep(t, inc);          // Add BLEP at 0
                // sample_component -= poly_blep(std::fmod(t + 0.5f, 1.0f), inc); // Subtract BLEP at 0.5
                break;
            case Waveform::Triangle:
                 if (currentPhasePos < 0.5f) {
                    sample_component = -1.0f + 4.0f * currentPhasePos;
                } else {
                    sample_component = 1.0f - 4.0f * (currentPhasePos - 0.5f);
                }
                break;
            case Waveform::Pulse: {
                float lfo_pwm_effect = pwmDepth * currentPWMSourceValue;
                float total_pwm_offset = lfo_pwm_effect + 
                                         polyModPWValue + 
                                         wheelModPWValue + 
                                         driftPWValue;
                float effectivePW = pulseWidth + total_pwm_offset;
                effectivePW = std::clamp(effectivePW, 0.01f, 0.99f);
                sample_component = (currentPhasePos < effectivePW) ? 1.0f : -1.0f;
                // PolyBLEP for Pulse (similar to Square but with 'effectivePW')
                // float t = currentPhasePos;
                // float inc = currentEffectiveFreq / oversampledRate;
                // sample_component = (t < effectivePW) ? 1.0f : -1.0f;
                // sample_component += poly_blep(t, inc); // Add BLEP at 0
                // sample_component -= poly_blep(std::fmod(t - effectivePW + 1.0f, 1.0f), inc); // Subtract BLEP at effectivePW
                break;
            }
            case Waveform::Additive: {
                sample_component = 0.0f;
                for (int h_idx = 0; h_idx < numHarmonics; ++h_idx) {
                    if (harmonicAmplitudes_[h_idx] != 0.0f) {
                        sample_component += harmonicAmplitudes_[h_idx] * std::sin(2.0f * static_cast<float>(M_PI) * currentPhasePos * static_cast<float>(h_idx + 1));
                    }
                }
                break;
            }
        }
        subSamples[i] = sample_component;

        phase += currentEffectiveFreq / oversampledRate;
        // Sync will reset phase externally if needed. If sync happens mid-subsample block, this won't catch it perfectly without more logic.
    }

    if (phase >= 1.0f) phase -= std::floor(phase); 
    else if (phase < 0.0f) phase -= std::floor(phase);


    outSample = 0.0f;
    for (int i = 0; i < oversamplingFactor; ++i) {
        outSample += subSamples[i];
    }
    outSample /= static_cast<float>(oversamplingFactor); // Simple averaging for downsampling

    return outSample;
}


// Placeholder for PolyBLEP function - this needs a proper implementation
// float HarmonicOscillator::poly_blep(float t, float dt) {
//     if (t < dt) { // Around t = 0
//         t /= dt;
//         return t + t - t * t - 1.0f;
//     } else if (t > 1.0f - dt) { // Around t = 1
//         t = (t - 1.0f) / dt;
//         return t * t + t + t + 1.0f; // Mistake in formula, should be similar to the t<dt case but for falling edge
//     }
//     return 0.0f;
// }
// A more standard PolyBLEP for a unit amplitude step at t=0:
// if (t < dt) { t = t / dt - 1.0; return -(t*t - 1.0); } // or return t*t + 2*t + 1 for other polarity
// else if (t > 1.0 - dt) { t = (t - 1.0) / dt + 1.0; return t*t -1.0; } // or -(t*t - 2*t +1)
// This still needs careful derivation for saw/square.
// For now, PolyBLEP is NOT integrated above to keep it simpler.


void HarmonicOscillator::resetPhase() {
    phase = 0.0f;
}

float HarmonicOscillator::getPhase() const {
    return phase;
}

void HarmonicOscillator::setPulseWidth(float width) {
    pulseWidth = std::clamp(width, 0.01f, 0.99f);
}

void HarmonicOscillator::setPWMDepth(float depth) {
    pwmDepth = std::clamp(depth, 0.0f, 1.0f);
}

void HarmonicOscillator::setPWMSource(float value) {
    currentPWMSourceValue = value;
}
void HarmonicOscillator::setPolyModPWValue(float value) {
    polyModPWValue = value;
}

void HarmonicOscillator::sync() {
    // Phase reset should ideally happen *at the exact sample (or sub-sample) point*
    // of the sync trigger. If sync is checked once per main `process()` call in `Voice`,
    // the reset will apply from the next block of sub-samples.
    // For perfect sync, the `sync()` might need to be aware of sub-sample timing
    // or `process()` would need to handle sync trigger within its sub-sample loop.
    phase = 0.0f;
}

void HarmonicOscillator::setWheelModPWValue(float value) {
    wheelModPWValue = value;
}

void HarmonicOscillator::setDriftPWValue(float value) {
    driftPWValue = value;
}

void HarmonicOscillator::setHarmonicAmplitude(int harmonicIndex, float amplitude) {
    if (harmonicIndex >= 0 && harmonicIndex < numHarmonics) {
        harmonicAmplitudes_[harmonicIndex] = std::clamp(amplitude, 0.0f, 1.0f);
    }
}

float HarmonicOscillator::getHarmonicAmplitude(int harmonicIndex) const {
    if (harmonicIndex >= 0 && harmonicIndex < numHarmonics) {
        return harmonicAmplitudes_[harmonicIndex];
    }
    return 0.0f;
}
// synth/harmonic_osc.cpp
#include "harmonic_osc.h"
#include <cmath>
#include <algorithm> // Required for std::clamp

HarmonicOscillator::HarmonicOscillator(int sampleRate, int numHarmonics)
    : sampleRate(sampleRate), numHarmonics(numHarmonics), 
      baseFreq(440.0f), phase(0.0f), gateOpen(false), waveform(Waveform::Sine),
      // amplitudes vector was unused, harmonicAmplitudes_ will be used instead
      lfos(numHarmonics, LFO(sampleRate)), 
      envelopes(numHarmonics, Envelope(0.01f, 0.1f, 0.7f, 0.3f, sampleRate)),
      noiseDist(-1.0f, 1.0f), rng(std::random_device{}()),
      pulseWidth(0.5f), pwmDepth(0.0f), currentPWMSourceValue(0.0f), 
      polyModPWValue(0.0f), wheelModPWValue(0.0f), driftPWValue(0.0f)
{
    harmonicAmplitudes_.resize(numHarmonics, 0.0f);
    if (numHarmonics > 0) {
        harmonicAmplitudes_[0] = 1.0f; // Default to fundamental at full amplitude
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
}

void HarmonicOscillator::noteOff() {
    gateOpen = false;
}

bool HarmonicOscillator::isRunning() const {
    return gateOpen; 
}

bool HarmonicOscillator::isGateOpen() const {
    return gateOpen;
}

float HarmonicOscillator::process() {
    float effectiveFreq = baseFreq;
    effectiveFreq = std::max(0.0f, effectiveFreq);

    float sample = 0.0f;
    float currentPhasePos = std::fmod(phase, 1.0f);
    if (currentPhasePos < 0.0f) currentPhasePos += 1.0f;

    switch (waveform) {
        case Waveform::Sine:
            sample = std::sin(2.0f * M_PI * currentPhasePos);
            break;
        case Waveform::Saw:
            sample = 2.0f * (currentPhasePos - std::floor(currentPhasePos + 0.5f));
            break;
        case Waveform::Square:
            sample = (currentPhasePos < 0.5f) ? 1.0f : -1.0f;
            break;
        case Waveform::Triangle:
             if (currentPhasePos < 0.5f) {
                sample = -1.0f + 4.0f * currentPhasePos;
            } else {
                sample = 1.0f - 4.0f * (currentPhasePos - 0.5f);
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
            sample = (currentPhasePos < effectivePW) ? 1.0f : -1.0f;
            break;
        }
        case Waveform::Additive: {
            sample = 0.0f;
            // numHarmonics is the size of harmonicAmplitudes_ vector
            // Loop up to numHarmonics, h is 0-based index for amplitudes
            // (h+1) is the harmonic number (1st, 2nd, ...)
            for (int h = 0; h < numHarmonics; ++h) {
                if (harmonicAmplitudes_[h] != 0.0f) {
                    sample += harmonicAmplitudes_[h] * std::sin(2.0f * M_PI * currentPhasePos * static_cast<float>(h + 1));
                }
            }
            // Note: Sum of amplitudes might exceed 1.0. Clipping might occur or further normalization might be desired.
            break;
        }
    }

    phase += effectiveFreq / static_cast<float>(sampleRate);
    if (phase >= 1.0f) phase -= std::floor(phase); 
    else if (phase < 0.0f) phase -= std::floor(phase);

    return sample;
}

// void HarmonicOscillator::setNoiseLevel(float level) {
    // noiseLevel member was unused in process(), Voice class handles noise separately.
    // noiseLevel = level; // コメントアウトまたは削除
// }

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
    phase = 0.0f;
}

void HarmonicOscillator::setWheelModPWValue(float value) {
    wheelModPWValue = value;
}

void HarmonicOscillator::setDriftPWValue(float value) {
    driftPWValue = value;
}

void HarmonicOscillator::setHarmonicAmplitude(int harmonicIndex, float amplitude) {
    // harmonicIndex is 0-based (0 for fundamental, 1 for 2nd harm, etc.)
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
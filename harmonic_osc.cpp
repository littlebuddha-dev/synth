// synth/harmonic_osc.cpp
#include "harmonic_osc.h"
#include <cmath>
#include <algorithm> // Required for std::clamp

HarmonicOscillator::HarmonicOscillator(int sampleRate, int numHarmonics)
    : sampleRate(sampleRate), numHarmonics(numHarmonics), 
      baseFreq(440.0f), phase(0.0f), gateOpen(false), waveform(Waveform::Sine),
      amplitudes(numHarmonics, 0.0f), 
      lfos(numHarmonics, LFO(sampleRate)), 
      envelopes(numHarmonics, Envelope(0.01f, 0.1f, 0.7f, 0.3f, sampleRate)),
      noiseDist(-1.0f, 1.0f), rng(std::random_device{}()),
      pulseWidth(0.5f), pwmDepth(0.0f), currentPWMSourceValue(0.0f), 
      polyModPWValue(0.0f), wheelModPWValue(0.0f), driftPWValue(0.0f)
{
    // Constructor body
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
    }

    phase += effectiveFreq / static_cast<float>(sampleRate);
    if (phase >= 1.0f) phase -= std::floor(phase); 
    else if (phase < 0.0f) phase -= std::floor(phase);

    return sample;
}

void HarmonicOscillator::setNoiseLevel(float level) {
    noiseLevel = level;
}

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

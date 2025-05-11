// synth/lfo.h
#pragma once
#include <cmath>
#include <algorithm> // For std::clamp
#include <random>    // For random number generation

enum class LfoWaveform {
    Triangle,
    SawUp, // Ramp Up
    Square,
    Sine,
    RandomStep // New: Sample & Hold random waveform
};

class LFO {
public:
    LFO(float sampleRate = 44100.0f)
        : rate(1.0f), depth(1.0f), sampleRate(sampleRate), phase(0.0f), 
          waveform(LfoWaveform::Triangle),
          randomEngine(std::random_device{}()), // Initialize random engine
          randomDistribution(-1.0f, 1.0f),    // Initialize distribution
          lastRandomValue(0.0f),
          samplesUntilNextRandomStep(0) {
            updateSamplesPerStep(); // Calculate initial samples per step
          }

    void setRate(float r) { 
        rate = std::max(0.01f, r); // Rate should not be negative or zero to avoid division by zero
        updateSamplesPerStep();
    }
    void setDepth(float d) { depth = std::clamp(d, 0.0f, 1.0f); } // Depth 0.0 to 1.0
    void setWaveform(LfoWaveform wf) { 
        waveform = wf; 
        if (waveform == LfoWaveform::RandomStep) {
            // Reset phase or counter for RandomStep if needed, or generate first value immediately
            samplesUntilNextRandomStep = 0; 
        } else {
            phase = 0.0f; // Reset phase for other waveforms
        }
    }
    void resetPhase() { 
        phase = 0.0f; 
        if (waveform == LfoWaveform::RandomStep) {
            samplesUntilNextRandomStep = 0;
        }
    }

    float getRate() const { return rate; }
    float getDepth() const { return depth; }
    LfoWaveform getWaveform() const { return waveform; }


    // Returns value in range -1.0 to 1.0 (for bipolar waveforms like Sine, Triangle, Square, RandomStep)
    // or 0.0 to 1.0 (for unipolar like SawUp, if desired, but typically LFOs are bipolar)
    // For Prophet-5 like behavior, most LFOs are bipolar.
    float step() {
        float val = 0.0f;

        if (waveform == LfoWaveform::RandomStep) {
            if (samplesUntilNextRandomStep <= 0) {
                lastRandomValue = randomDistribution(randomEngine);
                updateSamplesPerStep(); // Recalculate steps for next period
                samplesUntilNextRandomStep = samplesPerStep;
            }
            val = lastRandomValue;
            samplesUntilNextRandomStep--;
        } else {
            phase += rate / sampleRate;
            if (phase >= 1.0f) phase -= 1.0f;

            switch (waveform) {
                case LfoWaveform::Sine:
                    val = std::sin(2.0f * M_PI * phase);
                    break;
                case LfoWaveform::Triangle:
                    // Bipolar: -1.0 to 1.0
                    if (phase < 0.5f) {
                        val = -1.0f + 4.0f * phase; // Ramps from -1 to 1 for phase 0 to 0.5
                    } else {
                        val = 1.0f - 4.0f * (phase - 0.5f); // Ramps from 1 to -1 for phase 0.5 to 1
                    }
                    break;
                case LfoWaveform::SawUp: // Ramp up, then reset (Prophet-5 has ramp)
                    // Bipolar: -1.0 to 1.0 for consistency
                    val = 2.0f * phase - 1.0f;
                    break;
                case LfoWaveform::Square:
                    val = (phase < 0.5f) ? 1.0f : -1.0f;
                    break;
                case LfoWaveform::RandomStep:
                    // This case is handled above to avoid phase logic
                    break;
            }
        }
        return depth * val;
    }

private:
    void updateSamplesPerStep() {
        if (rate > 0.0f) {
            samplesPerStep = static_cast<int>(sampleRate / rate);
            if (samplesPerStep < 1) samplesPerStep = 1;
        } else {
            samplesPerStep = sampleRate; // Default to 1 second if rate is 0 (should be avoided by setRate clamp)
        }
    }

    float rate;      // Hz
    float depth;     // 0.0 to 1.0
    int sampleRate;
    float phase;
    LfoWaveform waveform;

    // For RandomStep (Sample & Hold)
    std::default_random_engine randomEngine;
    std::uniform_real_distribution<float> randomDistribution;
    float lastRandomValue;
    int samplesPerStep; // How many samples to hold the current random value
    int samplesUntilNextRandomStep;
};
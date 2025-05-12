// synth/lfo.h
#pragma once
#include <cmath>
#include <algorithm> 
#include <random>    

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

enum class LfoWaveform {
    Triangle,
    SawUp, 
    Square,
    Sine,
    RandomStep 
};

class LFO {
public:
    LFO(float sampleRate = 44100.0f)
        : rate(1.0f), depth(1.0f), sampleRate_(sampleRate), phase(0.0f), 
          waveform(LfoWaveform::Triangle),
          randomEngine(std::random_device{}()), 
          randomDistribution(-1.0f, 1.0f),    
          lastRandomValue(0.0f),
          samplesUntilNextRandomStep(0) {
            updateSamplesPerStep(); 
          }

    void setRate(float r) { 
        rate = std::max(0.01f, r); 
        updateSamplesPerStep();
    }
    void setDepth(float d) { depth = std::clamp(d, 0.0f, 1.0f); } 
    void setWaveform(LfoWaveform wf) { 
        waveform = wf; 
        if (waveform == LfoWaveform::RandomStep) {
            samplesUntilNextRandomStep = 0; 
        } else {
            phase = 0.0f; 
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


    float step() {
        float val = 0.0f;

        if (waveform == LfoWaveform::RandomStep) {
            if (samplesUntilNextRandomStep <= 0) {
                lastRandomValue = randomDistribution(randomEngine);
                updateSamplesPerStep(); 
                samplesUntilNextRandomStep = samplesPerStep;
            }
            val = lastRandomValue;
            samplesUntilNextRandomStep--;
        } else {
            phase += rate / sampleRate_;
            if (phase >= 1.0f) phase -= 1.0f;

            switch (waveform) {
                case LfoWaveform::Sine:
                    val = std::sin(2.0f * static_cast<float>(M_PI) * phase);
                    break;
                case LfoWaveform::Triangle:
                    if (phase < 0.5f) {
                        val = -1.0f + 4.0f * phase; 
                    } else {
                        val = 1.0f - 4.0f * (phase - 0.5f); 
                    }
                    break;
                case LfoWaveform::SawUp: 
                    val = 2.0f * phase - 1.0f;
                    break;
                case LfoWaveform::Square:
                    val = (phase < 0.5f) ? 1.0f : -1.0f;
                    break;
                case LfoWaveform::RandomStep: // Should be handled by the block above
                    break; 
            }
        }
        return depth * val;
    }

private:
    void updateSamplesPerStep() {
        if (rate > 0.0f) {
            samplesPerStep = static_cast<int>(sampleRate_ / rate);
            if (samplesPerStep < 1) samplesPerStep = 1;
        } else {
            samplesPerStep = static_cast<int>(sampleRate_); 
        }
    }

    float rate;      
    float depth;     
    float sampleRate_; // Renamed to avoid conflict if global `sampleRate` exists
    float phase;
    LfoWaveform waveform;

    std::default_random_engine randomEngine;
    std::uniform_real_distribution<float> randomDistribution;
    float lastRandomValue;
    int samplesPerStep; 
    int samplesUntilNextRandomStep;
};
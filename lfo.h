// synth/lfo.h
#pragma once
#include <cmath>
#include <algorithm> // For std::clamp

enum class LfoWaveform {
    Triangle,
    SawUp, // Ramp Up
    Square,
    Sine
};

class LFO {
public:
    LFO(float sampleRate = 44100.0f)
        : rate(1.0f), depth(1.0f), sampleRate(sampleRate), phase(0.0f), waveform(LfoWaveform::Triangle) {}

    void setRate(float r) { rate = std::max(0.0f, r); } // Rate should not be negative
    void setDepth(float d) { depth = std::clamp(d, 0.0f, 1.0f); } // Depth 0.0 to 1.0
    void setWaveform(LfoWaveform wf) { waveform = wf; }
    void resetPhase() { phase = 0.0f; }

    float getRate() const { return rate; }
    float getDepth() const { return depth; }
    LfoWaveform getWaveform() const { return waveform; }


    // Returns value in range -1.0 to 1.0 (for bipolar waveforms like Sine, Triangle, Square)
    // or 0.0 to 1.0 (for unipolar like SawUp, if desired, but typically LFOs are bipolar)
    // For Prophet-5 like behavior, most LFOs are bipolar.
    float step() {
        phase += rate / sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;

        float val = 0.0f;
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
                // Bipolar: -1.0 to 1.0 for consistency, or 0.0 to 1.0 if preferred.
                // A common synth LFO saw might be -1 to 1.
                val = 2.0f * phase - 1.0f;
                break;
            case LfoWaveform::Square:
                val = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
        }
        return depth * val;
    }

private:
    float rate;      // Hz
    float depth;     // 0.0 to 1.0
    int sampleRate;
    float phase;
    LfoWaveform waveform;
};
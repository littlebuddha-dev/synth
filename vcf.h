// synth/vcf.h
#pragma once
#include <algorithm> // For std::clamp
#include <cmath>     // For M_PI, pow, log2, sinf

class VCF {
public:
    VCF(float sampleRate = 44100.0f);

    void setBaseCutoff(float hz); // Renamed from setCutoff to clarify it's the panel/base setting
    float getBaseCutoff() const { return baseCutoffHz; }
    void setResonance(float q);
    void setKeyFollow(float factor);
    void setEnvelopeMod(float amount);
    void setNote(int midiNote); // For key follow calculation
    void setEnvelopeValue(float env); // Input from filter envelope
    
    // Process with additional direct modulation input (e.g., from LFO, PolyMod) in Hz
    float process(float input, float directModHz);

private:
    float baseCutoffHz; // Base cutoff frequency from UI/settings
    float resonance;    // 0.0 to 1.0 (approx)
    float keyFollow;    // 0.0 to 1.0
    float envModAmount; // -1.0 to 1.0, scales envelopeValue
    float envelopeValue = 0.0f; // Current value from filter envelope (0.0 to 1.0)
    float noteBaseFreq = 440.0f; // Frequency of the current note for key tracking
    float sampleRate;
    float z[4]; // filter state for 4-pole
};
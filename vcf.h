// synth/vcf.h
#pragma once
#include <algorithm> 
#include <cmath>     
#include "synth_parameters.h" // For SynthParams::FilterType

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

class VCF {
public:
    VCF(float sampleRate = 44100.0f);

    void setType(SynthParams::FilterType type); // New: Set filter type
    SynthParams::FilterType getType() const;    // New: Get current filter type

    void setBaseCutoff(float hz); 
    float getBaseCutoff() const { return baseCutoffHz; }
    void setResonance(float q); // Resonance 0.0 to 1.0 (mapped to Q internally)
    void setKeyFollow(float factor);
    void setEnvelopeMod(float amount);
    void setNote(int midiNote); 
    void setEnvelopeValue(float env); 
    
    float process(float input, float directModHz);

private:
    void calculateCoefficients(float cutoffHz, float resonanceValue); // Helper to calc coeffs

    SynthParams::FilterType currentFilterType_; // New
    float baseCutoffHz; 
    float resonance;    // User-facing resonance (0-1)
    float keyFollow;    
    float envModAmount; 
    float envelopeValue = 0.0f; 
    float noteBaseFreq = 440.0f; 
    float sampleRate;

    // State variables for Ladder Filter (LPF24)
    float z_ladder_[4]; 

    // State variables for State Variable Filter (SVF)
    float s1_svf_, s2_svf_; // Integrator states for SVF (Chamberlin form)
    
    // SVF Coefficients (calculated by calculateCoefficients)
    float svf_f_, svf_q_coeff_; // svf_q_coeff is related to 1/Q

    // Common effective cutoff after all modulations
    float currentEffectiveCutoffHz_;
};